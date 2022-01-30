#pragma once

#include <vulkan/vulkan.h>
#include <stdexcept>
#include "common.h"

template<typename FuncType>
inline FuncType procAddress(VkInstance instance, const char* procName){
    auto func =  reinterpret_cast<FuncType>(vkGetInstanceProcAddr(instance, procName));
    if(!func) throw std::runtime_error(fmt::format("unable to find prod address: {}", procName));
    return func;
}

template<typename FuncType>
inline FuncType procAddress(const char* procName){
    auto func =  reinterpret_cast<FuncType>(vkGetInstanceProcAddr(nullptr, procName));
    if(!func) throw std::runtime_error(fmt::format("unable to find prod address: {}", procName));
    return func;
}

struct VulkanExtensions{
    // FIXME remove this file, after ext::init runs you can call the initialized Vulkan procs directly

    VulkanExtensions() = default;

    VulkanExtensions(VkInstance instance)
    : instance(instance)
    , vkGetAccelerationStructureBuildSizesKHR(procAddress<PFN_vkGetAccelerationStructureBuildSizesKHR>(instance,"vkGetAccelerationStructureBuildSizesKHR"))
    , vkCreateAccelerationStructureKHR(procAddress<PFN_vkCreateAccelerationStructureKHR>(instance,"vkCreateAccelerationStructureKHR"))
    , vkDestroyAccelerationStructureKHR(procAddress<PFN_vkDestroyAccelerationStructureKHR>(instance,"vkDestroyAccelerationStructureKHR"))
    , vkCmdBuildAccelerationStructuresKHR(procAddress<PFN_vkCmdBuildAccelerationStructuresKHR>(instance,"vkCmdBuildAccelerationStructuresKHR"))
    , vkGetAccelerationStructureDeviceAddressKHR(procAddress<PFN_vkGetAccelerationStructureDeviceAddressKHR>(instance,"vkGetAccelerationStructureDeviceAddressKHR"))
    , vkGetRayTracingShaderGroupHandlesKHR(procAddress<PFN_vkGetRayTracingShaderGroupHandlesKHR>(instance,"vkGetRayTracingShaderGroupHandlesKHR"))
    , vkCmdTraceRaysKHR(procAddress<PFN_vkCmdTraceRaysKHR>(instance,"vkCmdTraceRaysKHR"))
    , vkCreateRayTracingPipelinesKHR(procAddress<PFN_vkCreateRayTracingPipelinesKHR>(instance,"vkCreateRayTracingPipelinesKHR"))
    {
        ext = this;
    }

    VkInstance instance;
    PFN_vkGetAccelerationStructureBuildSizesKHR vkGetAccelerationStructureBuildSizesKHR;
    PFN_vkCreateAccelerationStructureKHR vkCreateAccelerationStructureKHR;
    PFN_vkDestroyAccelerationStructureKHR vkDestroyAccelerationStructureKHR;
    PFN_vkCmdBuildAccelerationStructuresKHR vkCmdBuildAccelerationStructuresKHR;
    PFN_vkGetAccelerationStructureDeviceAddressKHR vkGetAccelerationStructureDeviceAddressKHR;
    PFN_vkGetRayTracingShaderGroupHandlesKHR vkGetRayTracingShaderGroupHandlesKHR;
    PFN_vkCreateRayTracingPipelinesKHR vkCreateRayTracingPipelinesKHR;
    PFN_vkCmdTraceRaysKHR vkCmdTraceRaysKHR;

    static VulkanExtensions* ext;
};

inline VulkanExtensions* VulkanExtensions::ext = nullptr;


namespace ext{

    extern void init(VkInstance instance);

}