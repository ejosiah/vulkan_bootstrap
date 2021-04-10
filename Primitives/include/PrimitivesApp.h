#pragma once

#define GLM_FORCE_SWIZZLE
#include "VulkanBaseApp.h"
#include "VulkanModel.h"

struct Object{
    VulkanBuffer vertices;
    VulkanBuffer indices;
    uint32_t indexCount;
    VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
};

struct Pipelines{
    VulkanPipeline trianglePipeline;
    VulkanPipeline triangleStripPipeline;
};

class PrimitivesApp : public VulkanBaseApp{
public:
    explicit PrimitivesApp(const Settings& settings);

protected:
    void initApp() override;

    void initCamera();

    void checkAppInputs() override;

    void onSwapChainDispose() override;

    void onSwapChainRecreation() override;

    void createPrimitives();

    void createPipeline();

    VkCommandBuffer *buildCommandBuffers(uint32_t imageIndex, uint32_t &numCommandBuffers) override;

    void update(float time) override;

private:
    VulkanCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;
    std::vector<Object> objects;
    int currentPrimitiveIndex = 0;
    Action* nextPrimitive;
    Pipelines pipelines;
    VulkanPipelineLayout layout;
    std::unique_ptr<OrbitingCameraController> cameraController;
};

