#include <cassert>
#include "VulkanExtensions.h"

static PFN_vkCmdTraceRaysKHR pfn_vkCmdTraceRaysKHR = nullptr;
static PFN_vkCreateDebugUtilsMessengerEXT  pfn_createDebugUtilsMessenger = nullptr;
static PFN_vkDestroyDebugUtilsMessengerEXT pfn_destroyDebugUtilsMessenger = nullptr;
static PFN_vkGetAccelerationStructureBuildSizesKHR pfn_vkGetAccelerationStructureBuildSizesKHR = nullptr;
static PFN_vkCreateAccelerationStructureKHR pfn_vkCreateAccelerationStructureKHR = nullptr;
static PFN_vkDestroyAccelerationStructureKHR pfn_vkDestroyAccelerationStructureKHR = nullptr;
static PFN_vkCmdBuildAccelerationStructuresKHR pfn_vkCmdBuildAccelerationStructuresKHR = nullptr;
static PFN_vkGetAccelerationStructureDeviceAddressKHR pfn_vkGetAccelerationStructureDeviceAddressKHR = nullptr;
static PFN_vkGetRayTracingShaderGroupHandlesKHR pfn_vkGetRayTracingShaderGroupHandlesKHR = nullptr;
static PFN_vkCreateRayTracingPipelinesKHR pfn_vkCreateRayTracingPipelinesKHR = nullptr;
static PFN_vkSetDebugUtilsObjectNameEXT pfn_vkSetDebugUtilsObjectNameEXT = nullptr;



namespace ext {

    void init(VkInstance instance){
        pfn_createDebugUtilsMessenger = procAddress<PFN_vkCreateDebugUtilsMessengerEXT>(instance, "vkCreateDebugUtilsMessengerEXT");
        pfn_destroyDebugUtilsMessenger = procAddress<PFN_vkDestroyDebugUtilsMessengerEXT>(instance, "vkDestroyDebugUtilsMessengerEXT");
        pfn_vkCmdTraceRaysKHR = procAddress<PFN_vkCmdTraceRaysKHR>(instance, "vkCmdTraceRaysKHR");
        pfn_vkGetAccelerationStructureBuildSizesKHR = procAddress<PFN_vkGetAccelerationStructureBuildSizesKHR>(instance, "vkGetAccelerationStructureBuildSizesKHR");
        pfn_vkCreateAccelerationStructureKHR = procAddress<PFN_vkCreateAccelerationStructureKHR>(instance, "vkCreateAccelerationStructureKHR");
        pfn_vkCmdBuildAccelerationStructuresKHR = procAddress<PFN_vkCmdBuildAccelerationStructuresKHR>(instance, "vkCmdBuildAccelerationStructuresKHR");
        pfn_vkGetAccelerationStructureDeviceAddressKHR = procAddress<PFN_vkGetAccelerationStructureDeviceAddressKHR>(instance, "vkGetAccelerationStructureDeviceAddressKHR");
        pfn_vkDestroyAccelerationStructureKHR = procAddress<PFN_vkDestroyAccelerationStructureKHR>(instance, "vkDestroyAccelerationStructureKHR");
    }

}

VKAPI_ATTR void VKAPI_CALL  vkCmdTraceRaysKHR(VkCommandBuffer commandBuffer, const VkStridedDeviceAddressRegionKHR* pRaygenShaderBindingTable, const VkStridedDeviceAddressRegionKHR* pMissShaderBindingTable, const VkStridedDeviceAddressRegionKHR* pHitShaderBindingTable, const VkStridedDeviceAddressRegionKHR* pCallableShaderBindingTable, uint32_t width, uint32_t height, uint32_t depth){
    assert(pfn_vkCmdTraceRaysKHR != nullptr);
    pfn_vkCmdTraceRaysKHR(commandBuffer, pRaygenShaderBindingTable, pMissShaderBindingTable, pHitShaderBindingTable, pCallableShaderBindingTable, width, height, depth);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateDebugUtilsMessengerEXT(
        VkInstance                                  instance,
        const VkDebugUtilsMessengerCreateInfoEXT*   pCreateInfo,
        const VkAllocationCallbacks*                pAllocator,
        VkDebugUtilsMessengerEXT*                   pMessenger){
    assert(pfn_createDebugUtilsMessenger);
    return pfn_createDebugUtilsMessenger(instance, pCreateInfo, pAllocator, pMessenger);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyDebugUtilsMessengerEXT(
        VkInstance                                  instance,
        VkDebugUtilsMessengerEXT                    messenger,
        const VkAllocationCallbacks*                pAllocator){
    assert(pfn_destroyDebugUtilsMessenger);
    return pfn_destroyDebugUtilsMessenger(instance, messenger, pAllocator);
}

VKAPI_ATTR void VKAPI_CALL vkGetAccelerationStructureBuildSizesKHR(
        VkDevice                                    device,
        VkAccelerationStructureBuildTypeKHR         buildType,
        const VkAccelerationStructureBuildGeometryInfoKHR* pBuildInfo,
        const uint32_t*                             pMaxPrimitiveCounts,
        VkAccelerationStructureBuildSizesInfoKHR*   pSizeInfo){
    assert(pfn_vkGetAccelerationStructureBuildSizesKHR);
    pfn_vkGetAccelerationStructureBuildSizesKHR(device, buildType, pBuildInfo, pMaxPrimitiveCounts, pSizeInfo);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateAccelerationStructureKHR(
        VkDevice                                    device,
        const VkAccelerationStructureCreateInfoKHR* pCreateInfo,
        const VkAllocationCallbacks*                pAllocator,
        VkAccelerationStructureKHR*                 pAccelerationStructure){
    assert(pfn_vkCreateAccelerationStructureKHR);
    return pfn_vkCreateAccelerationStructureKHR(device, pCreateInfo, pAllocator, pAccelerationStructure);
}

VKAPI_ATTR void VKAPI_CALL vkCmdBuildAccelerationStructuresKHR(
        VkCommandBuffer                             commandBuffer,
        uint32_t                                    infoCount,
        const VkAccelerationStructureBuildGeometryInfoKHR* pInfos,
        const VkAccelerationStructureBuildRangeInfoKHR* const* ppBuildRangeInfos) {
    assert(pfn_vkCmdBuildAccelerationStructuresKHR);
    pfn_vkCmdBuildAccelerationStructuresKHR(commandBuffer, infoCount, pInfos, ppBuildRangeInfos);
}

VKAPI_ATTR VkDeviceAddress VKAPI_CALL vkGetAccelerationStructureDeviceAddressKHR(
        VkDevice                                    device,
        const VkAccelerationStructureDeviceAddressInfoKHR* pInfo){
        return pfn_vkGetAccelerationStructureDeviceAddressKHR(device, pInfo);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyAccelerationStructureKHR(
        VkDevice                                    device,
        VkAccelerationStructureKHR                  accelerationStructure,
        const VkAllocationCallbacks*                pAllocator){
    assert(pfn_vkDestroyAccelerationStructureKHR);
    pfn_vkDestroyAccelerationStructureKHR(device, accelerationStructure, pAllocator);
}