#pragma once

#include <vulkan/vulkan.h>

template<typename Handle, typename Deleter>
struct VulkanHandle{

    VulkanHandle() = default;

    VulkanHandle(VkDevice device, Handle handle)
            :device(device)
            , handle(handle)
    {

    }

    VulkanHandle(const VulkanHandle&) = delete;

    VulkanHandle(VulkanHandle&& source) noexcept {
        operator=(static_cast<VulkanHandle&&>(source));
    }

    ~VulkanHandle(){
        if(handle){
            Deleter()(device, handle);
        }
    }

    VulkanHandle& operator=(const VulkanHandle&) = delete;

    VulkanHandle& operator=(VulkanHandle&& source) noexcept {
        if(this == &source) return *this;
        this->device = source.device;
        this->handle = source.handle;

        source.handle = VK_NULL_HANDLE;

        return *this;
    }

    operator Handle() const {
        return handle;
    }

//    operator Handle*() const {
//        return &handle;
//    }

    VkDevice device = VK_NULL_HANDLE;
    Handle handle = VK_NULL_HANDLE;
};

//struct SemaphoreDeleter{
//
//    inline void operator()(VkDevice device, VkSemaphore semaphore){
//        vkDestroySemaphore(device, semaphore, nullptr);
//    }
//};

#define VULKAN_RAII(Resource) \
struct Resource##Deleter{ \
    inline void operator()(VkDevice device, Vk##Resource resource){ \
        vkDestroy##Resource(device, resource, nullptr);          \
    }      \
};                       \
using Vulkan##Resource = VulkanHandle<Vk##Resource, Resource##Deleter>;

VULKAN_RAII(Pipeline)
VULKAN_RAII(DescriptorSetLayout)
VULKAN_RAII(Semaphore)
