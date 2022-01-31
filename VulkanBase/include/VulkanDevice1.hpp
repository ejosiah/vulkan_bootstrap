#pragma once
#include "common.h"
#include "vk_mem_alloc.h"
#include "VulkanImage.h"

struct VulkanDevice1{
    VkInstance instance{VK_NULL_HANDLE};
    VkPhysicalDevice pDevice{VK_NULL_HANDLE};
    VkDevice device{VK_NULL_HANDLE};
    VmaAllocator allocator{VK_NULL_HANDLE};

    [[nodiscard]]
    inline VulkanImage createImage(const VkImageCreateInfo& createInfo, VmaMemoryUsage usage = VMA_MEMORY_USAGE_GPU_ONLY) {
        VmaAllocationCreateInfo allocInfo{};
        allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        VkImage image = VK_NULL_HANDLE;
        VmaAllocation allocation = VK_NULL_HANDLE;
        VmaAllocationInfo allocationInfo{};
        spdlog::info("allocating memory for image");
        ASSERT(vmaCreateImage(allocator, &createInfo, &allocInfo, &image, &allocation, &allocationInfo));
        spdlog::info("allocated memory for image");

        return VulkanImage{ device, allocator, image, createInfo.format, allocation, createInfo.initialLayout, createInfo.extent };
    }
};