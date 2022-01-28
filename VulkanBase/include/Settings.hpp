#pragma once

#include <vulkan/vulkan.h>

/**
 * @file Settings.hpp
 * @breif Contains application startup settings
 */
struct Settings{
    /**
     * sets if application should run in fullscreen or windowed mode
     */
    bool fullscreen = false;

    struct {
        int width = 0;
        int height = 0;
        int refreshRate = 0;
    } videoMode;

    /**
     * mouse will be recentered if set
     */
    bool relativeMouseMode = false;

    /**
     * enables/disabbles depth testing
     */
    bool depthTest = false;

    /**
     * sets if draw calls should be synchronized with minotrs
     * refresh rate
     */
    bool vSync = false;

    /**
     * @brief surface format settings for swapchain
     */
    VkSurfaceFormatKHR surfaceFormat{
            VK_FORMAT_B8G8R8A8_UNORM,
            VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
    };

    uint32_t width = 1280;
    uint32_t height = 720;

    VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;

    VkQueueFlags queueFlags = VK_QUEUE_GRAPHICS_BIT;

    VkQueueFlags uniqueQueueFlags = 0;

    /**
     * sets Vulkan features to enable
     */
    VkPhysicalDeviceFeatures enabledFeatures{};

    std::vector<const char*> instanceExtensions;
    std::vector<const char*> validationLayers;
    std::vector<const char*> deviceExtensions;
};