#pragma once
#include "VulkanDevice.h"
#include "filemanager.hpp"


namespace gpu {

    enum Operation { ADD = 0, SUBTRACT, MULTIPLY, DIVIDE, AND, OR};

    static VulkanDevice* g_device{};
    static FileManager* g_fileMgr;
    static VulkanCommandPool* g_commandPool{};

    void init(VulkanDevice& device, FileManager& fileManager);

    void shutdown();

    float average(VulkanBuffer& vulkanBuffer);

    void average(VkCommandBuffer commandBuffer, VkDescriptorSet inOutDescriptorSet, VulkanBuffer& input);

    void average(VulkanBuffer& input, VulkanBuffer& output);

    void reduce(VkCommandBuffer commandBuffer, VkDescriptorSet inOutDescriptorSet, VulkanBuffer& input, Operation operation = Operation::ADD);

    std::string resource(const std::string& name);

    float lastSum();

}