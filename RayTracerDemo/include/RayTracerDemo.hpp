#include "VulkanBaseApp.h"
#include "VulkanRayTraceModel.hpp"

struct RayTracingScratchBuffer{
    VkDeviceAddress deviceAddress = 0;
    VulkanBuffer buffer;
};

struct AccelerationStructure{
    VkAccelerationStructureKHR handle;
    VkDeviceAddress deviceAddress = 0;
    VulkanBuffer buffer;
};

class RayTracerDemo : public VulkanBaseApp{
public:
    RayTracerDemo(const Settings& settings);

protected:
    void initApp() override;

    void initCamera();

    void loadSpaceShip();

    void loadRayTracingPropertiesAndFeatures();

    void createModel();

    void createInverseCam();

    void createCommandPool();

    void createDescriptorPool();

    void createDescriptorSetLayouts();

    void createCanvasDescriptorSetLayout();

    void createDescriptorSets();

    void createCanvasDescriptorSet();

    void createCanvasPipeline();

    void createGraphicsPipeline();

    void createRayTracePipeline();

    void onSwapChainDispose() override;

    void onSwapChainRecreation() override;

    void rayTrace(VkCommandBuffer commandBuffer);

    void drawCanvas(VkCommandBuffer commandBuffer);

    VkCommandBuffer *buildCommandBuffers(uint32_t imageIndex, uint32_t &numCommandBuffers) override;

    void renderUI(VkCommandBuffer commandBuffer);

    void update(float time) override;

    void checkAppInputs() override;

    RayTracingScratchBuffer createScratchBuffer(VkDeviceSize size);

    void createBottomLevelAccelerationStructure();

    void createTopLevelAccelerationStructure();

    void createShaderBindingTable();

    void createStorageImage();

    void loadTexture();

    void createVertexBuffer();

    void rayTraceToCanvasBarrier(VkCommandBuffer commandBuffer) const;

    void CanvasToRayTraceBarrier(VkCommandBuffer commandBuffer) const;

    void cleanup() override;

    struct {
        VulkanBuffer vertices;
        VulkanBuffer indices;
    } triangle;


    struct {
        VulkanPipeline pipeline;
        VulkanPipelineLayout layout;
    } graphics;

    struct {
        VulkanPipeline pipeline;
        VulkanPipelineLayout layout;
        VulkanDescriptorSetLayout descriptorSetLayout;
        VulkanDescriptorSetLayout instanceDescriptorSetLayout;
        VulkanDescriptorSetLayout vertexDescriptorSetLayout;
        VkDescriptorSet descriptorSet;
        VkDescriptorSet instanceDescriptorSet;
        VkDescriptorSet vertexDescriptorSet;
    } raytrace;

    VulkanCommandPool commandPool;
    VulkanDescriptorPool descriptorPool;
  //  std::unique_ptr<SpectatorCameraController> camera;
    std::unique_ptr<OrbitingCameraController> camera;
    std::vector<VkCommandBuffer> commandBuffers;
    bool useRayTracing = true;
    bool debugOn = false;

    AccelerationStructure bottomLevelAS{};
    AccelerationStructure topLevelAs{};

    VulkanBuffer transformBuffer;
    std::vector<VkRayTracingShaderGroupCreateInfoKHR> shaderGroups{};

    struct { ;
        VulkanBuffer rayGenShader;
        VulkanBuffer missShader;
        VulkanBuffer hitShader;
    } bindingTable;


    struct {
        VulkanImage image;
        VulkanImageView imageview;
        VkFormat format;
    } storageImage;

    Texture texture;
    struct {
        VulkanPipelineLayout pipelineLayout;
        VulkanPipeline pipeline;
        VulkanDescriptorSetLayout descriptorSetLayout;
        VkDescriptorSet descriptorSet;
    } canvas;

    VulkanBuffer vertexBuffer;
    VulkanBuffer vertexColorBuffer;
    VulkanBuffer debugBuffer;

    VulkanDrawable spaceShip;
    VulkanDrawableInstance spaceShipInstance;

    VulkanBuffer inverseCamProj;
    rt::AccelerationStructureBuilder rtBuilder;
    std::vector<rt::Instance> asInstances;
    std::vector<rt::InstanceGroup> sceneObjects;

    VulkanBuffer sceneObjectBuffer;
    VulkanBuffer vertexOffsetsBuffer;

    VkPhysicalDeviceRayTracingPipelinePropertiesKHR  rayTracingPipelineProperties{};
    VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures{};

    VkPhysicalDeviceBufferDeviceAddressFeatures enabledBufferDeviceAddressFeatures{};
    VkPhysicalDeviceRayTracingPipelineFeaturesKHR enabledRayTracingPipelineFeatures{};
    VkPhysicalDeviceAccelerationStructureFeaturesKHR enabledAccelerationStructureFeatures{};
    VkPhysicalDeviceDescriptorIndexingFeatures enabledDescriptorIndexingFeatures{};
};