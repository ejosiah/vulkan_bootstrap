#pragma once

#include "VulkanBaseApp.h"

class VulkanCube : public VulkanBaseApp{
public:
    VulkanCube();

protected:
    void initApp() override;

    void onSwapChainDispose() override;

    void onSwapChainRecreation() override;

    std::vector<VkCommandBuffer> buildCommandBuffers(uint32_t i) override;

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
    Texture texture;

};