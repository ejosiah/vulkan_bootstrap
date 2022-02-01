#pragma once

#include "common.h"
#include "VulkanBuffer.h"
#include "VulkanRAII.h"

void addComputeBufferMemoryBarriers(VkCommandBuffer commandBuffer, const std::vector<VulkanBuffer*>& buffers);

void addBufferMemoryBarriers(VkCommandBuffer commandBuffer, const std::vector<VulkanBuffer*>& buffers, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask);

