#pragma once

#include "common.h"

struct VulkanCommandPool{

    VulkanCommandPool() = default;

    inline VulkanCommandPool(VkDevice device , uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags = 0, void* nextChain = VK_NULL_HANDLE)
    :device(device)
    {
        VkCommandPoolCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        createInfo.pNext = nextChain;
        createInfo.flags = flags;
        createInfo.queueFamilyIndex = queueFamilyIndex;

        ASSERT(vkCreateCommandPool(device, &createInfo, nullptr, &pool));
    }

    VulkanCommandPool(const VulkanCommandPool&) = delete;

    VulkanCommandPool(VulkanCommandPool&& source) noexcept {
        operator=(static_cast<VulkanCommandPool&&>(source));
    }

    ~VulkanCommandPool(){
        if(pool){
            vkDestroyCommandPool(device, pool, nullptr);
        }
    }

    VulkanCommandPool& operator=(const VulkanCommandPool&) = delete;

    VulkanCommandPool& operator=(VulkanCommandPool&& source) noexcept {
        if(this == &source) return *this;

        this->device = source.device;
        this->pool = source.pool;

        source.pool = VK_NULL_HANDLE;

        return *this;
    }

    inline std::vector<VkCommandBuffer> allocate(uint32_t count = 1, VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY) const {
        std::vector<VkCommandBuffer> buffers(count);
        VkCommandBufferAllocateInfo allocateInfo{};
        allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocateInfo.commandPool = pool;
        allocateInfo.level = level;
        allocateInfo.commandBufferCount = count;

        ASSERT(vkAllocateCommandBuffers(device, &allocateInfo, buffers.data()));

        return buffers;
    }

    inline void free(const std::vector<VkCommandBuffer>& commandBuffers) const {
        vkFreeCommandBuffers(device, pool, COUNT(commandBuffers), commandBuffers.data());
    }

    inline void reset(VkCommandPoolResetFlags flags = VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT ) const {
        vkResetCommandPool(device, pool, flags);
    }

    template<typename Command>
    inline void oneTimeCommand(VkQueue queue, Command&& command) const {
        auto commandBuffer = allocate().front();
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(commandBuffer, &beginInfo);
        command(commandBuffer);
        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        ASSERT(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
        vkQueueWaitIdle(queue);
        vkFreeCommandBuffers(device, pool, 1, &commandBuffer);
    }

    operator VkCommandPool() const {
        return pool;
    }

    VkDevice device = VK_NULL_HANDLE;
    VkCommandPool pool = VK_NULL_HANDLE;
};

