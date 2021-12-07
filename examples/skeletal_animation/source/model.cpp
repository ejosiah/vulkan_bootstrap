#include "model.hpp"
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include "glm_assimp_interpreter.hpp"

VkPrimitiveTopology toVulkan(uint32_t MeshType){
    switch (MeshType) {
        case aiPrimitiveType_POINT: return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
        case aiPrimitiveType_LINE: return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
        case aiPrimitiveType_TRIANGLE: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        default:
            throw std::runtime_error{"unknown primitive type"};
    }
}

void map(const aiNode* node, const aiScene* scene, model::Model& model, aiMesh* aiMesh, model::Mesh& mesh){
    mesh.name = aiMesh->mName.C_Str();
    mesh.primitiveTopology = toVulkan(aiMesh->mPrimitiveTypes);
    std::map<uint32_t, std::vector<std::tuple<uint32_t , float>>> vertexBoneWeightMapping;

    auto updateVertexBoneMapping = [&](uint32_t boneId, aiBone* bone){
        for(auto i = 0; i < bone->mNumWeights; i++){
            auto weight = bone->mWeights[i];
            if(vertexBoneWeightMapping.find(weight.mVertexId) == vertexBoneWeightMapping.end()){
                vertexBoneWeightMapping[weight.mVertexId] = {};
            }
            vertexBoneWeightMapping[weight.mVertexId].push_back(std::make_tuple(boneId, weight.mWeight));
        }
    };

    auto loadBones = [&](){
        if(aiMesh->HasBones()){
            auto numBones = aiMesh->mNumBones;
            for(auto i = 0; i < numBones; i++){

                auto aiBone = aiMesh->mBones[i];
                auto boneName = std::string{aiBone->mName.C_Str()};

                if(model.bonesMapping.find(boneName) == model.bonesMapping.end()){
                    auto id = uint32_t(model.bones.size());
                    model::Bone bone{ id,  boneName, to_mat4(aiBone->mOffsetMatrix) };
                    model.bonesMapping[boneName] = id;
                    model.bones.push_back(bone);
                    updateVertexBoneMapping(id, aiBone);
                }
            }
        }
    };

    auto loadVertices = [&](){
        for(auto vertexId = 0u; vertexId < aiMesh->mNumVertices; vertexId++){
            Vertex vertex{};
            auto aiVec = aiMesh->mVertices[vertexId];


            vertex.position = {aiVec.x, aiVec.y, aiVec.z, 1.0f};
            mesh.bounds.min = glm::min(mesh.bounds.min, glm::vec3(vertex.position));
            mesh.bounds.max = glm::max(mesh.bounds.max, glm::vec3(vertex.position));

            if(aiMesh->HasNormals()){
                auto aiNorm = aiMesh->mNormals[vertexId];
                vertex.normal = {aiNorm.x, aiNorm.y, aiNorm.z};
            }

            if(aiMesh->HasTangentsAndBitangents()){
                auto aiTan = aiMesh->mTangents[vertexId];
                auto aiBi = aiMesh->mBitangents[vertexId];
                vertex.tangent = {aiTan.x, aiTan.y, aiTan.z};
                vertex.bitangent = {aiBi.x, aiBi.y, aiBi.z};
            }

            if(aiMesh->HasVertexColors(0)){
                auto aiCol = aiMesh->mColors[vertexId][0];
                vertex.color = {aiCol.r, aiCol.g, aiCol.b, aiCol.a};
            }

            if(aiMesh->HasTextureCoords((0))){  // TODO retrieve up to AI_MAX_NUMBER_OF_TEXTURECOORDS textures coordinates
                auto aiTex = aiMesh->mTextureCoords[0][vertexId];
                vertex.uv = {aiTex.x, aiTex.y};
            }
            mesh.vertices.push_back(vertex);


            if(vertexBoneWeightMapping.find(vertexId) != vertexBoneWeightMapping.end()){
                int slot = 0;
                model::VertexBoneInfo boneInfo{};
                for(auto [boneId, weight] : vertexBoneWeightMapping[vertexId]){
                    assert(slot < model::NUN_BONES_PER_VERTEX);
                    boneInfo.boneIds[slot] = boneId;
                    boneInfo.weights[slot] = weight;
                    slot++;
                }
                mesh.bones.push_back(boneInfo);
            }
        }
    };

    loadBones();
    loadVertices();
}

void load(model::Model& model, std::vector<model::Mesh>& meshes, const aiNode* node, const aiScene* scene){

    for(auto meshId = 0; meshId < node->mNumMeshes; meshId++){
        model::Mesh mesh;
        auto aiMesh = scene->mMeshes[node->mMeshes[meshId]];
        map(node, scene, model, aiMesh, mesh);
        meshes.push_back(mesh);
    }

    for(auto i = 0; i < node->mNumChildren; i++){
        load(model, meshes, node->mChildren[i], scene);
    }
}

std::shared_ptr<model::Model> model::load(const std::string &path, uint32_t flags) {
    Assimp::Importer importer;
    const auto scene = importer.ReadFile(path.data(), flags);

    if(!scene) throw std::runtime_error{fmt::format("unable to load model {}", path)};

    auto model = std::make_shared<model::Model>();
    std::vector<model::Mesh> meshes;
    load(*model, meshes, scene->mRootNode, scene);

    auto numMeshes = meshes.size();
    auto numBones = model->bones.size();
    auto numVertices = 0u;
    for(auto& mesh : meshes) numVertices += mesh.vertices.size();
    spdlog::info("model {}, num meshes: {}, num vertices: {}, num bones {}", numMeshes, numVertices, numBones);

    return model;
}
