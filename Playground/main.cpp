#include <fmt/format.h>
#include <VulkanInstance.h>
#include <VulkanDevice.h>
#include <VulkanDebug.h>

int main(){
    ExtensionsAndValidationLayers ext;
    ext.extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    ext.extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
    ext.validationLayers.push_back("VK_LAYER_KHRONOS_validation");
    auto appInfo = initializers::AppInfo();
    VulkanInstance instance{appInfo, ext};
    VulkanDebug debug{instance};

    VkPhysicalDevice physicalDevice;
    uint32_t count = 1;
    vkEnumeratePhysicalDevices(instance, &count, &physicalDevice);

    VkPhysicalDeviceAccelerationStructureFeaturesKHR accFeatures{};
    accFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
    VkPhysicalDeviceRayTracingPipelineFeaturesKHR rtFeatures{};
    rtFeatures.pNext = &accFeatures;
    rtFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
    VkPhysicalDeviceFeatures2 features{};
    features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    features.pNext = &rtFeatures;

    vkGetPhysicalDeviceFeatures2(physicalDevice, &features);


    fmt::print("ray tracing features:\n");
    fmt::print("\tray tracing pipeline: {}\n", rtFeatures.rayTracingPipeline == VK_TRUE ? "supported" : "not supported");
    fmt::print("\tray tracing pipeline shader group handle capture replay: {}\n", rtFeatures.rayTracingPipelineShaderGroupHandleCaptureReplay ? "supported" : "not supported");
    fmt::print("\tray tracing pipeline shader group handle capture replay mixed: {}\n", rtFeatures.rayTracingPipelineShaderGroupHandleCaptureReplayMixed ? "supported" : "not supported");
    fmt::print("\tray tracing pipeline trace ray indirect: {}\n", rtFeatures.rayTracingPipelineTraceRaysIndirect ? "supported" : "not supported");
    fmt::print("\tray Traversal Primitive culling: {}\n\n", rtFeatures.rayTraversalPrimitiveCulling ? "supported" : "not supported");

    fmt::print("Acceleration Structure features:\n");
    fmt::print("\thost commands: {}\n", accFeatures.accelerationStructureHostCommands ? "supported" : "not supported");


    VkPhysicalDeviceAccelerationStructurePropertiesKHR asProperties{};
    asProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR;
    VkPhysicalDeviceRayTracingPipelinePropertiesKHR rtProperties{};
    rtProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
    rtProperties.pNext = &asProperties;


    VkPhysicalDeviceProperties2 properties{};
    properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    properties.pNext = &rtProperties;

    vkGetPhysicalDeviceProperties2(physicalDevice, &properties);

    fmt::print("Ray tracing pipeline properties:\n");
    fmt::print("\tshader group handle size: {}\n", rtProperties.shaderGroupHandleSize);
    fmt::print("\tmax ray recursion Depth: {}\n", rtProperties.maxRayRecursionDepth);
    fmt::print("\tmax shader group stride: {}\n", rtProperties.maxShaderGroupStride);
    fmt::print("\tshader group base alignment: {}\n", rtProperties.shaderGroupBaseAlignment);
    fmt::print("\tshader group handle capture reply size: {}\n", rtProperties.shaderGroupHandleCaptureReplaySize);
    fmt::print("\tmax ray dispatch invocation count: {}\n", rtProperties.maxRayDispatchInvocationCount);
    fmt::print("\tshader group handle alignment: {}\n", rtProperties.shaderGroupHandleAlignment);
    fmt::print("\tmax ray hit attribute size: {}\n\n", rtProperties.maxRayHitAttributeSize);


    fmt::print("Accelerated structure properties\n");
    fmt::print("\tmax Geometry count: {}\n", asProperties.maxGeometryCount);
    fmt::print("\tmax instance count: {}\n", asProperties.maxInstanceCount);
    fmt::print("\tmax primitive count: {}\n", asProperties.maxPrimitiveCount);
    fmt::print("\tmax per stage descriptor acceleration structures: {}\n", asProperties.maxPerStageDescriptorAccelerationStructures);
    fmt::print("\tmax per stage descriptor update after bind acceleration structures: {}\n", asProperties.maxPerStageDescriptorUpdateAfterBindAccelerationStructures);
    fmt::print("\tmax descriptor set acceleration structures: {}\n", asProperties.maxDescriptorSetAccelerationStructures);
    fmt::print("\tmax descriptor set update after bind acceleration structures: {}\n", asProperties.maxDescriptorSetUpdateAfterBindAccelerationStructures);
    fmt::print("\tminimum acceleration structures scratch offset alignment: {}\n\n", asProperties.minAccelerationStructureScratchOffsetAlignment);
    return 0;
}