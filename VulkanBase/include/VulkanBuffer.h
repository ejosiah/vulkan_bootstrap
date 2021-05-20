#pragma once

#include <vulkan/vulkan.h>
#include "vk_mem_alloc.h"

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
        size = source.size;

        source.allocator = VK_NULL_HANDLE;
        source.buffer = VK_NULL_HANDLE;
        source.allocation = VK_NULL_HANDLE;
        source.name.clear();

        return *this;
    }

    void copy(const void* source, VkDeviceSize size, uint32_t offset = 0) const {
        assert(size + offset <= this->size);
        void* dest;
        vmaMapMemory(allocator, allocation, &dest);
        dest = static_cast<char*>(dest) + offset;
        memcpy(dest, source, size);
        vmaUnmapMemory(allocator, allocation);
    }

    template<typename T>
    void map(std::function<void(T*)> use){
        void* data;
        vmaMapMemory(allocator, allocation, &data);
        use(reinterpret_cast<T*>(data));
        vmaUnmapMemory(allocator, allocation);
    }

    template<typename T>
    void use(std::function<void(T)> func){
        map<T>([&](auto* ptr){
            int n = size/(sizeof(T));
           for(auto i = 0; i < n; i++){
               func(*(ptr+i));
           }
        });
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
    bool mappable = false;
};
