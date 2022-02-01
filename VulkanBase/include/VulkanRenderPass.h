#pragma once

#include "common.h"
#include "VulkanInitializers.h"
#include "VulkanStructs.h"

struct VulkanRenderPass{

    VulkanRenderPass() = default;

    VulkanRenderPass(  VkDevice device
                     , const std::vector<VkAttachmentDescription>& attachmentDescriptions
                     , const std::vector<SubpassDescription>& subpassDescriptions
                     , const std::vector<VkSubpassDependency>& dependencies = {})
    :device(device)
    {
        std::vector<VkSubpassDescription> vkSubpassDescriptions;
        vkSubpassDescriptions.reserve(subpassDescriptions.size());
        for(auto& subpassDescription : subpassDescriptions){
            vkSubpassDescriptions.push_back(subpassDescription);
        }
        VkRenderPassCreateInfo createInfo = initializers::renderPassCreateInfo(attachmentDescriptions,
                                                                               vkSubpassDescriptions,
                                                                               dependencies);

        ERR_GUARD_VULKAN(vkCreateRenderPass(device, &createInfo, nullptr, &renderPass));
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

        this->~VulkanRenderPass();

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

