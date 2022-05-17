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