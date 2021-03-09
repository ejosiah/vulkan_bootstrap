#pragma once

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

struct VulkanBuffer{

    VulkanBuffer() = default;

    inline VulkanBuffer(VmaAllocator allocator, VkBuffer buffer, VmaAllocation allocation, VkDeviceSize size)
    : allocator(allocator)
    , buffer(buffer)
    , allocation(allocation)
    , size(size)
    {}

    VulkanBuffer(const VulkanBuffer&) = delete;

    VulkanBuffer(VulkanBuffer&& source) noexcept {
        operator=(static_cast<VulkanBuffer&&>(source));
    }

    VulkanBuffer& operator=(const VulkanBuffer&) = delete;

    VulkanBuffer& operator=(VulkanBuffer&& source) noexcept{
        allocator = source.allocator;
        buffer = source.buffer;
        allocation = source.allocation;

        source.allocator = VK_NULL_HANDLE;
        source.buffer = VK_NULL_HANDLE;
        source.allocation = VK_NULL_HANDLE;

        return *this;
    }

    void copy(void* source, VkDeviceSize size) const {
        void* dest;
        vmaMapMemory(allocator, allocation, &dest);
        memcpy(dest, source, size);
        vmaUnmapMemory(allocator, allocation);
    }

    ~VulkanBuffer(){
        if(buffer){
            vmaDestroyBuffer(allocator, buffer, allocation);
        }
    }

    operator VkBuffer() const {
        return buffer;
    }

    VmaAllocator allocator = VK_NULL_HANDLE;
    VkBuffer buffer = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
    VkDeviceSize  size = 0;
};
