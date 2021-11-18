#include "ColorBlendStateBuilder.hpp"

ColorBlendStateBuilder::ColorBlendStateBuilder(VulkanDevice *device, GraphicsPipelineBuilder *parent)
: GraphicsPipelineBuilder(device, parent)
, _colorBlendAttachmentStateBuilder{ new ColorBlendAttachmentStateBuilder{this}}
, _logicOp{ this }
{
}

ColorBlendStateBuilder::ColorBlendStateBuilder(ColorBlendStateBuilder *parent)
: GraphicsPipelineBuilder(parent->_device, parent)
{

}

ColorBlendStateBuilder::~ColorBlendStateBuilder() {
    delete _colorBlendAttachmentStateBuilder;
}

ColorBlendStateBuilder &ColorBlendStateBuilder::blendConstants(float r, float g, float b, float a) {
    _info.blendConstants[0] = r;
    _info.blendConstants[1] = b;
    _info.blendConstants[2] = b;
    _info.blendConstants[3] = a;
    return *this;
}

ColorBlendAttachmentStateBuilder &ColorBlendStateBuilder::attachment() {
    return *_colorBlendAttachmentStateBuilder;
}

VkPipelineColorBlendStateCreateInfo &ColorBlendStateBuilder::buildColorBlendState() {
    auto& colorAttachmentStates = _colorBlendAttachmentStateBuilder->buildColorBlendAttachmentState();
    _info.attachmentCount = COUNT(colorAttachmentStates);
    _info.pAttachments = colorAttachmentStates.data();
    _info.logicOpEnable = _logicOp.enabled;
    _info.logicOp = _logicOp.logicOp;
    return _info;
}

ColorBlendAttachmentStateBuilder::ColorBlendAttachmentStateBuilder(ColorBlendStateBuilder *parent)
 : ColorBlendStateBuilder(parent)
{
    _srcColorBlendFactor._caller = this;
    _dstColorBlendFactor._caller = this;
    _srcAlphaBlendFactor._caller = this;
    _dstAlphaBlendFactor._caller = this;
    _colorBlendOp._caller = this;
    _alphaBlendOp._caller = this;
    resetScratchpad();
}

ColorBlendAttachmentStateBuilder &ColorBlendAttachmentStateBuilder::enableBlend() {
    dirty();
    _scratchpad.blendEnable = VK_TRUE;
    return *this;
}

ColorBlendAttachmentStateBuilder &ColorBlendAttachmentStateBuilder::disableBlend() {
    dirty();
    _scratchpad.blendEnable = VK_FALSE;
    return *this;
}

ColorBlendAttachmentStateBuilder &ColorBlendAttachmentStateBuilder::colorWriteMask(VkColorComponentFlags flags) {
    dirty();
    _scratchpad.colorWriteMask = flags;
    return *this;
}

void ColorBlendAttachmentStateBuilder::resetScratchpad() {
    _dirty = false;
    _scratchpad = VkPipelineColorBlendAttachmentState{};
    _scratchpad.colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
}

ColorBlendAttachmentStateBuilder &ColorBlendAttachmentStateBuilder::add() {
    _scratchpad.srcColorBlendFactor = _srcColorBlendFactor.blendFactor;
    _scratchpad.dstColorBlendFactor = _dstColorBlendFactor.blendFactor;
    _scratchpad.colorBlendOp = _colorBlendOp.blendOp;
    _scratchpad.srcAlphaBlendFactor = _srcAlphaBlendFactor.blendFactor;
    _scratchpad.dstAlphaBlendFactor = _dstAlphaBlendFactor.blendFactor;
    _scratchpad.alphaBlendOp = _alphaBlendOp.blendOp;
    _states.push_back(_scratchpad);
    resetScratchpad();
    return *this;
}

std::vector<VkPipelineColorBlendAttachmentState> &ColorBlendAttachmentStateBuilder::buildColorBlendAttachmentState() {
    return _states;
}

ColorBlendAttachmentStateBuilder &ColorBlendAttachmentStateBuilder::attachment() {
    if(_dirty){
        add();
    }
    return  *this;
}

ColorBlendAttachmentStateBuilder &ColorBlendAttachmentStateBuilder::clear() {
    _states.clear();
    resetScratchpad();
    return *this;
}
