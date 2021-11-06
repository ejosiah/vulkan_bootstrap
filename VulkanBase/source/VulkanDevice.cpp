#include "VulkanDevice.h"
#include "GraphicsPipelineBuilder.hpp"

GraphicsPipelineBuilder VulkanDevice::graphicsPipelineBuilder() {
    return GraphicsPipelineBuilder{ this };
}
