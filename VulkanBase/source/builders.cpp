#include "DescriptorSetBuilder.hpp"
#include "GraphisPipelineBuilder.hpp"

DescriptorSetLayoutBuilder VulkanDevice::descriptorSetLayoutBuilder() {
    return DescriptorSetLayoutBuilder{ *this };
}

GraphicsPipelineBuilder VulkanDevice::graphicsPipelineBuilder(){
    return GraphicsPipelineBuilder{ *this };
}