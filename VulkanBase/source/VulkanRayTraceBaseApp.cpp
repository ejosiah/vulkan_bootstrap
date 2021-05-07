#include "VulkanRayTraceBaseApp.hpp"

#include <utility>

VulkanRayTraceBaseApp::VulkanRayTraceBaseApp(std::string_view name, const Settings &settings, std::vector<std::unique_ptr<Plugin>> plugins)
    : VulkanBaseApp(name, settings, std::move(plugins))
{
    // Add raytracing device extensions
    deviceExtensions.push_back(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
    deviceExtensions.push_back(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);
    deviceExtensions.push_back(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);
    deviceExtensions.push_back(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
    deviceExtensions.push_back(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);
    deviceExtensions.push_back(VK_KHR_SPIRV_1_4_EXTENSION_NAME);
    deviceExtensions.push_back(VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME);

    // Enable features required for ray tracing using feature chaining via pNext
    enabledBufferDeviceAddressFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
    enabledBufferDeviceAddressFeatures.bufferDeviceAddress = VK_TRUE;

    enabledRayTracingPipelineFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
    enabledRayTracingPipelineFeatures.rayTracingPipeline = VK_TRUE;
    enabledRayTracingPipelineFeatures.pNext = &enabledBufferDeviceAddressFeatures;

    enabledAccelerationStructureFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
    enabledAccelerationStructureFeatures.accelerationStructure = VK_TRUE;
    enabledAccelerationStructureFeatures.pNext = &enabledRayTracingPipelineFeatures;

    enabledDescriptorIndexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
    enabledDescriptorIndexingFeatures.pNext = &enabledAccelerationStructureFeatures;
    enabledDescriptorIndexingFeatures.runtimeDescriptorArray = VK_TRUE;

    deviceCreateNextChain = &enabledDescriptorIndexingFeatures;
}

void VulkanRayTraceBaseApp::postVulkanInit() {
    loadRayTracingProperties();
    rtBuilder = rt::AccelerationStructureBuilder{&device};
    canvas = std::move(Canvas{this, VK_IMAGE_USAGE_STORAGE_BIT});
    canvas.init();
}


void VulkanRayTraceBaseApp::loadRayTracingProperties() {
    rayTracingPipelineProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
    VkPhysicalDeviceProperties2 properties{};
    properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    properties.pNext = &rayTracingPipelineProperties;

    vkGetPhysicalDeviceProperties2(device, &properties);
}

void VulkanRayTraceBaseApp::createAccelerationStructure(const std::vector<VulkanDrawableInstance>& drawableInstances) {
    if(drawableInstances.empty()) return;

    auto res = rtBuilder.buildAs(drawableInstances);
    sceneObjects = std::move(std::get<0>(res));
    asInstances = std::move(std::get<1>(res));
    VkDeviceSize size = sizeof(rt::ObjectInstance);
    auto stagingBuffer = device.createBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY, size * sceneObjects.size());

    std::vector<rt::ObjectInstance> sceneDesc;
    sceneDesc.reserve(sceneObjects.size());
    for(auto& instanceGroup : sceneObjects){
        for(auto& instance : instanceGroup.objectInstances){
            sceneDesc.push_back(instance);
        }
    }
    sceneObjectBuffer = device.createDeviceLocalBuffer(sceneDesc.data(), size * sceneDesc.size(), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
}

void
VulkanRayTraceBaseApp::createShaderBindingTable(ShaderBindingTable &shaderBindingTable, void *shaderHandleStoragePtr,
                                                VkBufferUsageFlags usageFlags, VmaMemoryUsage memoryUsage,
                                                uint32_t handleCount) {
    const auto [handleSize, _] = getShaderGroupHandleSizingInfo();

    VkDeviceSize size = handleSize * handleCount;
    auto stagingBuffer = device.createStagingBuffer(size);
    stagingBuffer.copy(shaderHandleStoragePtr, size);

    shaderBindingTable.buffer = device.createBuffer(usageFlags | VK_BUFFER_USAGE_TRANSFER_DST_BIT, memoryUsage, size);
    device.copy(stagingBuffer, shaderBindingTable.buffer, size, 0, 0);

    shaderBindingTable.stridedDeviceAddressRegion = getSbtEntryStridedDeviceAddressRegion(shaderBindingTable.buffer, handleCount);

}

VkStridedDeviceAddressRegionKHR
VulkanRayTraceBaseApp::getSbtEntryStridedDeviceAddressRegion(const VulkanBuffer &buffer, uint32_t handleCount) const {
    const auto [_, handleSizeAligned] = getShaderGroupHandleSizingInfo();
    VkStridedDeviceAddressRegionKHR stridedDeviceAddressRegion{};
    stridedDeviceAddressRegion.deviceAddress = device.getAddress(buffer);
    stridedDeviceAddressRegion.stride = handleSizeAligned;
    stridedDeviceAddressRegion.size = handleSizeAligned * handleCount;

    return stridedDeviceAddressRegion;
}

std::tuple<uint32_t, uint32_t> VulkanRayTraceBaseApp::getShaderGroupHandleSizingInfo() const {
    const uint32_t handleSize = rayTracingPipelineProperties.shaderGroupHandleSize;
    const uint32_t handleSizeAligned = alignedSize(handleSize, rayTracingPipelineProperties.shaderGroupHandleAlignment);

    return std::make_tuple(handleSize, handleSizeAligned);
}