#include "VulkanDevice.h"
#include "GraphicsPipelineBuilder.hpp"

GraphicsPipelineBuilder VulkanDevice::graphicsPipelineBuilder() const {
    return GraphicsPipelineBuilder{ const_cast<VulkanDevice*>(this) };  // FIXME const everything
}
