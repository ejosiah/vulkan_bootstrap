#pragma once

#include "common.h"
#include "VulkanInitializers.h"

struct VulkanRenderPass{

    VulkanRenderPass() = default;

    VulkanRenderPass(  VkDevice device
                     , const std::vector<VkAttachmentDescription>& attachmentDescriptions
                     , const std::vector<VkSubpassDescription>& subpassDescriptions
                     , const std::vector<VkSubpassDependency>& dependencies = {})
    :device(device)
    {
        VkRenderPassCreateInfo createInfo = initializers::renderPassCreateInfo(attachmentDescriptions,
                                                                               subpassDescriptions,
                                                                               dependencies);

        ASSERT(vkCreateRenderPass(device, &createInfo, nullptr, &renderPass));
    }

    VulkanRenderPass(const VulkanRenderPass&) = delete;

    VulkanRenderPass(VulkanRenderPass&& source) noexcept {
        operator=(static_cast<VulkanRenderPass&&>(source));
    }

    ~VulkanRenderPass(){
        if(renderPass){
            vkDestroyRenderPass(device, renderPass, nullptr);
        }
    }

    VulkanRenderPass& operator=(const VulkanRenderPass&) = delete;

    VulkanRenderPass& operator=(VulkanRenderPass&& source) noexcept {
        if(this == &source) return *this;
        this->device = source.device;
        this->renderPass = source.renderPass;

        source.renderPass = VK_NULL_HANDLE;

        return *this;
    }

    operator VkRenderPass() const {
        return renderPass;
    }

    VkDevice device = VK_NULL_HANDLE;
    VkRenderPass renderPass = VK_NULL_HANDLE;
};

