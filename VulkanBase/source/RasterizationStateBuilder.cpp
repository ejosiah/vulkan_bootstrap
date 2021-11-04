#include "RasterizationStateBuilder.hpp"

RasterizationStateBuilder::RasterizationStateBuilder(VulkanDevice *device, GraphicsPipelineBuilder *parent)
: GraphicsPipelineBuilder(device, parent)
, _info(initializers::rasterizationState())
{

}

RasterizationStateBuilder &RasterizationStateBuilder::enableDepthClamp() {
    _info.depthClampEnable = VK_TRUE;
    return *this;
}

RasterizationStateBuilder &RasterizationStateBuilder::disableDepthClamp() {
    _info.depthClampEnable = VK_FALSE;
    return *this;
}

RasterizationStateBuilder &RasterizationStateBuilder::enableRasterizerDiscard() {
    _info.rasterizerDiscardEnable = VK_TRUE;
    return *this;
}

RasterizationStateBuilder &RasterizationStateBuilder::disableRasterizerDiscard() {
    _info.rasterizerDiscardEnable = VK_FALSE;
    return *this;
}

RasterizationStateBuilder &RasterizationStateBuilder::polygonModeFill() {
    _info.polygonMode = VK_POLYGON_MODE_FILL;
    return *this;
}

RasterizationStateBuilder &RasterizationStateBuilder::polygonModeLine() {
    _info.polygonMode = VK_POLYGON_MODE_LINE;
    return *this;
}

RasterizationStateBuilder &RasterizationStateBuilder::polygonModePoint() {
    _info.polygonMode = VK_POLYGON_MODE_POINT;
    return *this;
}

RasterizationStateBuilder &RasterizationStateBuilder::cullNone() {
    _info.cullMode = VK_CULL_MODE_NONE;
    return *this;
}

RasterizationStateBuilder &RasterizationStateBuilder::cullFrontFace() {
    _info.cullMode = VK_CULL_MODE_FRONT_BIT;
    return *this;
}

RasterizationStateBuilder &RasterizationStateBuilder::cullBackFace() {
    _info.cullMode = VK_CULL_MODE_BACK_BIT;
    return *this;
}

RasterizationStateBuilder &RasterizationStateBuilder::cullFrontAndBackFace() {
    _info.cullMode = VK_CULL_MODE_FRONT_AND_BACK;
    return *this;
}

RasterizationStateBuilder &RasterizationStateBuilder::frontFaceCounterClockwise() {
    _info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    return *this;
}

RasterizationStateBuilder &RasterizationStateBuilder::frontFaceClockwise() {
    _info.frontFace = VK_FRONT_FACE_CLOCKWISE;
    return *this;
}

RasterizationStateBuilder &RasterizationStateBuilder::enableDepthBias() {
    _info.depthBiasEnable = VK_TRUE;
    return *this;
}

RasterizationStateBuilder &RasterizationStateBuilder::disableDepthBias() {
    _info.depthBiasEnable = VK_FALSE;
    return *this;
}

RasterizationStateBuilder &RasterizationStateBuilder::depthBiasConstantFactor(float value) {
    _info.depthBiasConstantFactor = value;
    return *this;
}

RasterizationStateBuilder &RasterizationStateBuilder::depthBiasClamp(float value) {
    _info.depthBiasClamp = value;
    return *this;
}

RasterizationStateBuilder &RasterizationStateBuilder::depthBiasSlopeFactor(float value) {
    _info.depthBiasSlopeFactor = value;
    return *this;
}

RasterizationStateBuilder &RasterizationStateBuilder::lineWidth(float value) {
    _info.lineWidth = value;
    return *this;
}

VkPipelineRasterizationStateCreateInfo &RasterizationStateBuilder::buildRasterState() {
    return _info;
}
