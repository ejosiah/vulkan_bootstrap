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

int load(std::vector<mesh::Mesh>& meshes, std::string_view parent, const aiNode* node, const aiScene* scene){
    uint32_t numVertices = 0;
    for(auto i = 0; i < node->mNumMeshes; i++){
        auto aiMesh = scene->mMeshes[node->mMeshes[i]];
        mesh::Mesh mesh;
        mesh.name = aiMesh->mName.C_Str();
        mesh.primitiveTopology = toVulkan(aiMesh->mPrimitiveTypes);
        numVertices = aiMesh->mNumVertices;
        for(auto j = 0u; j < numVertices; j++){
            Vertex vertex{};
            auto aiVec = aiMesh->mVertices[j];

            vertex.position = {aiVec.x, aiVec.y, aiVec.z, 1.0f};

            if(aiMesh->HasNormals()){
                auto aiNorm = aiMesh->mNormals[j];
                vertex.normal = {aiNorm.x, aiNorm.y, aiNorm.z};
            }

            if(aiMesh->HasTangentsAndBitangents()){
                auto aiTan = aiMesh->mTangents[j];
                auto aiBi = aiMesh->mBitangents[j];
//                vertex.tangents = {aiTan.x, aiTan.y, aiTan.z};
//                vertex.bitangents = {aiBi.x, aiBi.y, aiBi.z};
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

        // TODO load MATERIAL
    }

    for(auto i = 0; i < node->mNumChildren; i++){
        numVertices += load(meshes, parent, node->mChildren[i], scene);
    }
    return numVertices;
}

void mesh::load(std::vector<Mesh> &meshes, std::string_view path, uint32_t flags) {
    auto start = chrono::high_resolution_clock::now();
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path.data(), flags);
    auto i = path.find_last_of("\\") + 1;
    auto parentPath = path.substr(0, i); // TODO use file path
    auto numVertices = load(meshes, parentPath, scene->mRootNode, scene);

    auto end = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(end - start).count();
    spdlog::info("loaded {} vertices in {} milliseconds from {}", numVertices, duration, path);
}
