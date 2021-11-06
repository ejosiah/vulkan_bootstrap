#include "DepthStencilStateBuilder.hpp"

DepthStencilStateBuilder::DepthStencilStateBuilder(VulkanDevice *device, GraphicsPipelineBuilder *parent)
: GraphicsPipelineBuilder(device, parent)
, _info{ initializers::depthStencilState() }
, _front{ new StencilOpStateBuilder{this} }
, _back{ new StencilOpStateBuilder{this} }
{
}

DepthStencilStateBuilder::DepthStencilStateBuilder(DepthStencilStateBuilder *parent)
:GraphicsPipelineBuilder(parent->_device, parent)
{
    _info.depthTestEnable = VK_FALSE;
    _info.depthWriteEnable = VK_FALSE;
    _info.minDepthBounds = 0.f;
    _info.maxDepthBounds = 1.f;
}

DepthStencilStateBuilder &DepthStencilStateBuilder::enableDepthTest() {
    _info.depthTestEnable = VK_TRUE;
    return *this;
}

DepthStencilStateBuilder &DepthStencilStateBuilder::disableDepthTest() {
    _info.depthTestEnable = VK_FALSE;
    return *this;
}

DepthStencilStateBuilder &DepthStencilStateBuilder::enableDepthWrite() {
    _info.depthWriteEnable = VK_TRUE;
    return *this;
}

DepthStencilStateBuilder &DepthStencilStateBuilder::disableDepthWrite() {
    _info.depthWriteEnable = VK_FALSE;
    return *this;
}

DepthStencilStateBuilder &DepthStencilStateBuilder::compareOpNever() {
    _info.depthCompareOp = VK_COMPARE_OP_NEVER;
    return *this;
}

DepthStencilStateBuilder &DepthStencilStateBuilder::compareOpLess() {
    _info.depthCompareOp = VK_COMPARE_OP_LESS;
    return *this;
}

DepthStencilStateBuilder &DepthStencilStateBuilder::compareOpEqual() {
    _info.depthCompareOp = VK_COMPARE_OP_EQUAL;
    return *this;
}

DepthStencilStateBuilder &DepthStencilStateBuilder::compareOpLessOrEqual() {
    _info.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    return *this;
}

DepthStencilStateBuilder &DepthStencilStateBuilder::compareOpGreater() {
    _info.depthCompareOp = VK_COMPARE_OP_GREATER;
    return *this;
}

DepthStencilStateBuilder &DepthStencilStateBuilder::compareOpGreaterOrEqual() {
    _info.depthCompareOp = VK_COMPARE_OP_GREATER_OR_EQUAL;
    return *this;
}

DepthStencilStateBuilder &DepthStencilStateBuilder::compareOpNotEqual() {
    _info.depthCompareOp = VK_COMPARE_OP_NOT_EQUAL;
    return *this;
}

DepthStencilStateBuilder &DepthStencilStateBuilder::compareOpAlways() {
    _info.depthCompareOp = VK_COMPARE_OP_ALWAYS;
    return *this;
}

DepthStencilStateBuilder &DepthStencilStateBuilder::enableDepthBoundsTest() {
    _info.depthBoundsTestEnable = VK_TRUE;
    return *this;
}

DepthStencilStateBuilder &DepthStencilStateBuilder::disableDepthBoundsTest() {
    _info.depthBoundsTestEnable = VK_FALSE;
    return *this;
}

DepthStencilStateBuilder &DepthStencilStateBuilder::enableStencilTest() {
    _info.stencilTestEnable = VK_TRUE;
    return *this;
}

DepthStencilStateBuilder &DepthStencilStateBuilder::disableStencilTest() {
    _info.stencilTestEnable = VK_FALSE;
    return *this;
}

StencilOpStateBuilder &DepthStencilStateBuilder::front() {
    return *_front;
}

StencilOpStateBuilder &DepthStencilStateBuilder::back() {
    return *_back;
}

DepthStencilStateBuilder &DepthStencilStateBuilder::minDepthBounds(float value) {
    _info.minDepthBounds = value;
    return *this;
}

DepthStencilStateBuilder &DepthStencilStateBuilder::maxDepthBounds(float value) {
    _info.maxDepthBounds = value;
    return *this;
}

VkPipelineDepthStencilStateCreateInfo &DepthStencilStateBuilder::buildDepthStencilState() {
    _info.front = _front->buildStencilOpState();
    _info.back = _back->buildStencilOpState();
    return _info;
}

StencilOpStateBuilder::StencilOpStateBuilder(DepthStencilStateBuilder *parent)
: DepthStencilStateBuilder(parent)
, _stencilOpState{}
{

}

StencilOpStateBuilder &StencilOpStateBuilder::failOpKeep() {
    _stencilOpState.failOp = VK_STENCIL_OP_KEEP;
    return *this;
}

StencilOpStateBuilder &StencilOpStateBuilder::failOpZero() {
    _stencilOpState.failOp = VK_STENCIL_OP_ZERO;
    return *this;
}

StencilOpStateBuilder &StencilOpStateBuilder::failOpReplace() {
    _stencilOpState.failOp = VK_STENCIL_OP_REPLACE;
    return *this;
}

StencilOpStateBuilder &StencilOpStateBuilder::failOpIncrementAndClamp() {
    _stencilOpState.failOp = VK_STENCIL_OP_INCREMENT_AND_CLAMP;
    return *this;
}

StencilOpStateBuilder &StencilOpStateBuilder::failOpDecrementAndClamp() {
    _stencilOpState.failOp = VK_STENCIL_OP_DECREMENT_AND_CLAMP;
    return *this;
}

StencilOpStateBuilder &StencilOpStateBuilder::failOpInvert() {
    _stencilOpState.failOp = VK_STENCIL_OP_INVERT;
    return *this;
}

StencilOpStateBuilder &StencilOpStateBuilder::failOpIncrementAndWrap() {
    _stencilOpState.failOp = VK_STENCIL_OP_INCREMENT_AND_WRAP;
    return *this;
}

StencilOpStateBuilder &StencilOpStateBuilder::failOpDecrementAndWrap() {
    _stencilOpState.failOp = VK_STENCIL_OP_DECREMENT_AND_WRAP;
    return *this;
}

StencilOpStateBuilder &StencilOpStateBuilder::passOpKeep() {
    _stencilOpState.passOp = VK_STENCIL_OP_KEEP;
    return *this;
}

StencilOpStateBuilder &StencilOpStateBuilder::passOpZero() {
    _stencilOpState.passOp = VK_STENCIL_OP_ZERO;
    return *this;
}

StencilOpStateBuilder &StencilOpStateBuilder::passOpReplace() {
    _stencilOpState.passOp = VK_STENCIL_OP_REPLACE;
    return *this;
}

StencilOpStateBuilder &StencilOpStateBuilder::passOpIncrementAndClamp() {
    _stencilOpState.passOp = VK_STENCIL_OP_INCREMENT_AND_CLAMP;
    return *this;
}

StencilOpStateBuilder &StencilOpStateBuilder::passOpDecrementAndClamp() {
    _stencilOpState.passOp = VK_STENCIL_OP_DECREMENT_AND_CLAMP;
    return *this;
}

StencilOpStateBuilder &StencilOpStateBuilder::passOpInvert() {
    _stencilOpState.passOp = VK_STENCIL_OP_INVERT;
    return *this;
}

StencilOpStateBuilder &StencilOpStateBuilder::depthFailOpKeep() {
    _stencilOpState.depthFailOp = VK_STENCIL_OP_KEEP;
    return *this;
}

StencilOpStateBuilder &StencilOpStateBuilder::depthFailOpZero() {
    _stencilOpState.depthFailOp = VK_STENCIL_OP_ZERO;
    return *this;
}

StencilOpStateBuilder &StencilOpStateBuilder::depthFailOpReplace() {
    _stencilOpState.depthFailOp = VK_STENCIL_OP_REPLACE;
    return *this;
}

StencilOpStateBuilder &StencilOpStateBuilder::depthFailOpIncrementAndClamp() {
    _stencilOpState.depthFailOp = VK_STENCIL_OP_INCREMENT_AND_CLAMP;
    return *this;
}

StencilOpStateBuilder &StencilOpStateBuilder::depthFailOpDecrementAndClamp() {
    _stencilOpState.depthFailOp = VK_STENCIL_OP_DECREMENT_AND_CLAMP;
    return *this;
}

StencilOpStateBuilder &StencilOpStateBuilder::depthFailOpInvert() {
    _stencilOpState.depthFailOp = VK_STENCIL_OP_INVERT;
    return *this;
}

StencilOpStateBuilder &StencilOpStateBuilder::depthFailOpIncrementAndWrap() {
    _stencilOpState.depthFailOp = VK_STENCIL_OP_INCREMENT_AND_WRAP;
    return *this;
}

StencilOpStateBuilder &StencilOpStateBuilder::depthFailOpDecrementAndWrap() {
    _stencilOpState.depthFailOp = VK_STENCIL_OP_DECREMENT_AND_WRAP;
    return *this;
}

StencilOpStateBuilder &StencilOpStateBuilder::passOpIncrementAndWrap() {
    _stencilOpState.passOp = VK_STENCIL_OP_INCREMENT_AND_WRAP;
    return *this;
}

StencilOpStateBuilder &StencilOpStateBuilder::passOpDecrementAndWrap() {
    _stencilOpState.passOp = VK_STENCIL_OP_DECREMENT_AND_WRAP;
    return *this;
}

StencilOpStateBuilder &StencilOpStateBuilder::compareOpNever() {
    _stencilOpState.compareOp = VK_COMPARE_OP_NEVER;
    return *this;
}

StencilOpStateBuilder &StencilOpStateBuilder::compareOpLess() {
    _stencilOpState.compareOp = VK_COMPARE_OP_LESS;
    return *this;
}

StencilOpStateBuilder &StencilOpStateBuilder::compareOpEqual() {
    _stencilOpState.compareOp = VK_COMPARE_OP_EQUAL;
    return *this;
}

StencilOpStateBuilder &StencilOpStateBuilder::compareOpLessOrEqual() {
    _stencilOpState.compareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    return *this;
}

StencilOpStateBuilder &StencilOpStateBuilder::compareOpGreater() {
    _stencilOpState.compareOp = VK_COMPARE_OP_GREATER;
    return *this;
}

StencilOpStateBuilder &StencilOpStateBuilder::compareOpGreaterOrEqual() {
    _stencilOpState.compareOp = VK_COMPARE_OP_GREATER_OR_EQUAL;
    return *this;
}

StencilOpStateBuilder &StencilOpStateBuilder::compareOpNotEqual() {
    _stencilOpState.compareOp = VK_COMPARE_OP_NOT_EQUAL;
    return *this;
}

StencilOpStateBuilder &StencilOpStateBuilder::compareOpAlways() {
    _stencilOpState.compareOp = VK_COMPARE_OP_ALWAYS;
    return *this;
}

StencilOpStateBuilder &StencilOpStateBuilder::compareMask(uint32_t value) {
    _stencilOpState.compareMask = value;
    return *this;
}

StencilOpStateBuilder &StencilOpStateBuilder::writeMask(uint32_t value) {
    _stencilOpState.writeMask = value;
    return *this;
}

StencilOpStateBuilder &StencilOpStateBuilder::reference(uint32_t value) {
    _stencilOpState.reference = value;
    return *this;
}

VkStencilOpState StencilOpStateBuilder::buildStencilOpState() {
    return _stencilOpState;
}
