#include <vulkan/vulkan.h>
#include <stdexcept>

#pragma once

template<typename FuncType>
FuncType procAddress(VkInstance instance, const char* procName){
    auto func =  reinterpret_cast<FuncType>(vkGetInstanceProcAddr(instance, procName));
    if(!func) throw std::runtime_error("unable to find prod address: " + std::string(procName));
    return func;
}

template<typename FuncType>
FuncType procAddress(const char* procName){
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
    {
    }

    VkInstance instance;
    PFN_vkCreateDebugUtilsMessengerEXT  createDebugUtilsMessenger;
    PFN_vkDestroyDebugUtilsMessengerEXT destroyDebugUtilsMessenger;
};