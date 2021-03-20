#pragma once

#include "common.h"

struct VulkanPipelineLayout{

    VulkanPipelineLayout() = default;

    explicit VulkanPipelineLayout(  VkDevice device
                         , const std::vector<VkDescriptorSetLayout>& layouts = {}
                         , const std::vector<VkPushConstantRange>& ranges = {})
    :device(device)
    {
        VkPipelineLayoutCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        createInfo.setLayoutCount = COUNT(layouts);
        createInfo.pSetLayouts = layouts.data();
        createInfo.pushConstantRangeCount = COUNT(ranges);
        createInfo.pPushConstantRanges = ranges.data();

        ASSERT(vkCreatePipelineLayout(device, &createInfo, nullptr, &pipelineLayout));
    }

    VulkanPipelineLayout(const VulkanPipelineLayout&) = delete;

    VulkanPipelineLayout(VulkanPipelineLayout&& source) noexcept {
        operator=(static_cast<VulkanPipelineLayout&&>(source));
    }

    ~VulkanPipelineLayout(){
        if(pipelineLayout){
            vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
        }
    }

    VulkanPipelineLayout& operator=(const VulkanPipelineLayout&) = delete;

    VulkanPipelineLayout& operator=(VulkanPipelineLayout&& source) noexcept {
        if(this == &source) return *this;
        this->pipelineLayout = source.pipelineLayout;
        this->device = source.device;

        source.pipelineLayout = VK_NULL_HANDLE;
        return *this;
    }

    operator VkPipelineLayout(){
        return pipelineLayout;
    }

    VkDevice device = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
};