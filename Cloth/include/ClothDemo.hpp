#pragma once

#include "VulkanBaseApp.h"
#include "VulkanRayTraceBaseApp.hpp"
#include "VulkanQuery.hpp"
#include "ImGuiPlugin.hpp"


class ClothDemo : public VulkanRayTraceBaseApp{
public:
    explicit ClothDemo(const Settings& settings);

protected:
    void initApp() override;

    void checkInvariants();

    void initCamera();

    void initQueryPools();

    void checkAppInputs() override;

    void createCloth();

    void createSphere();

    void loadModel();


    void createFloor();

    void createPositionDescriptorSetLayout();

    void createDescriptorPool();

    void createPositionDescriptorSet();

    void createPipelines();

    void createComputePipeline();

    void createRayTraceDescriptorSetLayout();

    void createRayTraceDescriptorSet();

    void createRayTracePipeline();

    void createShaderBindingTables();

    void computeToComputeBarrier(VkCommandBuffer commandBuffer);

    void rayTraceToComputeBarrier(VkCommandBuffer commandBuffer);

    void computeToRayTraceBarrier(VkCommandBuffer commandBuffer);

    VkCommandBuffer dispatchCompute();

    void onSwapChainDispose() override;

    void onSwapChainRecreation() override;

    void newFrame() override;

    VkCommandBuffer *buildCommandBuffers(uint32_t imageIndex, uint32_t &numCommandBuffers) override;

    void drawWireframe(VkCommandBuffer commandBuffer);

    void drawShaded(VkCommandBuffer commandBuffer);

    void update(float time) override;

    void runPhysics(float time);

    void runPhysics0(float time, int i, int j);

    void renderUI(VkCommandBuffer commandBuffer);

    void cleanup() override;

private:
    VulkanCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;
    std::unique_ptr<BaseCameraController> cameraController;

    int input_index = 0;
    int output_index = 1;

    VulkanDescriptorSetLayout positionSetLayout;
    VulkanDescriptorPool descriptorPool;
    std::array<VkDescriptorSet, 2> positionDescriptorSets;
    VkDescriptorSet pointDescriptorSet;


    struct {
        VulkanPipelineLayout point;
        VulkanPipelineLayout wireframe;
        VulkanPipelineLayout shaded;
        VulkanPipelineLayout spaceShip;
        VulkanPipelineLayout compute;
        VulkanPipelineLayout normals;
    } pipelineLayouts;

    struct {
        VulkanPipeline point;
        VulkanPipeline wireframe;
        VulkanPipeline shaded;
        VulkanPipeline spaceShip;
        VulkanPipeline spaceShipWireframe;
        VulkanPipeline compute;
        VulkanPipeline normals;
    } pipelines;

    struct {
        VulkanDescriptorSetLayout setLayout;
        VkDescriptorSet descriptorSet;
    } collision;

    struct {
        glm::vec2 inv_cloth_size;
        float timeStep;
        float mass = 1.0;
        float ksStruct = 2000.0f;
        float ksShear = 2000.25f;
        float ksBend = 2000.25f;
        float kdStruct = 500.5f;
        float kdShear = 200.25f;
        float kdBend = 200.25f;
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
        VkDescriptorSet descriptorSet;
        VulkanDescriptorSetLayout setLayout;
        Texture diffuseMap;
        glm::vec2 size = glm::vec2(60.0f);
        glm::vec2 gridSize{100};
    } cloth;

    struct Sphere{
        VulkanBuffer vertices;
        VulkanBuffer indices;
        VulkanBuffer uboBuffer;
        float radius = 10;
        uint32_t indexCount;

        struct {
            glm::mat4 xform = glm::mat4(1);
            alignas(16) glm::vec3 center;
            float radius;
        } ubo;

    } sphere;

    VulkanDrawable model;
    rt::MeshObjectInstance modelInstance;

    struct ModelData{
        glm::mat4 xform;
        glm::mat4 xformIT;
        int numTriangles;
    } modelData;
    VulkanBuffer modelBuffer;

    float frameTime = 0.0005;
    bool startSim = false;

    enum class Shading : int {
        WIREFRAME = 0,
        SHADED = 1
    } shading = Shading::SHADED;
    bool showPoints = false;
    bool showNormals = false;
    float shine = 50;

   // TimestampQueryPool timerQueryPool;
    VkQueryPool queryPool = VK_NULL_HANDLE;
    struct TimeQueryResults{
        uint64_t computeStart;
        uint64_t computeEnd;
        uint64_t collisionStart;
        uint64_t collisionEnd;
    } timeQueryResults;
    float computeDuration = 0;
    float collisionDuration = 0;

    struct {
        VulkanPipelineLayout layout;
        VulkanPipeline pipeline;
        VulkanDescriptorSetLayout descriptorSetLayout;
        VkDescriptorSet descriptorSet;
    } raytrace;

    struct {
        ShaderBindingTable rayGen;
        ShaderBindingTable miss;
        ShaderBindingTable hit;
    } shaderBindingTables;

    std::vector<VkRayTracingShaderGroupCreateInfoKHR> shaderGroups{};
};