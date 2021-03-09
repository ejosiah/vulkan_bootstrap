#pragma once

#include "common.h"

struct VulkanFramebuffer{

    VulkanFramebuffer() = default;

    VulkanFramebuffer(VkDevice device, VkRenderPass renderPass
                      , const std::vector<VkImageView>& attachments
                      , uint32_t width, uint32_t height, uint32_t layers = 1)
    :device(device)
    {
        VkFramebufferCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        createInfo.renderPass = renderPass;
        createInfo.attachmentCount = COUNT(attachments);
        createInfo.pAttachments = attachments.data();
        createInfo.width = width;
        createInfo.height = height;
        createInfo.layers = layers;

        ASSERT(vkCreateFramebuffer(device, &createInfo, nullptr, &frameBuffer));
    }

    VulkanFramebuffer(const VulkanFramebuffer&) = delete;

    VulkanFramebuffer(VulkanFramebuffer&& source) noexcept {
        operator=(static_cast<VulkanFramebuffer&&>(source));
    }

    ~VulkanFramebuffer(){
        if(frameBuffer){
            vkDestroyFramebuffer(device, frameBuffer, nullptr);
        }
    }

    VulkanFramebuffer& operator=(const VulkanFramebuffer&) = delete;

    VulkanFramebuffer& operator=(VulkanFramebuffer&& source) noexcept {
        if(this == &source) return *this;
        this->device = source.device;
        this->frameBuffer = source.frameBuffer;

        source.frameBuffer = VK_NULL_HANDLE;

        return *this;
    }

    operator VkFramebuffer() const {
        return frameBuffer;
    }

    VkDevice device = VK_NULL_HANDLE;
    VkFramebuffer frameBuffer = VK_NULL_HANDLE;
};