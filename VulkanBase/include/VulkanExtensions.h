#pragma once

#include <vulkan/vulkan.h>
#include <stdexcept>

template<typename FuncType>
inline FuncType procAddress(VkInstance instance, const char* procName){
    auto func =  reinterpret_cast<FuncType>(vkGetInstanceProcAddr(instance, procName));
    if(!func) throw std::runtime_error("unable to find prod address: " + std::string(procName));
    return func;
}

template<typename FuncType>
inline FuncType procAddress(const char* procName){
    auto func =  reinterpret_cast<FuncType>(vkGetInstanceProcAddr(nullptr, procName));
    if(!func) throw std::runtime_error("unable to find prod address: " + std::string(procName));
    return func;
}

struct VulkanExtensions{

    VulkanExtensions() = default;

    VulkanExtensions(VkInstance instance)
    : instance(instance)
    , createDebugUtilsMessenger(procAddress<PFN_vkCreateDebugUtilsMessengerEXT>(instance,"vkCreateDebugUtilsMessengerEXT"))
    , destroyDebugUtilsMessenger(procAddress<PFN_vkDestroyDebugUtilsMessengerEXT>(instance,"vkDestroyDebugUtilsMessengerEXT"))
    , vkGetAccelerationStructureBuildSizesKHR(procAddress<PFN_vkGetAccelerationStructureBuildSizesKHR>(instance,"vkGetAccelerationStructureBuildSizesKHR"))
    , vkCreateAccelerationStructureKHR(procAddress<PFN_vkCreateAccelerationStructureKHR>(instance,"vkCreateAccelerationStructureKHR"))
    , vkDestroyAccelerationStructureKHR(procAddress<PFN_vkDestroyAccelerationStructureKHR>(instance,"vkDestroyAccelerationStructureKHR"))
    , vkCmdBuildAccelerationStructuresKHR(procAddress<PFN_vkCmdBuildAccelerationStructuresKHR>(instance,"vkCmdBuildAccelerationStructuresKHR"))
    , vkGetAccelerationStructureDeviceAddressKHR(procAddress<PFN_vkGetAccelerationStructureDeviceAddressKHR>(instance,"vkGetAccelerationStructureDeviceAddressKHR"))
    , vkGetRayTracingShaderGroupHandlesKHR(procAddress<PFN_vkGetRayTracingShaderGroupHandlesKHR>(instance,"vkGetRayTracingShaderGroupHandlesKHR"))
    , vkCmdTraceRaysKHR(procAddress<PFN_vkCmdTraceRaysKHR>(instance,"vkCmdTraceRaysKHR"))
    , vkCreateRayTracingPipelinesKHR(procAddress<PFN_vkCreateRayTracingPipelinesKHR>(instance,"vkCreateRayTracingPipelinesKHR"))
    , vkSetDebugUtilsObjectNameEXT(procAddress<PFN_vkSetDebugUtilsObjectNameEXT>(instance,"vkSetDebugUtilsObjectNameEXT"))
    {
        ext = this;
    }

    VkInstance instance;
    PFN_vkCreateDebugUtilsMessengerEXT  createDebugUtilsMessenger;
    PFN_vkDestroyDebugUtilsMessengerEXT destroyDebugUtilsMessenger;
    PFN_vkGetAccelerationStructureBuildSizesKHR vkGetAccelerationStructureBuildSizesKHR;
    PFN_vkCreateAccelerationStructureKHR vkCreateAccelerationStructureKHR;
    PFN_vkDestroyAccelerationStructureKHR vkDestroyAccelerationStructureKHR;
    PFN_vkCmdBuildAccelerationStructuresKHR vkCmdBuildAccelerationStructuresKHR;
    PFN_vkGetAccelerationStructureDeviceAddressKHR vkGetAccelerationStructureDeviceAddressKHR;
    PFN_vkGetRayTracingShaderGroupHandlesKHR vkGetRayTracingShaderGroupHandlesKHR;
    PFN_vkCreateRayTracingPipelinesKHR vkCreateRayTracingPipelinesKHR;
    PFN_vkCmdTraceRaysKHR vkCmdTraceRaysKHR;
    PFN_vkSetDebugUtilsObjectNameEXT vkSetDebugUtilsObjectNameEXT;

    static VulkanExtensions* ext;
};

inline VulkanExtensions* VulkanExtensions::ext = nullptr;