#pragma once

#include "model.hpp"
#include "VulkanBaseApp.h"
#include "SdfCompute.hpp"
#include "VolumeParticleEmitter.hpp"

constexpr uint32_t MAX_PARTICLES = 500000;
constexpr float WATER_DENSITY = 1000.0;

class SPHFluidSimulation : public VulkanBaseApp{
public:
    explicit SPHFluidSimulation(const Settings& settings = {});

protected:
    void initApp() override;

    void initGrid();

    void createParticles();

    void initGridBuilder();

    void createSdf();

    void createEmitter();

    void createDescriptorPool();

    void addParticleBufferBarrier(VkCommandBuffer commandBuffer, int index);

    void createDescriptorSetLayouts();

    void createDescriptorSets();

    void createRenderDescriptorSet();

    void createCommandPool();

    void createPipelineCache();

    void createRenderPipeline();

    void createComputePipeline();

    void runPhysics();

    void onSwapChainDispose() override;

    void onSwapChainRecreation() override;

    VkCommandBuffer *buildCommandBuffers(uint32_t imageIndex, uint32_t &numCommandBuffers) override;

    void update(float time) override;

    void checkAppInputs() override;

    void cleanup() override;

    void onPause() override;

    void createGridBuilderDescriptorSetLayout();

    void createGridBuilderDescriptorSet(VkDeviceSize offset, VkDeviceSize range);

    void createGridBuilderPipeline();

    void buildPointHashGrid();

    void GeneratePointHashGrid(int pass = 0);

    void initCamera();

protected:
    struct {
        VulkanPipelineLayout layout;
        VulkanPipeline pipeline;
        VulkanDescriptorSetLayout setLayout;
        VkDescriptorSet descriptorSet;
        VulkanBuffer mvpBuffer;
    } render;

    struct {
        VulkanPipelineLayout layout;
        VulkanPipeline pipeline;
    } compute;

    struct {
        VulkanBuffer vertexBuffer;
        struct {
            glm::mat4 model = glm::mat4(1);
            glm::mat4 view  = glm::mat4(1);
            glm::mat4 projection = glm::mat4(1);
        } xforms;
        uint32_t numCells = 16;
        float size = 1;
    } grid;

    struct {
        VulkanPipelineLayout layout;
        VulkanPipeline pipeline;
        std::array<VulkanBuffer, 2> buffers;
        std::array<VkDescriptorSet, 2> descriptorSets;
        VulkanDescriptorSetLayout descriptorSetLayout;

        struct {
            glm::vec3 gravity{0, -9.8, 0};
            uint32_t numParticles{ 1024 };
            float drag = 1e-4;
            float time;
        } constants;

    } particles;

    struct {
        VulkanPipelineLayout layout;
        VulkanPipeline pipeline;
        VulkanDescriptorSetLayout descriptorSetLayout;
        VkDescriptorSet descriptorSet;
        VulkanBuffer buffer;

        struct {
            glm::ivec3 resolution{1};
            float gridSpacing{1};
            uint32_t pass{0};
            uint32_t numParticles{0};
        } constants;
    } gridBuilder;

    VulkanDescriptorPool descriptorPool;
    VulkanCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;
    VulkanPipelineCache pipelineCache;

    SdfCompute sdf;
    VolumeParticleEmitter emitter;
    std::shared_ptr<OrbitingCameraController> camera;

    uint32_t numParticles;
    float eosExponent = 7.0;
    float negativePressureScale = 0.0;
    float viscosityCoefficient = 0.01;
    float pseudoViscosityCoefficient = 10.0;
    float speedOfSound = 100.0;

    struct {
        float radius = 1e-3;
        float mass = 1e-3;
        float targetDensity = WATER_DENSITY;
        float targetSpacing = 0.1;
        float kernelRadiusOverTargetSpacing = 1.8;
        float kernelRadius = 0.0f;
    } particleProperties;
};