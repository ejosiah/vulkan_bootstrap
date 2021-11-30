#pragma once

#include "VulkanBaseApp.h"

class VulkanCubeInstanced : public VulkanBaseApp {
public:
    VulkanCubeInstanced();

protected:
    void initApp() override;

    void onSwapChainDispose() override;

    void onSwapChainRecreation() override;

    VkCommandBuffer* buildCommandBuffers(uint32_t imageIndex, uint32_t& numCommandBuffers) override;

    void update(float time) override;

    void createCommandPool();

    void createCommandBuffer();

    void createGraphicsPipeline();

    void createDescriptorSetLayout();

    void createDescriptorPool();

    void createDescriptorSet();

    void createTextureBuffers();

    void createVertexBuffer();

    void createIndexBuffer();

private:
    VulkanPipelineLayout layout;
    VulkanPipeline pipeline;
    VulkanCommandPool commandPool;
    VulkanDescriptorPool descriptorPool;
    VulkanDescriptorSetLayout descriptorSetLayout;

    std::vector<VkCommandBuffer> commandBuffers;
    VkDescriptorSet descriptorSet;

    VulkanBuffer vertexBuffer;
    VulkanBuffer indexBuffer;
    VulkanBuffer xformBuffer;
    Texture texture;
    uint32_t numInstances = 100;
};