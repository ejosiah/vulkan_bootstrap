#pragma once

#include "VulkanBaseApp.h"

class MarchingCubeDemo : public VulkanBaseApp{
public:
    MarchingCubeDemo(Settings settings);

protected:
    void initApp() override;

    void initCamera();

    void initVertexBuffer();

    void createCellBuffer();

    void createPipeline();

    void onSwapChainDispose() override;

    void onSwapChainRecreation() override;

    VkCommandBuffer *buildCommandBuffers(uint32_t imageIndex, uint32_t &numCommandBuffers) override;

    void update(float time) override;

    void checkAppInputs() override;

    void cleanup() override;

    void generateTriangles();

private:
    VulkanCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;
    std::vector<glm::vec3> vertices;
    std::vector<uint32_t> indices;
    VulkanBuffer vertexBuffer;
    VulkanBuffer drawCommandBuffer;
    VulkanBuffer cellBuffer;
    VulkanBuffer cellIndices;
    Action* nextConfig = nullptr;
    uint32_t config = 0;

    struct{
        VulkanPipeline points;
        VulkanPipeline lines;
        VulkanPipeline triangles;
    } pipelines;

    struct {
        VulkanPipelineLayout lines;
        VulkanPipelineLayout points;
        VulkanPipelineLayout triangles;
    } pipelineLayout;

    std::unique_ptr<OrbitingCameraController> camera;
};