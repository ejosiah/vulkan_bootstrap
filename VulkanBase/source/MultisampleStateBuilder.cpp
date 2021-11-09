
#include "MultisampleStateBuilder.hpp"

MultisampleStateBuilder::MultisampleStateBuilder(VulkanDevice *device, GraphicsPipelineBuilder *parent)
: GraphicsPipelineBuilder(device, parent)
, _info(initializers::multisampleState())
{

}

MultisampleStateBuilder &MultisampleStateBuilder::rasterizationSamples(VkSampleCountFlagBits flags) {
    _info.rasterizationSamples = flags;
    return *this;
}

MultisampleStateBuilder &MultisampleStateBuilder::enableSampleShading() {
    _info.sampleShadingEnable = VK_TRUE;
    return *this;
}

MultisampleStateBuilder &MultisampleStateBuilder::disableSampleShading() {
    _info.sampleShadingEnable = VK_FALSE;
    return *this;
}

MultisampleStateBuilder &MultisampleStateBuilder::minSampleShading(float value) {
    _info.minSampleShading = value;
    return *this;
}

MultisampleStateBuilder &MultisampleStateBuilder::sampleMask(const VkSampleMask *mask) {
    _info.pSampleMask = mask;
    return *this;
}

MultisampleStateBuilder &MultisampleStateBuilder::enableAlphaToCoverage() {
    _info.alphaToCoverageEnable = VK_TRUE;
    return *this;
}

MultisampleStateBuilder &MultisampleStateBuilder::disableAlphaToCoverage() {
    _info.alphaToCoverageEnable = VK_FALSE;
    return *this;
}

MultisampleStateBuilder &MultisampleStateBuilder::enableAlphaToOne() {
    _info.alphaToOneEnable = VK_TRUE;
    return *this;
}

VkPipelineMultisampleStateCreateInfo &MultisampleStateBuilder::buildMultisampleState() {
    _info.alphaToOneEnable = VK_FALSE;
    return _info;
}
