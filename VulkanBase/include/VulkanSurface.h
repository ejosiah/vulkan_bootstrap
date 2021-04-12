#pragma once

#include <GLFW/glfw3.h>
#include "common.h"

struct VulkanSurface{

    VulkanSurface() = default;

    inline VulkanSurface(VkInstance instance, GLFWwindow* window)
    : instance(instance)
    {
        assert(instance && window);
        auto result = glfwCreateWindowSurface(instance, window, nullptr, &surface);
        if(result != VK_SUCCESS) throw std::runtime_error{"Failed to create Surface"};

    }

    VulkanSurface(const VulkanSurface&) = delete;

    VulkanSurface(VulkanSurface&& source) noexcept {
        operator=(static_cast<VulkanSurface&&>(source));
    }

    VulkanSurface& operator=(const VulkanSurface&) = delete;

    VulkanSurface& operator=(VulkanSurface&& source) noexcept {
        instance = source.instance;
        surface = source.surface;

        source.surface = VK_NULL_HANDLE;
        return *this;
    }

    ~VulkanSurface(){
        if(surface){
            vkDestroySurfaceKHR(instance, surface, nullptr);
        }
    }

    inline VkSurfaceCapabilitiesKHR getCapabilities(VkPhysicalDevice physicalDevice) const {
        VkSurfaceCapabilitiesKHR capabilities;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &capabilities);
        return capabilities;
    }

    inline std::vector<VkSurfaceFormatKHR> getFormats(VkPhysicalDevice physicalDevice) const {
        return get<VkSurfaceFormatKHR>([&](uint32_t *size, VkSurfaceFormatKHR *formats) {
            vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, size, formats);
        });
    }

    inline std::vector<VkPresentModeKHR> getPresentModes(VkPhysicalDevice physicalDevice) const {
        return get<VkPresentModeKHR>([&](uint32_t *size, VkPresentModeKHR *presentMode) {
            vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, size, presentMode);
        });
    }

    operator VkSurfaceKHR() const {
        return surface;
    }

    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkInstance instance = VK_NULL_HANDLE;
};