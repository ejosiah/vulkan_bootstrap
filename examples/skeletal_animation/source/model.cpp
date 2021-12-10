#include "model.hpp"
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <model.hpp>
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

void map(const aiNode* node, const aiScene* scene, mdl::Model& model, aiMesh* aiMesh, mdl::Mesh& mesh){
    mesh.name = aiMesh->mName.C_Str();
    mesh.primitiveTopology = toVulkan(aiMesh->mPrimitiveTypes);
    std::map<uint32_t, std::vector<std::tuple<uint32_t , float>>> vertexBoneWeightMapping;

    auto updateVertexBoneMapping = [&](uint32_t boneId, aiBone* bone) -> std::tuple<glm::vec3, glm::vec3> {
        glm::vec3 min{bone->mNumWeights > 0 ? std::numeric_limits<float>::max() : 0};
        glm::vec3 max{bone->mNumWeights > 0 ? std::numeric_limits<float>::min() : 0};
        for(auto i = 0; i < bone->mNumWeights; i++){
            auto weight = bone->mWeights[i];
            if(vertexBoneWeightMapping.find(weight.mVertexId) == vertexBoneWeightMapping.end()){
                vertexBoneWeightMapping[weight.mVertexId] = {};
            }
            auto vertex = to_vec3(aiMesh->mVertices[weight.mVertexId]);
            min = glm::min(min, vertex);
            max = glm::max(max, vertex);
            vertexBoneWeightMapping[weight.mVertexId].push_back(std::make_tuple(boneId, weight.mWeight));
        }

        return std::make_tuple(min, max);
    };

    auto loadBones = [&](){
        if(aiMesh->HasBones()){
            auto numBones = aiMesh->mNumBones;
            for(auto i = 0; i < numBones; i++){

                auto aiBone = aiMesh->mBones[i];
                auto boneName = std::string{aiBone->mName.C_Str()};

                if(model.bonesMapping.find(boneName) == model.bonesMapping.end()){
                    auto id = int(model.bones.size());
                    mdl::Bone bone{id, boneName, to_mat4(aiBone->mOffsetMatrix) };
                    model.bonesMapping[boneName] = id;
                    model.bones.push_back(bone);
                    auto bounds = updateVertexBoneMapping(id, aiBone);
                    model.boneBounds.push_back(bounds);
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
                mdl::VertexBoneInfo boneInfo{};
                for(auto [boneId, weight] : vertexBoneWeightMapping[vertexId]){
                    assert(slot < mdl::NUN_BONES_PER_VERTEX);
                    boneInfo.boneIds[slot] = boneId;
                    boneInfo.weights[slot] = weight;
                    slot++;
                }
                mesh.bones.push_back(boneInfo);
            }
        }

        if(aiMesh->HasFaces()){
            for(auto j = 0; j < aiMesh->mNumFaces; j++){
                auto face = aiMesh->mFaces[j];
                for(auto k = 0; k < face.mNumIndices; k++){
                    mesh.indices.push_back(face.mIndices[k]);
                }
            }
        }
    };

    loadBones();
    loadVertices();
}

void load(mdl::Model& model, std::vector<mdl::Mesh>& meshes, const aiNode* node, const aiScene* scene){

    for(auto meshId = 0; meshId < node->mNumMeshes; meshId++){
        mdl::Mesh mesh;
        auto aiMesh = scene->mMeshes[node->mMeshes[meshId]];
        map(node, scene, model, aiMesh, mesh);
        meshes.push_back(mesh);
    }

    for(auto i = 0; i < node->mNumChildren; i++){
        load(model, meshes, node->mChildren[i], scene);
    }
}

void connectBones(const aiScene* scene, mdl::Model& model){

    std::function<void(aiNode*)> walkBoneHierarchy = [&](aiNode* node){
        auto name = std::string{ node->mName.C_Str() };
        auto itr = model.bonesMapping.find(name);
        if(itr != model.bonesMapping.end()){
            auto& bone = model.bones[itr->second];
            bone.transform = to_mat4(node->mTransformation);

            for(auto childId = 0; childId < node->mNumChildren; childId++){
                auto child = node->mChildren[childId];
                auto cItr = model.bonesMapping.find(child->mName.C_Str());
                assert(cItr != model.bonesMapping.end());
                auto& childBone = model.bones[cItr->second];
                bone.children.push_back(childBone.id);
                childBone.parent = bone.id;
                walkBoneHierarchy(child);
            }
        }else{
            for(auto childId = 0; childId < node->mNumChildren; childId++){
                walkBoneHierarchy(node->mChildren[childId]);
            }
        }
    };

    std::function<void(std::vector<mdl::Bone>&, mdl::Bone&, int)> reorder = [&](std::vector<mdl::Bone>& bones, mdl::Bone& bone, int newId){
        int oldId = bone.id;
        bone.id = newId;
        for(auto cid : bone.children){
            bones[cid].parent = newId;
        }
        bones[newId].id = oldId;
        for(auto cid : bones[newId].children){
            bones[cid].parent = oldId;
        }
        int numChildren = int(bone.children.size());
        for(int i = newId + 1; i < numChildren; i++){
            int childId = newId * numChildren + i;
            reorder(bones, bones[bone.children[i]], childId);
            bone.children[i] = childId;
        }
        std::swap(bones[oldId], bones[newId]);
    };

    walkBoneHierarchy(scene->mRootNode);
    model.globalInverseTransform = glm::inverse(to_mat4(scene->mRootNode->mTransformation));

    int rootCount = 0;
    int unconnectedBones = 0;
    for(auto& bone : model.bones){
        if(bone.parent == mdl::NULL_BONE){
            model.rootBone = bone.id;
            rootCount++;
        }
    }
    assert(rootCount == 1); // there should only be one root bone
    assert(!model.bones[model.rootBone].children.empty());
//    reorder(model.bones, model.bones[model.rootBone], 0);
//    model.rootBone = 0;
}

std::shared_ptr<mdl::Model> mdl::load(const VulkanDevice& device, const std::string &path, uint32_t flags) {
    Assimp::Importer importer;
    const auto scene = importer.ReadFile(path.data(), flags);

    if(!scene) throw std::runtime_error{fmt::format("unable to load model {}", path)};

    auto model = std::make_shared<mdl::Model>();
    std::vector<mdl::Mesh> meshes;

    load(*model, meshes, scene->mRootNode, scene);
    connectBones(scene, *model);

    auto numMeshes = meshes.size();
    auto numBones = model->bones.size();
    auto numVertices = 0u;
    for(auto& mesh : meshes) numVertices += mesh.vertices.size();
    spdlog::info("model {}, num meshes: {}, num vertices: {}, num bones {}", path, numMeshes, numVertices, numBones);

    model->createBuffers(device, meshes);

    return model;
}

void mdl::Model::createBuffers(const VulkanDevice& device, const std::vector<Mesh>& meshes) {
    auto numIndices = 0u;
    auto numVertices = 0u;
    glm::vec3 min{MAX_FLOAT};
    glm::vec3 max{MIN_FLOAT};

    // get Drawable bounds
    for(auto& mesh : meshes){
        numIndices += mesh.indices.size();
        numVertices += mesh.vertices.size();

        for(const auto& vertex : mesh.vertices){
//            mesh.bounds.min = glm::min(glm::vec3(vertex.position), mesh.bounds.min);
//            mesh.bounds.max = glm::max(glm::vec3(vertex.position), mesh.bounds.max);
            bounds.min = glm::min(glm::vec3(vertex.position), bounds.min);
            bounds.max = glm::max(glm::vec3(vertex.position), bounds.max);
        }
    }

    // copy meshes into vertex/index buffers
    this->primitives.resize(meshes.size());
    uint32_t firstVertex = 0;
    uint32_t firstIndex = 0;
    auto sizeOfInt = sizeof(uint32_t);
    auto offset = 0;
    std::vector<char> indexBuffer(numIndices * sizeof(uint32_t));
    std::vector<char> vertexBuffer(numVertices * sizeof(Vertex));
    std::vector<glm::ivec4> offsetBuffer;

    uint32_t numPrimitives = 0;
    for(int i = 0; i < meshes.size(); i++){
        auto& mesh = meshes[i];
        auto numVertices = mesh.vertices.size();
        auto size = numVertices * sizeof(Vertex);
        void* dest = vertexBuffer.data() + firstVertex * sizeof(Vertex);
        std::memcpy(dest, mesh.vertices.data(), size);

        size = mesh.indices.size() * sizeof(mesh.indices[0]);
        dest = indexBuffer.data() + firstIndex * sizeof(mesh.indices[0]);
        std::memcpy(dest, mesh.indices.data(), size);

        auto primitive = vkn::Primitive::indexed(mesh.indices.size(), firstIndex, numVertices, firstVertex);
//        primitives[i].name = mesh.name;
        primitives[i].firstIndex = primitive.firstIndex;
        primitives[i].indexCount = primitive.indexCount;
        primitives[i].firstVertex = primitive.firstVertex;
        primitives[i].vertexCount = primitive.vertexCount;
        primitives[i].vertexOffset = primitive.vertexOffset;

        firstVertex += mesh.vertices.size();
        firstIndex += mesh.indices.size();
        numPrimitives += primitives[i].numTriangles();
    }
    buffers.vertices = device.createDeviceLocalBuffer(vertexBuffer.data(), numVertices * sizeof(Vertex), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    buffers.indices = device.createDeviceLocalBuffer(indexBuffer.data(), numIndices * sizeof(uint32_t), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
}

float mdl::Model::height() const {
    return (bounds.max - bounds.min).y;
}

uint32_t mdl::Model::numTriangles() const {
    auto count = 0u;
    for(auto& primitive : primitives){
        count += primitive.numTriangles();
    }
    return count;
}

void mdl::Model::render(VkCommandBuffer commandBuffer) const {
    auto numPrims = primitives.size();
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &buffers.vertices.buffer, &offset);
    vkCmdBindIndexBuffer(commandBuffer, buffers.indices, 0, VK_INDEX_TYPE_UINT32);
    for (auto i = 0; i < numPrims; i++) {
        primitives[i].drawIndexed(commandBuffer);
    }
}

glm::vec3 mdl::Model::diagonal() const {
    return bounds.max - bounds.min;
}
