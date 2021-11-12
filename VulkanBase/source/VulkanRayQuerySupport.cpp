#include "VulkanRayQuerySupport.hpp"

void VulkanRayQuerySupport::enableRayQuery() {
    auto self = dynamic_cast<VulkanBaseApp*>(this);
    assert(self);
    auto& app = *self;

    if(!app.device.extensionSupported(VK_KHR_RAY_QUERY_EXTENSION_NAME)){
        throw std::runtime_error{ "Ray query not supported by your device" };
    }

    app.deviceExtensions.push_back(VK_KHR_RAY_QUERY_EXTENSION_NAME);
    app.deviceExtensions.push_back(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
    app.deviceExtensions.push_back(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
    app.deviceExtensions.push_back(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);

    bufferDeviceAddressFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
    bufferDeviceAddressFeatures.bufferDeviceAddress = VK_TRUE;
    bufferDeviceAddressFeatures.pNext = app.deviceCreateNextChain;

    accelerationStructureFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
    accelerationStructureFeatures.accelerationStructure = VK_TRUE;
    accelerationStructureFeatures.pNext = &bufferDeviceAddressFeatures;

    rayQueryFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR;
    rayQueryFeatures.rayQuery = VK_TRUE;
    rayQueryFeatures.pNext = &accelerationStructureFeatures;

    app.deviceCreateNextChain = &rayQueryFeatures;
}
