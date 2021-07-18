#pragma once

#include "common.h"
#include "VulkanBuffer.h"
#include "VulkanRAII.h"

void addComputeBufferMemoryBarriers(VkCommandBuffer commandBuffer, const std::vector<VulkanBuffer*>& buffers);

void addBufferMemoryBarriers(VkCommandBuffer commandBuffer, const std::vector<VulkanBuffer*>& buffers, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask);

//void setDebugName(const std::string& name){
//    VkDebugUtilsObjectNameInfoEXT s{VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT, nullptr, VK_OBJECT_TYPE_BUFFER, (uint64_t)buffer, name.c_str()};
//    auto SetDebugUtilsObjectName = procAddress<PFN_vkSetDebugUtilsObjectNameEXT>(instance, "vkSetDebugUtilsObjectNameEXT");
//    SetDebugUtilsObjectName(logicalDevice, &s);
//}

