#include <glm/gtc/type_ptr.hpp>
#include "Mesh.h"

VkPrimitiveTopology toVulkan(uint32_t MeshType){
    switch (MeshType) {
        case aiPrimitiveType_POINT: return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
        case aiPrimitiveType_LINE: return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
        case aiPrimitiveType_TRIANGLE: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        default:
            throw std::runtime_error{"unknown primitive type"};
    }
}

mesh::Mesh loadMesh(std::string_view parent, const aiNode* node, const aiScene* scene, uint32_t* numVertices, uint32_t meshId){
    auto aiMesh = scene->mMeshes[node->mMeshes[meshId]];
    mesh::Mesh mesh;
    mesh.name = aiMesh->mName.C_Str();
    mesh.primitiveTopology = toVulkan(aiMesh->mPrimitiveTypes);
    *numVertices = aiMesh->mNumVertices;
    for(auto j = 0u; j < aiMesh->mNumVertices; j++){
        Vertex vertex{};
        auto aiVec = aiMesh->mVertices[j];


        vertex.position = {aiVec.x, aiVec.y, aiVec.z, 1.0f};
        mesh.bounds.min = glm::min(mesh.bounds.min, glm::vec3(vertex.position));
        mesh.bounds.max = glm::max(mesh.bounds.max, glm::vec3(vertex.position));

        if(aiMesh->HasNormals()){
            auto aiNorm = aiMesh->mNormals[j];
            vertex.normal = {aiNorm.x, aiNorm.y, aiNorm.z};
        }

        if(aiMesh->HasTangentsAndBitangents()){
            auto aiTan = aiMesh->mTangents[j];
            auto aiBi = aiMesh->mBitangents[j];
            vertex.tangent = {aiTan.x, aiTan.y, aiTan.z};
            vertex.bitangent = {aiBi.x, aiBi.y, aiBi.z};
        }

        if(aiMesh->HasVertexColors(0)){
            auto aiCol = aiMesh->mColors[j][0];
            vertex.color = {aiCol.r, aiCol.g, aiCol.b, aiCol.a};
        }

        if(aiMesh->HasTextureCoords((0))){  // TODO retrieve up to AI_MAX_NUMBER_OF_TEXTURECOORDS textures coordinates
            auto aiTex = aiMesh->mTextureCoords[0][j];
            vertex.uv = {aiTex.x, aiTex.y};
        }
        mesh.vertices.push_back(vertex);
    }

    if(aiMesh->HasFaces()){
        for(auto j = 0; j < aiMesh->mNumFaces; j++){
            auto face = aiMesh->mFaces[j];
            for(auto k = 0; k < face.mNumIndices; k++){
                mesh.indices.push_back(face.mIndices[k]);
            }
        }
    }

    auto aiMaterial = scene->mMaterials[aiMesh->mMaterialIndex];
    mesh::Material material;
    aiString aiString;

    auto ret = aiMaterial->Get(AI_MATKEY_NAME, aiString);
    if(ret == aiReturn_SUCCESS){
        material.name = aiString.C_Str();
    }

    float scalar = 1.0f;
    ret = aiMaterial->Get(AI_MATKEY_OPACITY, scalar);
    if(ret == aiReturn_SUCCESS){
        material.opacity = scalar;
    }

    ret = aiMaterial->Get(AI_MATKEY_REFRACTI, scalar);
    if(ret == aiReturn_SUCCESS){
        material.indexOfRefraction = scalar;
    }

    ret = aiMaterial->Get(AI_MATKEY_SHININESS, scalar);
    if(ret == aiReturn_SUCCESS){
        material.shininess = scalar;
    }

    glm::vec3 color;
    uint32_t size = 3;
    ret = aiMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, glm::value_ptr(color), &size);
    if(ret == aiReturn_SUCCESS){
        material.diffuse = color;
    }

    ret = aiMaterial->Get(AI_MATKEY_COLOR_AMBIENT, glm::value_ptr(color), &size);
    if(ret == aiReturn_SUCCESS){
        material.ambient = color;
    }

    ret = aiMaterial->Get(AI_MATKEY_COLOR_SPECULAR, glm::value_ptr(color), &size);
    if(ret == aiReturn_SUCCESS){
        material.specular = color;
    }

    ret = aiMaterial->Get(AI_MATKEY_COLOR_EMISSIVE, glm::value_ptr(color), &size);
    if(ret == aiReturn_SUCCESS){
        material.emission = color;
    }

    ret = aiMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &aiString);
    if(ret == aiReturn_SUCCESS){
        material.diffuseMap = aiString.C_Str(); // FIXME prepend parent path
    }

    ret = aiMaterial->GetTexture(aiTextureType_AMBIENT, 0, &aiString);
    if(ret == aiReturn_SUCCESS){
        material.ambientMap = aiString.C_Str();
    }

    ret = aiMaterial->GetTexture(aiTextureType_SPECULAR, 0, &aiString);
    if(ret == aiReturn_SUCCESS){
        material.specularMap = aiString.C_Str();
    }

    ret = aiMaterial->GetTexture(aiTextureType_NORMALS, 0, &aiString);
    if(ret == aiReturn_SUCCESS){
        material.normalMap = aiString.C_Str();
    }

    ret = aiMaterial->GetTexture(aiTextureType_AMBIENT_OCCLUSION, 0, &aiString);
    if(ret == aiReturn_SUCCESS){
        material.ambientOcclusionMap = aiString.C_Str();
    }
    mesh.material = material;

    return mesh;
}

int load(std::vector<mesh::Mesh>& meshes, std::string_view parent, const aiNode* node, const aiScene* scene){
    uint32_t numVertices = 0;
 //   std::vector<std::future<mesh::Mesh>> futures;
    for(auto i = 0; i < node->mNumMeshes; i++){
        meshes.push_back(loadMesh(parent, node, scene, &numVertices, i));
    }

    for(auto i = 0; i < node->mNumChildren; i++){
        numVertices += load(meshes, parent, node->mChildren[i], scene);
    }
    return numVertices;
}

int mesh::load(std::vector<Mesh> &meshes, std::string_view path, uint32_t flags) {

    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path.data(), flags);
    auto i = path.find_last_of("\\") + 1;
    auto parentPath = path.substr(0, i); // TODO use file path

    auto start = chrono::high_resolution_clock::now();
    auto res =  load(meshes, parentPath, scene->mRootNode, scene);
    auto end = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(end - start).count();
    spdlog::info("constructed mesh in {} ms", duration, meshes.size());

    return res;
}
