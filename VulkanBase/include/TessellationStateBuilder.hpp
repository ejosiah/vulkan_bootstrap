#pragma once

#include "GraphicsPipelineBuilder.hpp"

class TessellationStateBuilder :  public GraphicsPipelineBuilder{
public:
    TessellationStateBuilder(VulkanDevice* device, GraphicsPipelineBuilder* parent);

    TessellationStateBuilder& patchControlPoints(uint32_t count);

    TessellationStateBuilder& domainOrigin(VkTessellationDomainOrigin origin);

    GraphicsPipelineBuilder& clear();

    VkPipelineTessellationStateCreateInfo& buildTessellationState();


private:
    VkPipelineTessellationStateCreateInfo _info{VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO};
    VkPipelineTessellationDomainOriginStateCreateInfo originStateInfo{VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_DOMAIN_ORIGIN_STATE_CREATE_INFO};
};