#pragma once

#include "VulkanDevice.h"
#include "VulkanBuffer.h"
#include "Texture.h"

class VulkanImageOps {
public:
    VulkanImageOps(VulkanDevice *device = nullptr);

    VulkanImageOps &srcTexture(Texture &texture);

    VulkanImageOps &dstTexture(Texture &texture);

    VulkanImageOps& srcBuffer(VulkanBuffer buffer);

    VulkanImageOps& imageSubresourceRange(VkImageAspectFlags aspectMask,
                               uint32_t baseMipLevel,
                               uint32_t levelCount,
                               uint32_t baseArrayLayer,
                               uint32_t layerCount);


    VulkanImageOps& sourceSrcPipelineStage(VkPipelineStageFlags flags);

    VulkanImageOps& srcPipelineStage(VkPipelineStageFlags flags);

    VulkanImageOps& dstPipelineStage(VkPipelineStageFlags flags);

    VulkanImageOps& sourceSrcAccessMask(VkAccessFlags flags);

    VulkanImageOps& srcAccessMask(VkAccessFlags flags);

    VulkanImageOps& dstAccessMask(VkAccessFlags flags);

    void copy(VkCommandBuffer commandBuffer);

    void transfer(VkCommandBuffer commandBuffer);


protected:
    class Impl;
    Impl* pimpl{ nullptr };
};