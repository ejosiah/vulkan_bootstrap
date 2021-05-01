#pragma once

#include "common.h"
#include "VulkanBuffer.h"
#include "VulkanDescriptorSet.h"
#include "Phong.h"

struct VulkanDrawable{
    VulkanBuffer vertexBuffer;
    VulkanBuffer indexBuffer;
    VulkanBuffer materialBuffer;
    std::vector<phong::Mesh> meshes;
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
};