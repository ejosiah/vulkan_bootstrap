#include "DynamicStateBuilder.hpp"

DynamicStateBuilder::DynamicStateBuilder(VulkanDevice *device, GraphicsPipelineBuilder *parent)
: GraphicsPipelineBuilder(device, parent)
, _info{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO}
, _dynamicStates{}
{

}

DynamicStateBuilder &DynamicStateBuilder::viewport() {
    _dynamicStates.push_back(VK_DYNAMIC_STATE_VIEWPORT);
    return *this;
}

DynamicStateBuilder &DynamicStateBuilder::scissor() {
    _dynamicStates.push_back(VK_DYNAMIC_STATE_SCISSOR);
    return *this;
}

DynamicStateBuilder &DynamicStateBuilder::lineWidth() {
    _dynamicStates.push_back(VK_DYNAMIC_STATE_LINE_WIDTH);
    return *this;
}

DynamicStateBuilder &DynamicStateBuilder::depthBias() {
    _dynamicStates.push_back(VK_DYNAMIC_STATE_DEPTH_BIAS);
    return *this;
}

DynamicStateBuilder &DynamicStateBuilder::blendConstants() {
    _dynamicStates.push_back(VK_DYNAMIC_STATE_BLEND_CONSTANTS);
    return *this;
}

DynamicStateBuilder &DynamicStateBuilder::depthBounds() {
    _dynamicStates.push_back(VK_DYNAMIC_STATE_DEPTH_BOUNDS);
    return *this;
}

DynamicStateBuilder &DynamicStateBuilder::stencilCompareMask() {
    _dynamicStates.push_back(VK_DYNAMIC_STATE_STENCIL_COMPARE_MASK);
    return *this;
}

DynamicStateBuilder &DynamicStateBuilder::stencilWriteMask() {
    _dynamicStates.push_back(VK_DYNAMIC_STATE_STENCIL_WRITE_MASK);
    return *this;
}

DynamicStateBuilder &DynamicStateBuilder::stencilReferenceMask() {
    _dynamicStates.push_back(VK_DYNAMIC_STATE_STENCIL_REFERENCE);
    return *this;
}

VkPipelineDynamicStateCreateInfo& DynamicStateBuilder::buildPipelineDynamicState() {
    _info.dynamicStateCount = COUNT(_dynamicStates);
    _info.pDynamicStates = _dynamicStates.data();
    return _info;
}
