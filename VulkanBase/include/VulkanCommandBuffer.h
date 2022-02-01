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

        ERR_GUARD_VULKAN(vkCreateCommandPool(device, &createInfo, nullptr, &pool));
        vkGetDeviceQueue(device, queueFamilyIndex, 0, &queue);
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
        this->queue = source.queue;

        source.pool = VK_NULL_HANDLE;
        source.queue = VK_NULL_HANDLE;

        return *this;
    }

    [[nodiscard]]
    inline std::vector<VkCommandBuffer> allocateCommandBuffers(uint32_t count = 1, VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY) const {
        std::vector<VkCommandBuffer> buffers(count);
        VkCommandBufferAllocateInfo allocateInfo{};
        allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocateInfo.commandPool = pool;
        allocateInfo.level = level;
        allocateInfo.commandBufferCount = count;

        ERR_GUARD_VULKAN(vkAllocateCommandBuffers(device, &allocateInfo, buffers.data()));

        return buffers;
    }

    template<typename CommandBuffers>
    inline void allocate(CommandBuffers& commandBuffers, VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY) const {
        VkCommandBufferAllocateInfo allocateInfo{};
        allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocateInfo.commandPool = pool;
        allocateInfo.level = level;
        allocateInfo.commandBufferCount = COUNT(commandBuffers);

        ERR_GUARD_VULKAN(vkAllocateCommandBuffers(device, &allocateInfo, commandBuffers.data()));
    }

    inline VkCommandBuffer allocate(VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY) const {
        VkCommandBuffer commandBuffer;
        VkCommandBufferAllocateInfo allocateInfo{};
        allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocateInfo.commandPool = pool;
        allocateInfo.level = level;
        allocateInfo.commandBufferCount = 1;

        ERR_GUARD_VULKAN(vkAllocateCommandBuffers(device, &allocateInfo, &commandBuffer));
        return commandBuffer;
    }

    inline void free(const std::vector<VkCommandBuffer>& commandBuffers) const {
        vkFreeCommandBuffers(device, pool, COUNT(commandBuffers), commandBuffers.data());
    }

    inline void reset(VkCommandPoolResetFlags flags = VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT ) const {
        vkResetCommandPool(device, pool, flags);
    }

    template<typename Command>
    inline void oneTimeCommand(Command&& command) const {
        auto commandBuffer = allocateCommandBuffers().front();
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

        ERR_GUARD_VULKAN(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
        vkQueueWaitIdle(queue);
        vkFreeCommandBuffers(device, pool, 1, &commandBuffer);
    }

    template<typename Command>
    inline void oneTimeCommands(uint32_t size, Command&& command, const std::string& name = "") const {
        auto commandBuffers = allocateCommandBuffers(size);
        for(auto i = 0; i < size; i++){

#ifdef DEBUG_MODE
            if(!name.empty()){
                VkDebugUtilsObjectNameInfoEXT nameInfo{};
                nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
                nameInfo.pObjectName = fmt::format("{}_{}", name, i).c_str();
                nameInfo.objectType = VK_OBJECT_TYPE_COMMAND_BUFFER;
                nameInfo.objectHandle = (uint64_t)commandBuffers[i];
                vkSetDebugUtilsObjectNameEXT(device, &nameInfo);
            }
#endif

            VkCommandBufferBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

            vkBeginCommandBuffer(commandBuffers[i], &beginInfo);
            command(i, commandBuffers[i]);
            vkEndCommandBuffer(commandBuffers[i]);
        }
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = size;
        submitInfo.pCommandBuffers = commandBuffers.data();

        ERR_GUARD_VULKAN(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
        vkQueueWaitIdle(queue);
        vkFreeCommandBuffers(device, pool, size, commandBuffers.data());
    }

    operator VkCommandPool() const {
        return pool;
    }

    VkDevice device = VK_NULL_HANDLE;
    VkCommandPool pool = VK_NULL_HANDLE;
    VkQueue queue = VK_NULL_HANDLE;
};
