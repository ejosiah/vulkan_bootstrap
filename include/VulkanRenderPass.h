#pragma once

#include "common.h"

struct VulkanRenderPass{

    VulkanRenderPass() = default;

    VulkanRenderPass(  VkDevice device
                     , std::vector<VkAttachmentDescription> attachmentDescriptions
                     , std::vector<VkSubpassDescription> subpassDescriptions
                     , std::vector<VkSubpassDependency> dependencies = {})
    :device(device)
    {
        VkRenderPassCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        createInfo.attachmentCount = COUNT(attachmentDescriptions);
        createInfo.pAttachments = attachmentDescriptions.data();
        createInfo.subpassCount = COUNT(subpassDescriptions);
        createInfo.pSubpasses = subpassDescriptions.data();
        createInfo.dependencyCount = COUNT(dependencies);
        createInfo.pDependencies = dependencies.data();

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

