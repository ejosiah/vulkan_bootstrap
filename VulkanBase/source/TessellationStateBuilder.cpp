#include "TessellationStateBuilder.hpp"

TessellationStateBuilder::TessellationStateBuilder(VulkanDevice *device, GraphicsPipelineBuilder *parent)
        : GraphicsPipelineBuilder(device, parent) {
}

TessellationStateBuilder &TessellationStateBuilder::patchControlPoints(uint32_t count) {
    _info.patchControlPoints = count;
    return *this;
}

TessellationStateBuilder &TessellationStateBuilder::domainOrigin(VkTessellationDomainOrigin origin) {
    originStateInfo.domainOrigin = origin;
    return *this;
}

VkPipelineTessellationStateCreateInfo &TessellationStateBuilder::buildTessellationState() {
    _info.pNext = &originStateInfo;
    return _info;
}

GraphicsPipelineBuilder &TessellationStateBuilder::clear() {
    auto graphicsPipelineBuilder = reinterpret_cast<GraphicsPipelineBuilder*>(_parent);
    _info = {VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO};
    return *graphicsPipelineBuilder;
}
