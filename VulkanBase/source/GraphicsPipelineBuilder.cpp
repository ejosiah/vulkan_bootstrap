#include "GraphicsPipelineBuilder.hpp"
#include "ShaderStageBuilder.hpp"
#include "VertexInputStateBuilder.hpp"

GraphicsPipelineBuilder::GraphicsPipelineBuilder(VulkanDevice *device)
: Builder{ device, nullptr }
, _shaderStageBuilder{ new ShaderStageBuilder{device, this}}
, _vertexInputStateBuilder{ new VertexInputStateBuilder{device, this}}
{
}

GraphicsPipelineBuilder::GraphicsPipelineBuilder(VulkanDevice *device, GraphicsPipelineBuilder *parent)
: Builder{ device, parent }
{
}

GraphicsPipelineBuilder::~GraphicsPipelineBuilder() {
     delete _shaderStageBuilder;
     delete _vertexInputStateBuilder;
}

ShaderStageBuilder &GraphicsPipelineBuilder::shaderStage() {
    if(!parent()){
        return *_shaderStageBuilder;
    }
    return parent()->shaderStage();
}

VertexInputStateBuilder &GraphicsPipelineBuilder::vertexInputState() {
    if(!parent()){
        return *_vertexInputStateBuilder;
    }
    return parent()->vertexInputState();
}

GraphicsPipelineBuilder *GraphicsPipelineBuilder::parent()  {
    return dynamic_cast<GraphicsPipelineBuilder*>(Builder::parent());
}

VulkanPipeline GraphicsPipelineBuilder::build() {
    auto info = createInfo();
    return device().createGraphicsPipeline(info);
}

VkGraphicsPipelineCreateInfo GraphicsPipelineBuilder::createInfo() {
    auto shaderStages = _shaderStageBuilder->buildShaderStage();
    auto vertexInputState = _vertexInputStateBuilder->buildVertexInputState();

    return initializers::graphicsPipelineCreateInfo();
}
