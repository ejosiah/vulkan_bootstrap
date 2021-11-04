#pragma once

#include "GraphicsPipelineBuilder.hpp"

class MultisampleStateBuilder : public GraphicsPipelineBuilder{
public:
    MultisampleStateBuilder(VulkanDevice* device, GraphicsPipelineBuilder* parent);

    VkPipelineMultisampleStateCreateInfo& buildMultisampleState();

private:
    VkPipelineMultisampleStateCreateInfo _info;
};