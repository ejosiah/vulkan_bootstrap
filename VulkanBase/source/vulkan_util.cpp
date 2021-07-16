#include "vulkan_util.h"

void addComputeBufferMemoryBarriers(VkCommandBuffer commandBuffer, const std::vector<VulkanBuffer*>& buffers){
    addBufferMemoryBarriers(commandBuffer, buffers, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
}

void addBufferMemoryBarriers(VkCommandBuffer commandBuffer, const std::vector<VulkanBuffer*> &buffers, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask) {
    std::vector<VkBufferMemoryBarrier> barriers(buffers.size());

    for (int i = 0; i < buffers.size(); i++) {
        barriers[i].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        barriers[i].srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        barriers[i].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        barriers[i].offset = 0;
        barriers[i].buffer = *buffers[i];
        barriers[i].size = buffers[i]->size;
    }

    vkCmdPipelineBarrier(commandBuffer, srcStageMask, dstStageMask, 0,
                         0,
                         nullptr, COUNT(barriers), barriers.data(), 0, nullptr);
}

