#pragma once

#include "common.h"

struct VulkanFence{

    DISABLE_COPY(VulkanFence)

    VulkanFence() = default;

    VulkanFence(VkDevice device, VkFenceCreateFlags flags = VK_FENCE_CREATE_SIGNALED_BIT)
    : device(device)
    {
        VkFenceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        createInfo.flags = flags;

        ERR_GUARD_VULKAN(vkCreateFence(device, &createInfo, nullptr, &fence));
    }

    VulkanFence(VulkanFence&& source) noexcept {
        operator=(static_cast<VulkanFence&&>(source));
    }

    ~VulkanFence(){
        if(fence){
            vkDestroyFence(device, fence, nullptr);
        }
    }

    VulkanFence& operator=(VulkanFence&& source) noexcept {
        if(&source == this) return *this;

        fence = source.fence;
        device = source.device;

        source.fence = VK_NULL_HANDLE;
        source.device = VK_NULL_HANDLE;

        return *this;
    }

    VkResult wait(uint64_t timeout = UINT64_MAX) const {
        return vkWaitForFences(device, 1, &fence, VK_TRUE, timeout);
    }

    VkResult reset() const {
        return vkResetFences(device, 1, &fence);
    }

    static VkResult wait(std::vector<VulkanFence> fences, bool waitAll = VK_TRUE, uint64_t timeout = UINT64_MAX){
        std::vector<VkFence> rawFences(fences.size());
        std::transform(begin(fences), end(fences), begin(rawFences), [](auto& fen){ return fen.fence; });

        return vkWaitForFences(fences.front().device, COUNT(rawFences), rawFences.data(), waitAll, timeout);
    }

    static VkResult reset(std::vector<VulkanFence> fences){
        std::vector<VkFence> rawFences(fences.size());
        std::transform(begin(fences), end(fences), begin(rawFences), [](auto& fen){ return fen.fence; });

        return vkResetFences(fences.front().device, COUNT(rawFences), rawFences.data());
    }

    operator VkFence() const {
        return fence;
    }

    operator VkFence*() {
        return &fence;
    }

    VkDevice device = VK_NULL_HANDLE;
    VkFence fence = VK_NULL_HANDLE;
};