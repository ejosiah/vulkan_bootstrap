#pragma once

#include "GraphicsPipelineBuilder.hpp"

class DynamicStateBuilder : public GraphicsPipelineBuilder{
public:
    DynamicStateBuilder(VulkanDevice* device, GraphicsPipelineBuilder* parent);

    DynamicStateBuilder& viewport();

    DynamicStateBuilder& scissor();

    DynamicStateBuilder& lineWidth();

    DynamicStateBuilder& depthBias();

    DynamicStateBuilder& blendConstants();

    DynamicStateBuilder& depthBounds();

    DynamicStateBuilder& stencilCompareMask();

    DynamicStateBuilder& stencilWriteMask();

    DynamicStateBuilder& stencilReferenceMask();

    VkPipelineDynamicStateCreateInfo& buildPipelineDynamicState();

private:
    std::vector<VkDynamicState> _dynamicStates;
    VkPipelineDynamicStateCreateInfo _info;
};