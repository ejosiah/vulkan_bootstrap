#include "VulkanBaseApp.h"
#include "VulkanRayTraceModel.hpp"
#include "Canvas.hpp"

struct ShaderBindingTable{
    VulkanBuffer buffer;
    VkStridedDeviceAddressRegionKHR stridedDeviceAddressRegion;

    operator VkStridedDeviceAddressRegionKHR*()  {
        return &stridedDeviceAddressRegion;
    }
};

class RayTracerDemo : public VulkanBaseApp{
public:
    RayTracerDemo(const Settings& settings);

protected:
    void initApp() override;

    void initCamera();

    void loadSpaceShip();

    void loadStatue();

    void loadRayTracingPropertiesAndFeatures();

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

    void drawCanvas(VkCommandBuffer commandBuffer);

    VkCommandBuffer *buildCommandBuffers(uint32_t imageIndex, uint32_t &numCommandBuffers) override;

    void renderUI(VkCommandBuffer commandBuffer);

    void update(float time) override;

    void checkAppInputs() override;

    void createBottomLevelAccelerationStructure();

    void createTopLevelAccelerationStructure();

    void createShaderbindingTables();

    void createShaderBindingTable(ShaderBindingTable& shaderBindingTable,  void* shaderHandleStoragePtr, VkBufferUsageFlags usageFlags, VmaMemoryUsage memoryUsage, uint32_t handleCount);

    VkStridedDeviceAddressRegionKHR getSbtEntryStridedDeviceAddressRegion(const VulkanBuffer& buffer, uint32_t handleCount) const;

    std::tuple<uint32_t, uint32_t> getShaderGroupHandleSizingInfo() const;


    void createStorageImage();

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


    struct {
        VulkanImage image;
        VulkanImageView imageview;
        VkFormat format;
    } storageImage;

    Texture texture;

    Canvas canvas;

    VulkanDrawable spaceShip;
    VulkanDrawableInstance spaceShipInstance;

    VulkanDrawable plane;
    VulkanDrawableInstance planeInstance;

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