#pragma once

#include "VulkanBaseApp.h"
#include "ImGuiPlugin.hpp"


class ClothDemo : public VulkanBaseApp{
public:
    explicit ClothDemo(const Settings& settings);

protected:
    void initApp() override;

    void initCamera();

    void checkAppInputs() override;

    void createCloth();

    void createSphere();

    void createFloor();

    void createPositionDescriptorSetLayout();

    void createDescriptorPool();

    void createPositionDescriptorSet();

    void createPipelines();

    void createComputePipeline();

    void computeToComputeBarrier(VkCommandBuffer commandBuffer);

    VkCommandBuffer dispatchCompute();

    void onSwapChainDispose() override;

    void onSwapChainRecreation() override;

    VkCommandBuffer *buildCommandBuffers(uint32_t imageIndex, uint32_t &numCommandBuffers) override;

    void update(float time) override;

    void runPhysics(float time);

    void runPhysics0(float time, int i, int j);

    void renderUI(VkCommandBuffer commandBuffer);

private:
    VulkanCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;
    std::unique_ptr<BaseCameraController> cameraController;

    int input_index = 0;
    int output_index = 1;

    VulkanDescriptorSetLayout positionSetLayout;
    VulkanDescriptorPool descriptorPool;
    std::array<VkDescriptorSet, 2> positionDescriptorSets;


    struct {
        VulkanPipelineLayout point;
        VulkanPipelineLayout wireframe;
        VulkanPipelineLayout shaded;
        VulkanPipelineLayout compute;
    } pipelineLayouts;

    struct {
        VulkanPipeline point;
        VulkanPipeline wireframe;
        VulkanPipeline shaded;
        VulkanPipeline compute;
    } pipelines;

    struct {
        VulkanDescriptorSetLayout setLayout;
        VkDescriptorSet descriptorSet;
    } collision;

    struct {
        glm::vec2 inv_cloth_size;
        float timeStep;
        float mass = 1.0;
        float ksStruct = 100.0f;
        float ksShear = 1.25f;
        float ksBend = 1.25f;
        float kdStruct = 5.5f;
        float kdShear = 0.25f;
        float kdBend = 0.25f;
        float kd = 0.05f;
        float elapsedTime = 0;
    } constants{};

    int numIterations = 1;

    struct {
        VulkanBuffer vertices;
        VulkanBuffer indices;
        uint32_t indexCount;
    } floor;

    struct Cloth{
        std::array<VulkanBuffer, 2> vertices;
        VulkanBuffer vertexAttributes;
        VulkanBuffer indices;
        uint32_t indexCount;
        uint32_t vertexCount;
        glm::vec2 size = glm::vec2(60.0f);
        glm::vec2 gridSize{10};
    } cloth;

    struct Sphere{
        VulkanBuffer vertices;
        VulkanBuffer indices;
        VulkanBuffer uboBuffer;
        float radius = 10;
        uint32_t indexCount;

        struct {
            glm::mat4 xform = glm::mat4(0);
        } ubo;

    } sphere;

    float frameTime = 0.0005;
    bool startSim = false;
   // float frameTime =  0.0083;
};