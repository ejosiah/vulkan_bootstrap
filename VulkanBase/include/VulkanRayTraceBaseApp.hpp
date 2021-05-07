#pragma once

#include <VulkanBaseApp.h>
#include "VulkanRayTraceModel.hpp"

struct ShaderBindingTable{
    VulkanBuffer buffer;
    VkStridedDeviceAddressRegionKHR stridedDeviceAddressRegion;

    operator VkStridedDeviceAddressRegionKHR*()  {
        return &stridedDeviceAddressRegion;
    }
};

class VulkanRayTraceBaseApp : public VulkanBaseApp{
public:
    VulkanRayTraceBaseApp(std::string_view name, const Settings& settings = {}, std::vector<std::unique_ptr<Plugin>> plugins = {});


protected:
    void loadRayTracingProperties();

    void createShaderbindingTables();

    void createShaderBindingTable(ShaderBindingTable& shaderBindingTable,  void* shaderHandleStoragePtr, VkBufferUsageFlags usageFlags, VmaMemoryUsage memoryUsage, uint32_t handleCount);

    VkStridedDeviceAddressRegionKHR getSbtEntryStridedDeviceAddressRegion(const VulkanBuffer& buffer, uint32_t handleCount) const;

    std::tuple<uint32_t, uint32_t> getShaderGroupHandleSizingInfo() const;

    void createStorageImage();

    void createCanvas();

    void createCanvasDescriptorSetLayout();

    void createCanvasDescriptorSet();

    void createCanvasPipeline();

    void drawCanvas(VkCommandBuffer commandBuffer);

    struct {
        VulkanImage image;
        VulkanImageView imageview;
        VkFormat format;
    } storageImage;

    struct {
        VulkanPipelineLayout pipelineLayout;
        VulkanPipeline pipeline;
        VulkanDescriptorSetLayout descriptorSetLayout;
        VkDescriptorSet descriptorSet;
    } canvas;

    rt::AccelerationStructureBuilder rtBuilder;
    std::vector<rt::Instance> asInstances;
    std::vector<rt::InstanceGroup> sceneObjects;

    VkPhysicalDeviceRayTracingPipelinePropertiesKHR  rayTracingPipelineProperties{};
    VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures{};

    VkPhysicalDeviceBufferDeviceAddressFeatures enabledBufferDeviceAddressFeatures{};
    VkPhysicalDeviceRayTracingPipelineFeaturesKHR enabledRayTracingPipelineFeatures{};
    VkPhysicalDeviceAccelerationStructureFeaturesKHR enabledAccelerationStructureFeatures{};
    VkPhysicalDeviceDescriptorIndexingFeatures enabledDescriptorIndexingFeatures{};
};