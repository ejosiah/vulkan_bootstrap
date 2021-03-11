#pragma once

#include "common.h"

struct VulkanBuffer{

    DISABLE_COPY(VulkanBuffer)

    VulkanBuffer() = default;

    VulkanBuffer(VkBuffer buffer, VmaAllocation allocation, VkDeviceSize size, VmaAllocator allocator)
    : buffer(buffer)
    , allocation(allocation)
    , size(size)
    , allocator(allocator)
    {}

    VulkanBuffer(VulkanBuffer&& source) noexcept {
        operator=(static_cast<VulkanBuffer&&>(source));
    }

    VulkanBuffer& operator=(VulkanBuffer&& source) noexcept {
        if(&source == this) return *this;
        this->buffer = source.buffer;
        this->allocation = source.allocation;
        this->allocator = source.allocator;
        this->size  = source.size;

        source.buffer = VK_NULL_HANDLE;
        source.allocation = VK_NULL_HANDLE;
        source.allocator = VK_NULL_HANDLE;

        return * this;
    }


    void copy(void* source, VkDeviceSize size) const{
        void * dest;
        vmaMapMemory(allocator, allocation, &dest);
        memcpy(dest, source, size);
        vmaUnmapMemory(allocator, allocation);
    }

    ~VulkanBuffer(){
        if(buffer){
            vmaDestroyBuffer(allocator, buffer, allocation);

        }
    }

    VkBuffer buffer = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
    VkDeviceSize  size = 0;
    VmaAllocator allocator = VK_NULL_HANDLE;
};