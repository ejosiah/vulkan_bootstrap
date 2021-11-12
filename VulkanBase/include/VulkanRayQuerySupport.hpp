#pragma once
#include "common.h"
#include "VulkanBaseApp.h"

class VulkanRayQuerySupport{
public:
    void enableRayQuery();

protected:
    virtual void foo(){}    // needed to make polymorphic

private:
    VkPhysicalDeviceBufferDeviceAddressFeatures bufferDeviceAddressFeatures{};
    VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures{};
    VkPhysicalDeviceRayQueryFeaturesKHR  rayQueryFeatures{};
};