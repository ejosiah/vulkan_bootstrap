#pragma once

#include "VulkanBaseApp.h"
#include "VulkanRayTraceModel.hpp"
#include "VulkanDevice.h"
#include "Canvas.hpp"
#include "shader_binding_table.hpp"


class VulkanRayTraceBaseApp : public VulkanBaseApp{
public:
    explicit VulkanRayTraceBaseApp(std::string_view name, const Settings& settings = {}, std::vector<std::unique_ptr<Plugin>> plugins = {});

    ~VulkanRayTraceBaseApp() override;

protected:
    void postVulkanInit() final;

    void framebufferReady() final;

    void loadRayTracingProperties();

    void createAccelerationStructure(const std::vector<rt::MeshObjectInstance>& drawableInstances);

    void createShaderBindingTable(ShaderBindingTable& shaderBindingTable,  void* shaderHandleStoragePtr, VkBufferUsageFlags usageFlags, VmaMemoryUsage memoryUsage, uint32_t handleCount);

    VkStridedDeviceAddressRegionKHR getSbtEntryStridedDeviceAddressRegion(const VulkanBuffer& buffer, uint32_t handleCount) const;

    std::tuple<uint32_t, uint32_t> getShaderGroupHandleSizingInfo() const;

    Canvas canvas;

    rt::AccelerationStructureBuilder rtBuilder;
    std::vector<rt::Instance> asInstances;
    std::vector<rt::InstanceGroup> sceneObjects;

    VkPhysicalDeviceRayTracingPipelinePropertiesKHR  rayTracingPipelineProperties{};
    VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures{};

    VkPhysicalDeviceBufferDeviceAddressFeatures enabledBufferDeviceAddressFeatures{};
    VkPhysicalDeviceRayTracingPipelineFeaturesKHR enabledRayTracingPipelineFeatures{};
    VkPhysicalDeviceAccelerationStructureFeaturesKHR enabledAccelerationStructureFeatures{};
    VkPhysicalDeviceDescriptorIndexingFeatures enabledDescriptorIndexingFeatures{};

    VulkanBuffer sceneObjectBuffer;

    std::map<std::string, VulkanDrawable> drawables;
};