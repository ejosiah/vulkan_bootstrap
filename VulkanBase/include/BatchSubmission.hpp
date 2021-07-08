#pragma once

#include "VulkanDevice.h"

class BatchSubmission{
public:
    DISABLE_COPY(BatchSubmission);

    BatchSubmission() = default;

    explicit BatchSubmission(VkQueue queue);

private:
    VkQueue _queue;
    std::vector<VulkanSemaphore> _waits;
    std::vector<VkPipelineStageFlags> _waitFlags;
    std::vector<VulkanSemaphore> _signals;
    std::vector<VkCommandbuffer> _commands;

    [[nodiscard]]
    uint32_t commandBufferCount() const;

    [[nodiscard]]
    VkQueue queue() const;

    void init(VkQueue queue);

    void enqueue(std::vector<VkCommandBuffer> commandBuffers);

    void enqueue(VkCommandBuffer commandBuffer);

    void enqueue(VulkanSemaphore&& semaphore);

    void enqueueWait(VulkanSemaphore&& semaphore, VkPipelineStageFlags flag);

    VkResult execute(const VulkanFence* fence = nullptr,  uint32_t deviceMask = 0);

    void waitIdle() const;
};