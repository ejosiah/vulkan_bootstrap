#include "VulkanBaseApp.h"
#include "VulkanRayTraceModel.hpp"
#include "Canvas.hpp"
#include "VulkanRayTraceBaseApp.hpp"

class RayTracerDemo : public VulkanRayTraceBaseApp{
public:
    RayTracerDemo(const Settings& settings);

protected:
    void initApp() override;

    void initCamera();

    void loadSpaceShip();

    void loadStatue();

    void createModel();

    void createInverseCam();

    void createCommandPool();

    void createDescriptorPool();

    void createDescriptorSetLayouts();

    void createDescriptorSets();

    void createGraphicsPipeline();

    void createRayTracePipeline();

    void onSwapChainDispose() override;

    void onSwapChainRecreation() override;

    void rayTrace(VkCommandBuffer commandBuffer);

    VkCommandBuffer *buildCommandBuffers(uint32_t imageIndex, uint32_t &numCommandBuffers) override;

    void renderUI(VkCommandBuffer commandBuffer);

    void update(float time) override;

    void checkAppInputs() override;

    void createBottomLevelAccelerationStructure();

    void createShaderbindingTables();

    void loadTexture();

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

    VulkanBuffer transformBuffer;
    std::vector<VkRayTracingShaderGroupCreateInfoKHR> shaderGroups{};


    struct { ;
        ShaderBindingTable rayGenShader;
        ShaderBindingTable missShader;
        ShaderBindingTable hitShader;
    } bindingTables;

    Texture texture;

//    VulkanDrawable spaceShip;
//    VulkanDrawableInstance spaceShipInstance;

//    VulkanDrawable plane;
//    VulkanDrawableInstance planeInstance;

    VulkanBuffer inverseCamProj;

    VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures{};

    VkPhysicalDeviceBufferDeviceAddressFeatures enabledBufferDeviceAddressFeatures{};
    VkPhysicalDeviceRayTracingPipelineFeaturesKHR enabledRayTracingPipelineFeatures{};
    VkPhysicalDeviceAccelerationStructureFeaturesKHR enabledAccelerationStructureFeatures{};
    VkPhysicalDeviceDescriptorIndexingFeatures enabledDescriptorIndexingFeatures{};
};