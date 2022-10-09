#pragma once

#include "VulkanDevice.h"
#include "VulkanInstance.h"
#include "VulkanExtensions.h"
#include "VulkanDevice.h"
#include "filemanager.hpp"
#include <memory>
#include <vector>

struct ContextCreateInfo{
    VkApplicationInfo applicationInfo{};
    ExtensionsAndValidationLayers instanceExtAndLayers{
            {VK_EXT_DEBUG_UTILS_EXTENSION_NAME}
    };
    ExtensionsAndValidationLayers deviceExtAndLayers;
    Settings settings;
    VkSurfaceKHR surface{ VK_NULL_HANDLE };
    VkPhysicalDeviceFeatures enabledFeature{};
    void* deviceCreateNextChain{ nullptr };
    std::vector<fs::path>  searchPaths{};
};


class VulkanContext{
public:
    VulkanContext() = default;

    VulkanContext(ContextCreateInfo createInfo);

    ~VulkanContext();

    VulkanInstance instance{};
    VulkanDevice device{};
    VulkanExtensions extensions{};
    VulkanDebug vulkanDebug{};
    FileManager fileManager;

    void init();

    std::string resource(const std::string& path);

private:
    class Impl;
    Impl* pimpl{ nullptr };

};