#pragma once

#include "common.h"
#include "VulkanBuffer.h"
#include "VulkanDescriptorSet.h"
#include "Phong.h"


struct VulkanDrawable{
    std::vector<phong::Mesh> meshes;

    VulkanBuffer vertexBuffer;
    VulkanBuffer indexBuffer;
    VulkanBuffer offsetBuffer;
    VulkanBuffer materialIdBuffer;
    VulkanDescriptorSetLayout descriptorSetLayout;

    struct {
        glm::vec3 min{MAX_FLOAT};
        glm::vec3 max{MIN_FLOAT};
    } bounds;

    [[nodiscard]]
    float height() const {
        auto diagonal = bounds.max - bounds.min;
        return std::abs(diagonal.y);
    }

    void draw(VkCommandBuffer commandBuffer, VulkanPipelineLayout& layout) const {
        int numPrims = meshes.size();
        VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer.buffer, &offset);
        vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
        for (auto i = 0; i < numPrims; i++) {
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &meshes[i].material.descriptorSet, 0, VK_NULL_HANDLE);
            meshes[i].drawIndexed(commandBuffer);

        }
    }

    static VulkanDrawable flatten(VulkanDevice& device, const VulkanDescriptorPool& descriptorPool,
                                  const VulkanDescriptorSetLayout& descriptorSetLayout,
                                  VulkanDrawable&& drawable, uint32_t materialBinding,
                                  uint32_t materialIdBinding, VkBufferUsageFlagBits materialUsage) {
        VulkanDrawable result;
        result.indexBuffer = std::move(drawable.indexBuffer);
        result.vertexBuffer = std::move(drawable.vertexBuffer);
        result.bounds.min = drawable.bounds.min;
        result.bounds.max = drawable.bounds.max;

        result.meshes.resize(1);
        auto& meshes = drawable.meshes;
        auto& mesh = result.meshes.front();
        mesh.name = meshes.front().name;
        mesh.firstIndex = meshes.front().firstIndex;
        mesh.firstVertex = meshes.front().firstVertex;
        mesh.vertexOffset = meshes.front().vertexOffset;


        int materialId = 0;
        VkDeviceSize materialSize = 0;
        std::vector<int> materialIds;
        for(auto& pMesh : meshes){  // collect sizing data, and materialId
            mesh.indexCount += pMesh.indexCount;
            mesh.vertexCount += pMesh.vertexCount;
            materialSize += pMesh.material.materialBuffer.size;
            for(auto i = 0; i < pMesh.numTriangles(); i++){
                materialIds.push_back(materialId);
            }
            materialId++;
        }

        // flatten material buffer;
        mesh.material.materialBuffer = device.createBuffer(materialUsage | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY, materialSize);
        VkDeviceSize dstOffset = 0;
        for(auto& pMesh : meshes){
            device.copy(pMesh.material.materialBuffer, mesh.material.materialBuffer, pMesh.material.materialBuffer.size, 0, dstOffset);
            dstOffset += pMesh.material.materialBuffer.size;
        }

        result.materialIdBuffer = device.createDeviceLocalBuffer(materialIds.data(), sizeof(int) * materialIds.size(), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);

        // create descriptorSet
        auto& descriptorSet = mesh.material.descriptorSet;
        descriptorSet = descriptorPool.allocate({descriptorSetLayout}).front();

        std::vector<VkWriteDescriptorSet> writes;
        VkDescriptorBufferInfo matIdInfo{};
        matIdInfo.buffer = result.materialIdBuffer;
        matIdInfo.offset = 0;
        matIdInfo.range = VK_WHOLE_SIZE;

        auto write = initializers::writeDescriptorSet(descriptorSet);
        write.dstBinding = materialIdBinding;
        write.descriptorCount = 1;
        write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        write.pBufferInfo = &matIdInfo;
        writes.push_back(write);

        VkDescriptorBufferInfo materialInfo{};
        materialInfo.buffer = mesh.material.materialBuffer;
        materialInfo.offset = 0;
        materialInfo.range = VK_WHOLE_SIZE;

        write.dstBinding = materialBinding;
        write.descriptorCount = 1;
        write.descriptorType = (materialUsage & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT) ? VK_DESCRIPTOR_TYPE_STORAGE_BUFFER : VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        write.pBufferInfo = &materialInfo;
        writes.push_back(write);
        device.updateDescriptorSets(writes);

        // TODO flatten offset buffer

        // TODO flatten texture

        return result;
    }

    static std::vector<VulkanDrawable> split(VulkanDrawable& drawable){
        return {};
    }

    [[nodiscard]]
    uint32_t numVertices() const {
        return vertexBuffer.size / sizeof(Vertex);
    }
};


struct VulkanDrawableInstance{
    VulkanDrawable* drawable{ nullptr };
    glm::mat4 xform{ 1 };
    glm::mat4 xformIT{ 1 };
};