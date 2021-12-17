#include "model.hpp"
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <model.hpp>
#include "glm_assimp_interpreter.hpp"
#include "DescriptorSetBuilder.hpp"
#include "VulkanInitializers.h"
#include "Texture.h"
#include  <stb_image.h>
#define AI_MATKEY_COLOR_NULL nullptr,0,0


namespace mdl {

    VkPrimitiveTopology toVulkan(uint32_t MeshType) {
        switch (MeshType) {
            case aiPrimitiveType_POINT:
                return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
            case aiPrimitiveType_LINE:
                return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
            case aiPrimitiveType_TRIANGLE:
                return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            default:
                throw std::runtime_error{"unknown primitive type"};
        }
    }

    using VertexBoneMapping = std::map<uint32_t, std::vector<std::tuple<uint32_t, float>>>;

    void loadTexture(const aiScene* scene, aiMaterial* aiMaterial, MeshTexture& texture, aiTextureType textureType
                     , const char* pKey,unsigned int type, unsigned int idx, glm::vec3 defaultColor = glm::vec3{1}){

        aiString path;
        auto status = aiMaterial->GetTexture(textureType, 0, &path);
        if(status == aiReturn_SUCCESS){
            stbi_set_flip_vertically_on_load(1);
            int width, height;
            int numChannels = STBI_rgb_alpha;
            uint8_t* data = nullptr;
            if(auto aiTexture = scene->GetEmbeddedTexture(path.C_Str())){
                data = stbi_load_from_memory(reinterpret_cast<stbi_uc const *>(aiTexture->pcData), aiTexture->mWidth
                        , &width, &height, &numChannels, numChannels);
            }else{
                data = stbi_load(path.C_Str(), &width, &height, &numChannels, numChannels);
            }
            texture.data = data;
            texture.width = width;
            texture.height = height;
        }else{
            uint32_t size = 3;
            glm::vec3 color = defaultColor;
            if(pKey) {
                aiMaterial->Get(pKey, type, idx, &color, &size);
            }

            texture.data = new uint8_t[256u * 256u * 4];
            texture.width = texture.height = 256u;
            textures::color(texture.data, {texture.width, texture.height}, color);
        }
    }

    void loadMaterial(const aiScene* scene, const aiMesh* aiMesh, Mesh& mesh){
        const auto aiMaterial = scene->mMaterials[aiMesh->mMaterialIndex];
        aiString name;
        auto status = aiMaterial->Get(AI_MATKEY_NAME, name);
        if(status == aiReturn_SUCCESS){
            mesh.material.name = name.C_Str();
        }
        loadTexture(scene, aiMaterial, mesh.material.diffuse, aiTextureType_DIFFUSE, AI_MATKEY_COLOR_DIFFUSE, glm::vec3(0.6));

        aiString path;
        if(aiMaterial->GetTexture(aiTextureType_AMBIENT, 0, &path) == aiReturn_SUCCESS) {
            loadTexture(scene, aiMaterial, mesh.material.ambient, aiTextureType_AMBIENT, AI_MATKEY_COLOR_AMBIENT, glm::vec3(0.6));
        }else{
            loadTexture(scene, aiMaterial, mesh.material.ambient, aiTextureType_DIFFUSE, AI_MATKEY_COLOR_DIFFUSE, glm::vec3(0.6));
        }
        loadTexture(scene, aiMaterial, mesh.material.specular, aiTextureType_SPECULAR, AI_MATKEY_COLOR_SPECULAR);

        loadTexture(scene, aiMaterial, mesh.material.normal, aiTextureType_NORMALS, AI_MATKEY_COLOR_NULL, {0.5, 0.5, 1.0});

    }

    void loadBones(Model& model, aiMesh* aiMesh, VertexBoneMapping& vertexBoneWeightMapping){
        auto updateVertexBoneMapping = [&](uint32_t boneId, aiBone *bone) -> std::tuple<glm::vec3, glm::vec3> {
            glm::vec3 min{bone->mNumWeights > 0 ? std::numeric_limits<float>::max() : 0};
            glm::vec3 max{bone->mNumWeights > 0 ? std::numeric_limits<float>::min() : 0};
            for (auto i = 0; i < bone->mNumWeights; i++) {
                auto weight = bone->mWeights[i];
                if (vertexBoneWeightMapping.find(weight.mVertexId) == vertexBoneWeightMapping.end()) {
                    vertexBoneWeightMapping[weight.mVertexId] = {};
                }
                auto vertex = to_vec3(aiMesh->mVertices[weight.mVertexId]);
                min = glm::min(min, vertex);
                max = glm::max(max, vertex);
                vertexBoneWeightMapping[weight.mVertexId].push_back(std::make_tuple(boneId, weight.mWeight));
            }

            return std::make_tuple(min, max);
        };

        if (aiMesh->HasBones()) {
            auto numBones = aiMesh->mNumBones;
            for (auto i = 0; i < numBones; i++) {

                auto aiBone = aiMesh->mBones[i];
                auto boneName = std::string{aiBone->mName.C_Str()};

                if (model.bonesMapping.find(boneName) == model.bonesMapping.end()) {
                    auto id = int(model.bones.size());
                    Bone bone{id, boneName, to_mat4(aiBone->mOffsetMatrix)};
                    model.bonesMapping[boneName] = id;
                    model.bones.push_back(bone);
                    auto bounds = updateVertexBoneMapping(id, aiBone);
                    model.boneBounds.push_back(bounds);
                }
            }
        }
    }

    void loadVertices(Mesh& mesh, const aiMesh* aiMesh, const VertexBoneMapping& vertexBoneWeightMapping){
        for (auto vertexId = 0u; vertexId < aiMesh->mNumVertices; vertexId++) {
            Vertex vertex{};
            auto aiVec = aiMesh->mVertices[vertexId];


            vertex.position = {aiVec.x, aiVec.y, aiVec.z, 1.0f};
            mesh.bounds.min = glm::min(mesh.bounds.min, glm::vec3(vertex.position));
            mesh.bounds.max = glm::max(mesh.bounds.max, glm::vec3(vertex.position));

            if (aiMesh->HasNormals()) {
                auto aiNorm = aiMesh->mNormals[vertexId];
                vertex.normal = {aiNorm.x, aiNorm.y, aiNorm.z};
            }

            if (aiMesh->HasTangentsAndBitangents()) {
                auto aiTan = aiMesh->mTangents[vertexId];
                auto aiBi = aiMesh->mBitangents[vertexId];
                vertex.tangent = {aiTan.x, aiTan.y, aiTan.z};
                vertex.bitangent = {aiBi.x, aiBi.y, aiBi.z};
            }

            if (aiMesh->HasVertexColors(0)) {
                auto aiCol = aiMesh->mColors[vertexId][0];
                vertex.color = {aiCol.r, aiCol.g, aiCol.b, aiCol.a};
            }

            if (aiMesh->HasTextureCoords(
                    (0))) {  // TODO retrieve up to AI_MAX_NUMBER_OF_TEXTURECOORDS textures coordinates
                auto aiTex = aiMesh->mTextureCoords[0][vertexId];
                vertex.uv = {aiTex.x, aiTex.y};
            }
            mesh.vertices.push_back(vertex);

            VertexBoneInfo boneInfo{};
            boneInfo.weights[0] = 1.0f;
            if (vertexBoneWeightMapping.find(vertexId) != vertexBoneWeightMapping.end()) {
                int slot = 0;
                for (auto [boneId, weight] : vertexBoneWeightMapping.at(vertexId)) {
                    if (slot >= NUN_BONES_PER_VERTEX)
                        break;
                    boneInfo.boneIds[slot] = boneId;
                    boneInfo.weights[slot] = weight;
                    slot++;
                }
            }
            mesh.bones.push_back(boneInfo);
        }

        if (aiMesh->HasFaces()) {
            for (auto j = 0; j < aiMesh->mNumFaces; j++) {
                auto face = aiMesh->mFaces[j];
                for (auto k = 0; k < face.mNumIndices; k++) {
                    mesh.indices.push_back(face.mIndices[k]);
                }
            }
        }
    }


    void map(const aiNode *node, const aiScene *scene, Model &model, aiMesh *aiMesh, Mesh &mesh) {
        mesh.name = aiMesh->mName.C_Str();
        mesh.primitiveTopology = toVulkan(aiMesh->mPrimitiveTypes);
        std::map<uint32_t, std::vector<std::tuple<uint32_t, float>>> vertexBoneWeightMapping;


        loadBones(model, aiMesh, vertexBoneWeightMapping);
        loadVertices(mesh, aiMesh, vertexBoneWeightMapping);
        loadMaterial(scene, aiMesh, mesh);
    }

    void load(Model &model, std::vector<Mesh> &meshes, const aiNode *node, const aiScene *scene) {

        for (auto meshId = 0; meshId < node->mNumMeshes; meshId++) {
            Mesh mesh;
            auto aiMesh = scene->mMeshes[node->mMeshes[meshId]];
            map(node, scene, model, aiMesh, mesh);
            meshes.push_back(mesh);
        }

        for (auto i = 0; i < node->mNumChildren; i++) {
            load(model, meshes, node->mChildren[i], scene);
        }
    }

    void connectBones(const aiScene *scene, Model &model) {

        std::function<void(aiNode *)> walkBoneHierarchy = [&](aiNode *node) {
            auto name = std::string{node->mName.C_Str()};
            auto itr = model.bonesMapping.find(name);
            if (itr != model.bonesMapping.end()) {
                auto &bone = model.bones[itr->second];
                bone.transform = to_mat4(node->mTransformation);

                for (auto childId = 0; childId < node->mNumChildren; childId++) {
                    auto child = node->mChildren[childId];
                    auto cItr = model.bonesMapping.find(child->mName.C_Str());
                    assert(cItr != model.bonesMapping.end());
                    auto &childBone = model.bones[cItr->second];
                    bone.children.push_back(childBone.id);
                    childBone.parent = bone.id;
                    walkBoneHierarchy(child);
                }
            } else {
                for (auto childId = 0; childId < node->mNumChildren; childId++) {
                    walkBoneHierarchy(node->mChildren[childId]);
                }
            }
        };

        std::function<void(std::vector<Bone> &, Bone &, int)> reorder = [&](std::vector<Bone> &bones,
                                                                                      Bone &bone, int newId) {
            int oldId = bone.id;
            bone.id = newId;
            for (auto cid : bone.children) {
                bones[cid].parent = newId;
            }
            bones[newId].id = oldId;
            for (auto cid : bones[newId].children) {
                bones[cid].parent = oldId;
            }
            int numChildren = int(bone.children.size());
            for (int i = newId + 1; i < numChildren; i++) {
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
        for (auto &bone : model.bones) {
            if (bone.parent == NULL_BONE) {
                model.rootBone = bone.id;
                rootCount++;
            }
        }
        assert(rootCount == 1); // there should only be one root bone
        assert(!model.bones[model.rootBone].children.empty());
//    reorder(model.bones, model.bones[model.rootBone], 0);
//    model.rootBone = 0;
    }

    std::shared_ptr<Model> load(const VulkanDevice &device, const std::string &path, uint32_t flags) {
        Assimp::Importer importer;
        const auto scene = importer.ReadFile(path.data(), flags);

        if (!scene) throw std::runtime_error{fmt::format("unable to load model {}", path)};

        auto model = std::make_shared<Model>();
        std::vector<Mesh> meshes;

        load(*model, meshes, scene->mRootNode, scene);
//    connectBones(scene, *model);

        auto numMeshes = meshes.size();
        auto numBones = model->bones.size();
        auto numVertices = 0u;
        for (auto &mesh : meshes) numVertices += mesh.vertices.size();
        spdlog::info("model {}, num meshes: {}, num vertices: {}, num bones {}", path, numMeshes, numVertices,
                     numBones);

        model->createBuffers(device, meshes);

        return model;
    }

    void Model::createBuffers(const VulkanDevice &device, std::vector<Mesh> &meshes) {
        auto numIndices = 0u;
        auto numVertices = 0u;
        glm::vec3 min{MAX_FLOAT};
        glm::vec3 max{MIN_FLOAT};

        // get Drawable bounds
        for (auto &mesh : meshes) {
            numIndices += mesh.indices.size();
            numVertices += mesh.vertices.size();

            if (mesh.bones.empty()) {
                mesh.bones.resize(numVertices);
                VertexBoneInfo boneInfo{};
                boneInfo.weights[0] = 1;
                std::fill(begin(mesh.bones), end(mesh.bones), boneInfo);
            }

            for (const auto &vertex : mesh.vertices) {
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
        std::vector<char> vertexBoneBuffer(numVertices * sizeof(VertexBoneInfo));
        std::vector<glm::ivec4> offsetBuffer;


        uint32_t numPrimitives = 0;
        for (int i = 0; i < meshes.size(); i++) {
            auto &mesh = meshes[i];
            auto numVertices = mesh.vertices.size();
            auto size = numVertices * sizeof(Vertex);
            void *dest = vertexBuffer.data() + firstVertex * sizeof(Vertex);
            std::memcpy(dest, mesh.vertices.data(), size);

            size = numVertices * sizeof(VertexBoneInfo);
            dest = vertexBoneBuffer.data() + firstVertex * sizeof(VertexBoneInfo);
            std::memcpy(dest, mesh.bones.data(), size);

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
        buffers.vertices = device.createDeviceLocalBuffer(vertexBuffer.data(), BYTE_SIZE(vertexBuffer),
                                                          VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
        buffers.indices = device.createDeviceLocalBuffer(indexBuffer.data(), BYTE_SIZE(indexBuffer),
                                                         VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
        buffers.vertexBones = device.createDeviceLocalBuffer(vertexBoneBuffer.data(), BYTE_SIZE(vertexBoneBuffer),
                                                             VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);

        std::vector<glm::mat4> bonesXforms(numVertices, glm::mat4{1});
        buffers.boneTransforms = device.createCpuVisibleBuffer(bonesXforms.data(), BYTE_SIZE(bonesXforms),
                                                               VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);

        createImageBuffers(device, meshes);
        createDescriptorSetLayout(device);
    }

    float Model::height() const {
        return (bounds.max - bounds.min).y;
    }

    uint32_t Model::numTriangles() const {
        auto count = 0u;
        for (auto &primitive : primitives) {
            count += primitive.numTriangles();
        }
        return count;
    }

    void Model::render(VkCommandBuffer commandBuffer) const {
        auto numPrims = primitives.size();
        static std::array<VkBuffer, 2> bindBuffers;
        bindBuffers[0] = buffers.vertices.buffer;
        bindBuffers[1] = buffers.vertexBones.buffer;
        static std::array<VkDeviceSize, 2> offsets{0, 0};

        vkCmdBindVertexBuffers(commandBuffer, 0, 2, bindBuffers.data(), offsets.data());
        vkCmdBindIndexBuffer(commandBuffer, buffers.indices, 0, VK_INDEX_TYPE_UINT32);
        for (auto i = 0; i < numPrims; i++) {
            primitives[i].drawIndexed(commandBuffer);
        }
    }

    glm::vec3 Model::diagonal() const {
        return bounds.max - bounds.min;
    }

    void Model::createDescriptorSetLayout(const VulkanDevice &device) {
        descriptor.boneLayout =
            device.descriptorSetLayoutBuilder()
                .binding(kLayoutBinding_BONE)
                    .descriptorType(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
                    .descriptorCount(1)
                    .shaderStages(VK_SHADER_STAGE_VERTEX_BIT)
                .createLayout();

        descriptor.materialLayout =
            device.descriptorSetLayoutBuilder()
                .binding(kLayoutBinding_MATERIAL_DIFFIUSE)
                    .descriptorType(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
                    .descriptorCount(1)
                    .shaderStages(VK_SHADER_STAGE_FRAGMENT_BIT)
                .binding(kLayoutBinding_MATERIAL_AMBIENT)
                    .descriptorType(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
                    .descriptorCount(1)
                    .shaderStages(VK_SHADER_STAGE_FRAGMENT_BIT)
                .binding(kLayoutBinding_MATERIAL_SPECULAR)
                    .descriptorType(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
                    .descriptorCount(1)
                    .shaderStages(VK_SHADER_STAGE_FRAGMENT_BIT)
                .binding(kLayoutBinding_MATERIAL_NORMAL)
                    .descriptorType(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
                    .descriptorCount(1)
                    .shaderStages(VK_SHADER_STAGE_FRAGMENT_BIT)
                .createLayout();
    }

    void Model::updateDescriptorSet(const VulkanDevice &device, const VulkanDescriptorPool &descriptorPool) {
        int numSets = int(primitives.size());
        std::vector<VkDescriptorSetLayout> setLayouts(numSets + 1); // primitive materials + 1 bone set
        setLayouts[kSet_BONE] = descriptor.boneLayout;
        
        auto first = begin(setLayouts);
        std::advance(first, 1);
        auto last = end(setLayouts);
        std::fill(first, last, descriptor.materialLayout);
        
        
        descriptor.sets = descriptorPool.allocate( setLayouts );

        device.setName<VK_OBJECT_TYPE_DESCRIPTOR_SET>("bones", descriptor.sets[0]);

        std::vector<VkWriteDescriptorSet> writes(numSets * kSetBinding_SIZE +  1, { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET }); // primitive materials + 1 bone set
        
        writes[kSet_BONE].dstSet = descriptor.sets[kSet_BONE];
        writes[kSet_BONE].dstBinding = kLayoutBinding_BONE;
        writes[kSet_BONE].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes[kSet_BONE].descriptorCount = 1;
        VkDescriptorBufferInfo boneBufferInfo{buffers.boneTransforms, 0, VK_WHOLE_SIZE};
        writes[kSet_BONE].pBufferInfo = &boneBufferInfo;
        std::vector<VkDescriptorImageInfo> imageInfos(numSets * kSetBinding_SIZE + 1);
        for(auto i = 0; i < numSets; i++) {
            auto& material = materials[i];
            auto set = i * kSetBinding_SIZE + 1;
            device.setName<VK_OBJECT_TYPE_DESCRIPTOR_SET>(fmt::format("{}_{}", material.name, set), descriptor.sets[set]);

            writes[set + kSetBinding_DIFFUSE].dstSet = descriptor.sets[set];
            writes[set + kSetBinding_DIFFUSE].dstBinding = kSetBinding_DIFFUSE;
            writes[set + kSetBinding_DIFFUSE].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            writes[set + kSetBinding_DIFFUSE].descriptorCount = 1;

            imageInfos[set + kSetBinding_DIFFUSE] = { material.diffuse.sampler, material.diffuse.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
            writes[set + kSetBinding_DIFFUSE].pImageInfo = &imageInfos[set + kSetBinding_DIFFUSE];

            writes[set + kSetBinding_AMBIENT].dstSet = descriptor.sets[set];
            writes[set + kSetBinding_AMBIENT].dstBinding = kSetBinding_AMBIENT;
            writes[set + kSetBinding_AMBIENT].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            writes[set + kSetBinding_AMBIENT].descriptorCount = 1;

            imageInfos[set + kSetBinding_AMBIENT] = { material.ambient.sampler, material.ambient.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
            writes[set + kSetBinding_AMBIENT].pImageInfo = &imageInfos[set + kSetBinding_AMBIENT];

            writes[set + kSetBinding_SPECULAR].dstSet = descriptor.sets[set];
            writes[set + kSetBinding_SPECULAR].dstBinding = kSetBinding_SPECULAR;
            writes[set + kSetBinding_SPECULAR].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            writes[set + kSetBinding_SPECULAR].descriptorCount = 1;

            imageInfos[set + kSetBinding_SPECULAR] = { material.specular.sampler, material.specular.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
            writes[set + kSetBinding_SPECULAR].pImageInfo = &imageInfos[set + kSetBinding_SPECULAR];

            writes[set + kSetBinding_NORMAL].dstSet = descriptor.sets[set];
            writes[set + kSetBinding_NORMAL].dstBinding = kSetBinding_NORMAL;
            writes[set + kSetBinding_NORMAL].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            writes[set + kSetBinding_NORMAL].descriptorCount = 1;

            imageInfos[set + kSetBinding_NORMAL] = { material.normal.sampler, material.normal.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
            writes[set + kSetBinding_NORMAL].pImageInfo = &imageInfos[set + kSetBinding_NORMAL];

        }
        device.updateDescriptorSets(writes);


    }

    void Model::createImageBuffers(const VulkanDevice &device, const std::vector<Mesh> &meshes) {
        for(const auto& mesh : meshes){

            const auto& meshMaterial = mesh.material;
            const auto& diffuse = meshMaterial.diffuse;
            Texture texture;
            textures::create(device, texture, VK_IMAGE_TYPE_2D, meshMaterial.diffuse.format
                             , meshMaterial.diffuse.data, {meshMaterial.diffuse.width, meshMaterial.diffuse.height, 1 });

            Material material{ meshMaterial.name };
            material.diffuse.image = std::move(texture.image);
            material.diffuse.imageView = std::move(texture.imageView);
            material.diffuse.sampler = std::move(texture.sampler);

            textures::create(device, texture, VK_IMAGE_TYPE_2D, meshMaterial.ambient.format, meshMaterial.ambient.data
                             , {meshMaterial.ambient.width, meshMaterial.ambient.height, 1 });
            material.ambient.image = std::move(texture.image);
            material.ambient.imageView = std::move(texture.imageView);
            material.ambient.sampler = std::move(texture.sampler);

            textures::create(device, texture, VK_IMAGE_TYPE_2D, meshMaterial.specular.format, meshMaterial.specular.data
                             , {meshMaterial.specular.width, meshMaterial.specular.height, 1 });
            material.specular.image = std::move(texture.image);
            material.specular.imageView = std::move(texture.imageView);
            material.specular.sampler = std::move(texture.sampler);

            textures::create(device, texture, VK_IMAGE_TYPE_2D, meshMaterial.normal.format, meshMaterial.normal.data
                            , {meshMaterial.normal.width, meshMaterial.normal.height, 1});
            material.normal.image = std::move(texture.image);
            material.normal.imageView = std::move(texture.imageView);
            material.normal.sampler = std::move(texture.sampler);

            materials.push_back(std::move(material));
        }
    }


}