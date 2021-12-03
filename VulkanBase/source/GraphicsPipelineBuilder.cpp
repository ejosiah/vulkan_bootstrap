#include "GraphicsPipelineBuilder.hpp"

GraphicsPipelineBuilder::GraphicsPipelineBuilder(VulkanDevice *device)
: Builder{ device, nullptr }
, _shaderStageBuilder{ std::make_unique<ShaderStageBuilder>(device, this)}
, _vertexInputStateBuilder{ std::make_unique<VertexInputStateBuilder>(device, this)}
, _inputAssemblyStateBuilder{ std::make_unique<InputAssemblyStateBuilder>(device, this)}
, _pipelineLayoutBuilder{ std::make_unique<PipelineLayoutBuilder>( device, this )}
, _viewportStateBuilder{ std::make_unique<ViewportStateBuilder>( device, this )}
, _rasterizationStateBuilder{ std::make_unique<RasterizationStateBuilder>( device, this )}
, _multisampleStateBuilder{ std::make_unique<MultisampleStateBuilder>(device, this)}
, _depthStencilStateBuilder{ std::make_unique<DepthStencilStateBuilder>(device, this)}
, _colorBlendStateBuilder{ std::make_unique<ColorBlendStateBuilder>(device, this)}
, _dynamicStateBuilder{ std::make_unique<DynamicStateBuilder>(device, this)}
, _name{""}
{

}

GraphicsPipelineBuilder::GraphicsPipelineBuilder(VulkanDevice *device, GraphicsPipelineBuilder *parent)
: Builder{ device, parent }
{
}

GraphicsPipelineBuilder::~GraphicsPipelineBuilder() = default;

ShaderStageBuilder &GraphicsPipelineBuilder::shaderStage() {
    if(parent()){
        return parent()->shaderStage();
    }
    return *_shaderStageBuilder;
}

VertexInputStateBuilder &GraphicsPipelineBuilder::vertexInputState() {
    if(parent()){
        return parent()->vertexInputState();
    }
    return *_vertexInputStateBuilder;
}

GraphicsPipelineBuilder *GraphicsPipelineBuilder::parent()  {
    return dynamic_cast<GraphicsPipelineBuilder*>(Builder::parent());
}

InputAssemblyStateBuilder& GraphicsPipelineBuilder::inputAssemblyState() {
    if(parent()){
        return parent()->inputAssemblyState();
    }
    return *_inputAssemblyStateBuilder;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::allowDerivatives() {
    if(parent()){
        return parent()->allowDerivatives();
    }
    _flags |= VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT;
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::setDerivatives() {
    if(parent()){
        return parent()->setDerivatives();
    }
    _flags |= VK_PIPELINE_CREATE_DERIVATIVE_BIT;
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::subpass(uint32_t value) {
    if(parent()){
        return parent()->subpass(value);
    }
    _subpass = value;
    return *this;
}


GraphicsPipelineBuilder& GraphicsPipelineBuilder::layout(VulkanPipelineLayout&  aLayout) {
    if(parent()){
        return parent()->layout(aLayout);
    }
    _pipelineLayout = &aLayout;
    return *this;
}

GraphicsPipelineBuilder &GraphicsPipelineBuilder::renderPass(VkRenderPass aRenderPass) {
    if(parent()){
        parent()->renderPass(aRenderPass);
    }
    _renderPass = aRenderPass;
    return *this;
}

PipelineLayoutBuilder &GraphicsPipelineBuilder::layout() {
    if(parent()){
        return parent()->layout();
    }
    return *_pipelineLayoutBuilder;
}

VulkanPipeline GraphicsPipelineBuilder::build() {
    if(parent()){
        return parent()->build();
    }
    if(!_pipelineLayout){
        throw std::runtime_error{"either provide or create a pipelineLayout"};
    }
    VulkanPipelineLayout unused{};
    return build(unused);
}


VulkanPipeline GraphicsPipelineBuilder::build(VulkanPipelineLayout& pipelineLayout) {
    if(parent()){
        return parent()->build(pipelineLayout);
    }
    auto info = createInfo();
    pipelineLayout = std::move(_pipelineLayoutOwned);
    auto pipeline = device().createGraphicsPipeline(info);
    if(!_name.empty()){
        device().setName<VK_OBJECT_TYPE_PIPELINE>(_name, pipeline.handle);
    }
    return pipeline;
}

VkGraphicsPipelineCreateInfo GraphicsPipelineBuilder::createInfo() {
    if(parent()) return parent()->createInfo();

    auto& shaderStages = _shaderStageBuilder->buildShaderStage();
    auto& vertexInputState = _vertexInputStateBuilder->buildVertexInputState();
    auto& inputAssemblyState = _inputAssemblyStateBuilder->buildInputAssemblyState();
    auto& viewportState = _viewportStateBuilder->buildViewportState();
    auto& rasterState = _rasterizationStateBuilder->buildRasterState();
    auto& multisampleState = _multisampleStateBuilder->buildMultisampleState();
    auto& depthStencilState = _depthStencilStateBuilder->buildDepthStencilState();
    auto& colorBlendState = _colorBlendStateBuilder->buildColorBlendState();
    auto& dynamicState = _dynamicStateBuilder->buildPipelineDynamicState();

    auto info = initializers::graphicsPipelineCreateInfo();
    info.flags = _flags;
    info.stageCount = COUNT(shaderStages);
    info.pStages = shaderStages.data();
    info.pVertexInputState = &vertexInputState;
    info.pInputAssemblyState = &inputAssemblyState;
    info.pViewportState = &viewportState;
    info.pRasterizationState = &rasterState;
    info.pMultisampleState = &multisampleState;
    info.pDepthStencilState = &depthStencilState;
    info.pColorBlendState = &colorBlendState;
    info.pDynamicState = &dynamicState;

    if(_flags & VK_PIPELINE_CREATE_DERIVATIVE_BIT){
        assert(_basePipeline);
        info.basePipelineHandle = _basePipeline->handle;
        info.basePipelineIndex = -1;
    }

    if(!_pipelineLayout){
        _pipelineLayoutOwned = _pipelineLayoutBuilder->buildPipelineLayout();
        info.layout = _pipelineLayoutOwned;
    }else{
        info.layout = *_pipelineLayout;
    }
    info.renderPass = _renderPass;
    info.subpass = _subpass;

    return info;
}

ViewportStateBuilder &GraphicsPipelineBuilder::viewportState() {
    if(parent()){
        return parent()->viewportState();
    }
    return *_viewportStateBuilder;
}

RasterizationStateBuilder& GraphicsPipelineBuilder::rasterizationState() {
    if(parent()){
        return parent()->rasterizationState();
    }
    return *_rasterizationStateBuilder;
}

DepthStencilStateBuilder& GraphicsPipelineBuilder::depthStencilState() {
    if(parent()){
        return parent()->depthStencilState();
    }
    return *_depthStencilStateBuilder;
}

ColorBlendStateBuilder &GraphicsPipelineBuilder::colorBlendState() {
    if(parent()){
        return parent()->colorBlendState();
    }
    return *_colorBlendStateBuilder;
}

GraphicsPipelineBuilder &GraphicsPipelineBuilder::name(const std::string &value) {
    if(parent()){
        parent()->name(value);
    }
    _name = value;
    return *this;
}

GraphicsPipelineBuilder &GraphicsPipelineBuilder::reuse() {
    if(parent()){
        parent()->reuse();
    }
    _vertexInputStateBuilder->clear();
    _shaderStageBuilder->clear();
    _pipelineLayoutBuilder->clearLayouts();
    _pipelineLayoutBuilder->clearRanges();
    return *this;
}

GraphicsPipelineBuilder &GraphicsPipelineBuilder::basePipeline(VulkanPipeline &pipeline) {
    if(parent()){
        parent()->basePipeline(pipeline);
    }
    _basePipeline = &pipeline;
    return *this;
}

MultisampleStateBuilder &GraphicsPipelineBuilder::multisampleState() {
    if(parent()){
        return parent()->multisampleState();
    }
    return *_multisampleStateBuilder;
}

GraphicsPipelineBuilder &GraphicsPipelineBuilder::pipelineCache(VkPipelineCache pCache) {
    if(parent()){
        parent()->pipelineCache(pCache);
    }
    pipelineCache_ = pCache;
    return *this;
}

DynamicStateBuilder &GraphicsPipelineBuilder::dynamicState() {
    if(parent()){
        return parent()->dynamicState();
    }
    return *_dynamicStateBuilder;
}
