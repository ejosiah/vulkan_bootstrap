#pragma once

#include "common.h"
#include "VulkanDevice.h"
#include "VulkanSurface.h"

struct VulkanSwapChain{

    VulkanSwapChain() = default;

    inline VulkanSwapChain(const VulkanDevice& device, const VulkanSurface& surface, uint32_t width, uint32_t height,  VkSwapchainKHR oldSwapChain = VK_NULL_HANDLE){
        auto capabilities = surface.getCapabilities(device);
        auto formats = surface.getFormats(device);
        auto presentModes = surface.getPresentModes(device);

      //  extent = capabilities
        VkSurfaceFormatKHR surfaceFormat = choose(formats);
        auto presentMode = choose(presentModes);
        auto extent = chooseExtent(capabilities, {width, height});

        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = surface;
        createInfo.minImageCount = std::min(capabilities.minImageCount + 1, capabilities.maxImageCount);
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        if(device.queueFamilyIndex.graphics == device.queueFamilyIndex.present) {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }else{
            std::vector<uint32_t> indices{ *device.queueFamilyIndex.graphics, *device.queueFamilyIndex.present };
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = indices.data();
        }
        createInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentMode;
        createInfo.clipped = true;
        createInfo.oldSwapchain = oldSwapChain;

        ASSERT(vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain));

        this->extent = extent;
        this->format = surfaceFormat.format;
        this->device = device;
        this->images = get<VkImage>([&](uint32_t* count, VkImage* ptr){ vkGetSwapchainImagesKHR(device, swapChain, count, ptr); });
        createImageViews();

    }

    VulkanSwapChain(const VulkanSwapChain&) = delete;

    VulkanSwapChain(VulkanSwapChain&& source) noexcept {
        operator=(static_cast<VulkanSwapChain&&>(source));
    }

    VulkanSwapChain& operator=(const VulkanSwapChain&) = delete;

    VulkanSwapChain& operator=(VulkanSwapChain&& source) noexcept {
        this->swapChain = source.swapChain;
        this->format = source.format;
        this->extent = source.extent;
        this->device = source.device;
        this->images = std::move(source.images);
        this->imageViews = std::move(source.imageViews);

        source.swapChain = VK_NULL_HANDLE;

        return *this;
    }

    ~VulkanSwapChain(){
        if(swapChain){
            vkDestroySwapchainKHR(device, swapChain, nullptr);
            for(auto& imageView : imageViews){
                vkDestroyImageView(device, imageView, nullptr);
            }
        }
    }

    inline VkSurfaceFormatKHR choose(const std::vector<VkSurfaceFormatKHR>& formats) {
        auto itr = std::find_if(begin(formats), end(formats), [](const auto& fmt){
           return fmt.format == VK_FORMAT_B8G8R8A8_SRGB && fmt.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
        });
        return itr != end(formats) ?  *itr : formats.front();
    }

    inline VkPresentModeKHR choose(const std::vector<VkPresentModeKHR>& presentModes) {
        auto itr = std::find_if(begin(presentModes), end(presentModes), [](const auto& presentMode){
           return presentMode == VK_PRESENT_MODE_MAILBOX_KHR;
        });
        return itr != end(presentModes) ? *itr : VK_PRESENT_MODE_FIFO_KHR;
    }

    inline VkExtent2D chooseExtent(const VkSurfaceCapabilitiesKHR& capabilities, VkExtent2D actualExtent){
        if(capabilities.currentExtent.width != UINT32_MAX){
            return capabilities.currentExtent;
        }else{
            return {
                    std::clamp(actualExtent.width,
                               capabilities.minImageExtent.width,
                               capabilities.maxImageExtent.width),
                    std::clamp(actualExtent.height,
                               capabilities.minImageExtent.height,
                               capabilities.maxImageExtent.height)
            };
        }
    }

    inline void createImageViews(){
        imageViews.resize(images.size());
        for(int i = 0; i < images.size(); i++){
            VkImageViewCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.image = images[i];
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            createInfo.format = format;
            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.layerCount = 1;
            createInfo.subresourceRange.levelCount = 1;

            ASSERT(vkCreateImageView(device, &createInfo, nullptr, &imageViews[i]));
        }
    }

    [[nodiscard]]
    uint32_t imageCount() const{
        return static_cast<uint32_t>(images.size());
    }

    operator VkSwapchainKHR() const {
        return swapChain;
    }

    VkSwapchainKHR swapChain = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkFormat format = VK_FORMAT_UNDEFINED;
    VkExtent2D extent{0, 0};
    std::vector<VkImage> images;
    std::vector<VkImageView> imageViews;
};