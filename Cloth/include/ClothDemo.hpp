#pragma once

#include "VulkanBaseApp.h"

struct Cloth{
    VulkanBuffer vertices;
    VulkanBuffer indices;
    uint32_t indexCount;
    uint32_t vertexCount;
    float size = 1.0f;
};

class ClothDemo : public VulkanBaseApp{
public:
    explicit ClothDemo(const Settings& settings);

protected:
    void initApp() override;

    void initCamera();

    void checkAppInputs() override;

    void createCloth();

    void createPipelines();

    void onSwapChainDispose() override;

    void onSwapChainRecreation() override;

    VkCommandBuffer *buildCommandBuffers(uint32_t imageIndex, uint32_t &numCommandBuffers) override;

    void update(float time) override;

private:
    VulkanCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;
    std::unique_ptr<OrbitingCameraController> cameraController;

    Cloth cloth;

    struct {
        VulkanPipelineLayout point;
        VulkanPipelineLayout wireframe;
        VulkanPipelineLayout shaded;
    } pipelineLayouts;

    struct {
        VulkanPipeline point;
        VulkanPipeline wireframe;
        VulkanPipeline shaded;
    } pipelines;
};