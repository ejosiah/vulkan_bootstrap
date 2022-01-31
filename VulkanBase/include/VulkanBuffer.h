#pragma once

#include "common.h"
#include <vulkan/vulkan.h>
#include "vk_mem_alloc.h"
struct VulkanBuffer{

    VulkanBuffer() = default;

    inline VulkanBuffer(VmaAllocator allocator, VkBuffer buffer, VmaAllocation allocation, VkDeviceSize size, const std::string name = "", bool mappable = false)
    : allocator(allocator)
    , buffer(buffer)
    , allocation(allocation)
    , size(size)
    , name(name)
    , mappable(mappable)
    {
        refCounts[buffer] = 1;
    }

    VulkanBuffer(const VulkanBuffer& source){
        operator=(source);
    }

    VulkanBuffer& operator=(const VulkanBuffer& source){
        if(&source == this) return *this;

        allocator = source.allocator;
        buffer = source.buffer;
        allocation = source.allocation;
        name = source.name;
        size = source.size;
        if(buffer) {
            incrementRef(buffer);
        }
        return *this;
    }

    VulkanBuffer(VulkanBuffer&& source) noexcept {
        operator=(static_cast<VulkanBuffer&&>(source));
    }

    VulkanBuffer& operator=(VulkanBuffer&& source) noexcept {
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

    template<typename T>
    void copy(std::vector<T> dest, uint32_t offset = 0) const {
        assert(!dest.empty());
        copy(dest.data(), sizeof(T) * dest.size(), offset);
    }

    void copy(const void* source, VkDeviceSize size, uint32_t offset = 0) const {
        assert(size + offset <= this->size);
        void* dest;
        ASSERT(vmaMapMemory(allocator, allocation, &dest));
        dest = static_cast<char*>(dest) + offset;
        memcpy(dest, source, size);
        ASSERT(vmaUnmapMemory(allocator, allocation));
    }

    template<typename T>
    void map(std::function<void(T*)> use){
        auto ptr = map();
        use(reinterpret_cast<T*>(ptr));
        unmap();
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
            auto itr = refCounts.find(buffer);
            assert(itr != refCounts.end());
            if(itr->second == 1) {
                spdlog::debug("no more references to VkBuffer[{}], destroying now ...", (uint64_t)buffer);
                refCounts.erase(itr);
                if (mapped) {
                    unmap();
                }
                vmaDestroyBuffer(allocator, buffer, allocation);
            }else{
                decrementRef(buffer);
            }
        }
    }

    operator VkBuffer() const {
        return buffer;
    }

    operator VkBuffer*() {
        return &buffer;
    }

    void* map(){
        vmaMapMemory(allocator, allocation, &mapped);
        return mapped;
    }

    void unmap() {
        vmaUnmapMemory(allocator, allocation);
        mapped = nullptr;
    }

    template<typename T>
    T get(int index){
        // TODO check if mappable & bounds
        T res;
        map<T>([&](auto ptr){
            res = ptr[index];
        });
        return res;
    }

    void clear(VkCommandBuffer commandBuffer) const {
        vkCmdFillBuffer(commandBuffer, buffer, 0, size, 0);
    }

    static void incrementRef(VkBuffer buffer){
        ensureRef(buffer);
        refCounts[buffer]++;
        spdlog::debug("{} current references to VkBuffer[{}]", refCounts[buffer], (uint64_t)buffer);
    }

    static void decrementRef(VkBuffer buffer){
        ensureRef(buffer);
        refCounts[buffer]--;
        spdlog::debug("{} current references to VkBuffer[{}]", refCounts[buffer], (uint64_t)buffer);
    }

    static void ensureRef(VkBuffer buffer){
        assert(refCounts.find(buffer) != refCounts.end());
    }

    VmaAllocator allocator = VK_NULL_HANDLE;
    VkBuffer buffer = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
    VkDeviceSize  size = 0;
    std::string name{};
    void* mapped = nullptr;
    bool isMapped = false;
    bool mappable = false;
    static std::map<VkBuffer, std::atomic_uint32_t> refCounts;
};