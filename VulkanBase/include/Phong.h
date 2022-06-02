#pragma once
#include "common.h"
#include "VulkanDevice.h"
#include "VulkanModel.h"
#include "Texture.h"
#include "Mesh.h"

namespace phong{

    enum class LightType : int{
        POINT = 0,
        DIRECTIONAL = 1,
        SPOT = 2
    };

    struct LightSource{
        glm::vec3 position;
        glm::vec3 intensity;
        glm::vec3 spotDirection;
        float spotAngle;
        float spotExponent;
        float kc = 1;
        float ki = 0;
        float kq = 0;

        static constexpr VkPushConstantRange pushConstant(){
            return {VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(LightSource)};
        }
    };

    struct Material {
        std::string name;
        struct {
            std::unique_ptr<Texture> ambientMap;
            std::unique_ptr<Texture> diffuseMap;
            std::unique_ptr<Texture> specularMap;
            std::unique_ptr<Texture> normalMap;
            std::unique_ptr<Texture> ambientOcclusionMap;
        } textures;

        VulkanBuffer materialBuffer;
        VkDeviceSize materialOffset;

        VkDescriptorSet descriptorSet = VK_NULL_HANDLE;

        void init(const mesh::Mesh& mesh, VulkanDevice& device, const VulkanDescriptorPool& descriptorPool, const VulkanDescriptorSetLayout& descriptorSetLayout, VkBufferUsageFlags usageFlags = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    };

    struct Mesh : public vkn::Primitive{
        std::string name;
        Material material;
    };

    inline std::vector<VkDescriptorPoolSize> getPoolSizes(uint32_t numSets = 1){
        std::vector<VkDescriptorPoolSize> poolSizes(2);
        poolSizes[0].descriptorCount = 2 * numSets;
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

        poolSizes[1].descriptorCount = 5 * numSets;
        poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

        return poolSizes;
    }

    struct VulkanDrawableInfo{
        VkBufferUsageFlags vertexUsage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        VkBufferUsageFlags indexUsage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        VkBufferUsageFlags materialUsage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        VkBufferUsageFlags  materialIdUsage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        bool generateMaterialId = true;
    };

    /**
     * @brief loads a phong object
     * @tparam Drawable type with members
     *  VulkanBuffer* vertexBuffer,
     *  VulkanBuffer* indexBuffer,
     *  VulkanDescriptorSetLayout descriptorSetLayout;
     *  std::vector<phong::Mesh> meshes;
     * @param path
     * @param device
     * @param pool
     * @param drawable
     * @return
     */
    template<typename Drawable>
    inline void load(const std::string& path, VulkanDevice &device, const VulkanDescriptorPool& pool,
                     Drawable& drawable,
                     const VulkanDrawableInfo info = {}, bool normalize = false, float size = 1){
        std::array<VkDescriptorSetLayoutBinding, 6> bindings{};

        bindings[0].binding = 0;
        bindings[0].descriptorCount = 1;
        bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        for(auto i = 1; i < 6; i++){
            bindings[i].binding = i;
            bindings[i].descriptorCount = 1;
            bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            bindings[i].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        }
        drawable.descriptorSetLayout = device.createDescriptorSetLayout(bindings);

        std::vector<mesh::Mesh> meshes;
        mesh::load(meshes, path);

        if(normalize) {
            mesh::normalize(meshes, size);
        }

        int numIndices = 0;
        int numVertices = 0;
        glm::vec3 min{MAX_FLOAT};
        glm::vec3 max{MIN_FLOAT};

        // get Drawable bounds
        for(auto& mesh : meshes){
            numIndices += mesh.indices.size();
            numVertices += mesh.vertices.size();

            for(const auto& vertex : mesh.vertices){
                mesh.bounds.min = glm::min(glm::vec3(vertex.position), mesh.bounds.min);
                mesh.bounds.max = glm::max(glm::vec3(vertex.position), mesh.bounds.max);
                drawable.bounds.min = glm::min(glm::vec3(vertex.position), drawable.bounds.min);
                drawable.bounds.max = glm::max(glm::vec3(vertex.position), drawable.bounds.max);
            }
        }

        // copy meshes into vertex/index buffers
        drawable.meshes.resize(meshes.size());
        uint32_t firstVertex = 0;
        uint32_t firstIndex = 0;
        uint32_t materialOffset = 0;
        auto sizeOfInt = sizeof(uint32_t);
        auto offset = 0;
        std::vector<char> indexBuffer(numIndices * sizeof(uint32_t));
        std::vector<char> vertexBuffer(numVertices * sizeof(Vertex));
        std::vector<glm::ivec4> offsetBuffer;

        VkDeviceSize materialBufferSize = (sizeof(meshes[0].material) - offsetof(mesh::Material, diffuse));
        drawable.materialBuffer = device.createBuffer(
                                        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
                                        | VK_BUFFER_USAGE_TRANSFER_DST_BIT
                                        | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                        VMA_MEMORY_USAGE_GPU_ONLY,
                                        materialBufferSize * meshes.size());
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
            drawable.meshes[i].name = mesh.name;
            drawable.meshes[i].firstIndex = primitive.firstIndex;
            drawable.meshes[i].indexCount = primitive.indexCount;
            drawable.meshes[i].firstVertex = primitive.firstVertex;
            drawable.meshes[i].vertexCount = primitive.vertexCount;
            drawable.meshes[i].vertexOffset = primitive.vertexOffset;

            drawable.meshes[i].material.init(mesh, device, pool, drawable.descriptorSetLayout, info.materialUsage);
            device.copy(drawable.meshes[i].material.materialBuffer, drawable.materialBuffer, materialBufferSize, 0, i * materialBufferSize);

//            std::memcpy(offsetBuffer.data() + offset, &firstIndex, sizeOfInt);
//            offset += sizeOfInt;
//            std::memcpy(offsetBuffer.data() + offset, &primitive.vertexOffset, sizeOfInt);
//            offset += sizeOfInt;
            offsetBuffer.emplace_back(firstIndex, primitive.vertexOffset, materialOffset, 0);

            firstVertex += mesh.vertices.size();
            firstIndex += mesh.indices.size();
            materialOffset += mesh.indices.size()/3;
            numPrimitives += drawable.meshes[i].numTriangles();
        }

        if(info.generateMaterialId){
            std::vector<int> materialIds;
            materialIds.reserve(numPrimitives);
            int materialId = 0;
            for(phong::Mesh& mesh : drawable.meshes){
                for(int i = 0; i < mesh.numTriangles(); i++){
                    materialIds.push_back(materialId);
                }
                materialId++;
            }
            drawable.materialIdBuffer = device.createDeviceLocalBuffer(materialIds.data(), numPrimitives * sizeof(int), info.materialIdUsage);
        }

        drawable.offsetBuffer = device.createDeviceLocalBuffer(offsetBuffer.data(), offsetBuffer.size() * sizeof(glm::ivec4), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
        drawable.vertexBuffer = device.createDeviceLocalBuffer(vertexBuffer.data(), numVertices * sizeof(Vertex), info.vertexUsage);
        drawable.indexBuffer = device.createDeviceLocalBuffer(indexBuffer.data(), numIndices * sizeof(uint32_t), info.indexUsage);
        spdlog::info("{} vertices loaded", numVertices);
    }

}