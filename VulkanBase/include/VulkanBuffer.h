#pragma once

#include <vulkan/vulkan.h>
#include "../../3rdParty/include/vk_mem_alloc.h"

struct VulkanBuffer{

    DISABLE_COPY(VulkanBuffer)

    VulkanBuffer() = default;

    inline VulkanBuffer(VmaAllocator allocator, VkBuffer buffer, VmaAllocation allocation, VkDeviceSize size, const std::string name = "")
    : allocator(allocator)
    , buffer(buffer)
    , allocation(allocation)
    , size(size)
    , name(name)
    {}

    VulkanBuffer(VulkanBuffer&& source) noexcept {
        operator=(static_cast<VulkanBuffer&&>(source));
    }

    VulkanBuffer& operator=(VulkanBuffer&& source) noexcept{
        if(&source == this) return *this;

        this->~VulkanBuffer();

        allocator = source.allocator;
        buffer = source.buffer;
        allocation = source.allocation;
        name = source.name;

        source.allocator = VK_NULL_HANDLE;
        source.buffer = VK_NULL_HANDLE;
        source.allocation = VK_NULL_HANDLE;
        source.name.clear();

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

    operator VkBuffer*() {
        return &buffer;
    }

    VmaAllocator allocator = VK_NULL_HANDLE;
    VkBuffer buffer = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
    VkDeviceSize  size = 0;
    std::string name;
};
