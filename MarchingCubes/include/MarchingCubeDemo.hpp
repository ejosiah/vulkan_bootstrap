#pragma once

#include "VulkanBaseApp.h"

struct mVertex{
    glm::vec3 position;
    glm::vec3 normal;
};

class MarchingCubeDemo : public VulkanBaseApp{
public:
    MarchingCubeDemo(Settings settings);

protected:
    void initApp() override;

    void initCamera();

    void initSdf();

    void createSdf();

    void initVertexBuffer();

    void createCellBuffer();

    void createDescriptorPool();

    void createDescriptorSetLayout();

    void createMarchingCubeDescriptorSetLayout();

    void createMarchingCubeDescriptorSet();

    void createMarchingCubePipeline();

    void createDescriptorSet();

    void createPipeline();

    void onSwapChainDispose() override;

    void onSwapChainRecreation() override;

    VkCommandBuffer *buildCommandBuffers(uint32_t imageIndex, uint32_t &numCommandBuffers) override;

    void update(float time) override;

    void checkAppInputs() override;

    void cleanup() override;

    void generateTriangles();

    void renderText(VkCommandBuffer commandBuffer);

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
        VulkanPipeline sdf;
    } pipelines;

    struct {
        VulkanPipelineLayout lines;
        VulkanPipelineLayout points;
        VulkanPipelineLayout triangles;
        VulkanPipelineLayout sdf;
    } pipelineLayout;

    struct {
        VulkanDescriptorSetLayout descriptorSetLayout;
        VkDescriptorSet descriptorSet;
        VulkanPipelineLayout layout;
        VulkanPipeline pipeline;
        VulkanBuffer vertexCountBuffer;
        VulkanBuffer nextVertexIdBuffer;
        VulkanBuffer edgeTableBuffer;
        VulkanBuffer triTableBuffer;

        struct {
            float isoLevel = 0.0;
            int pass = 0;
        } constants;
    } marchingCube;

    Texture sdf;

    VulkanDescriptorSetLayout renderDescriptorSetLayout;
    VulkanDescriptorSetLayout computeDescriptorSetLayout;
    VkDescriptorSet renderDescriptorSet;
    VkDescriptorSet computeDescriptorSet;
    VulkanDescriptorPool descriptorPool;

    std::unique_ptr<OrbitingCameraController> camera;
};