#pragma once

#include "VulkanBaseApp.h"
#include "Font.h"
#include "CappedSink.h"
#include <spdlog/details/null_mutex.h>

class VulkanCube : public VulkanBaseApp {
public:
    explicit VulkanCube(const Settings& settings);

protected:
    void initApp() override;

    void onSwapChainDispose() override;

    void onSwapChainRecreation() override;

    VkCommandBuffer* buildCommandBuffers(uint32_t imageIndex, uint32_t& numCommandBuffers) override;

    void update(float time) override;

    void checkAppInputs() override;

    RenderPassInfo  buildRenderPass() override;

    void createCommandPool();

    void createCommandBuffer();

    void createGraphicsPipeline();

    void createDescriptorSetLayout();

    void createDescriptorPool();

    void createDescriptorSet();

    void createTextureBuffers();

    void createVertexBuffer();

    void createIndexBuffer();

    void cleanup() override;

private:
    VulkanPipelineLayout layout;
    VulkanPipeline pipeline;
    VulkanPipelineCache pipelineCache;
    VulkanCommandPool commandPool;
    VulkanDescriptorPool descriptorPool;
    VulkanDescriptorSetLayout descriptorSetLayout;

    std::vector<VkCommandBuffer> commandBuffers;
    VkDescriptorSet descriptorSet{};

    VulkanBuffer vertexBuffer;
    VulkanBuffer indexBuffer;
    Texture texture;
    std::unique_ptr<BaseCameraController> cameraController;
    Font* font;
    Vertices mesh;
    uint32_t numIndices;
    CappedSink<spdlog::details::null_mutex, 20> cappedSink;
};