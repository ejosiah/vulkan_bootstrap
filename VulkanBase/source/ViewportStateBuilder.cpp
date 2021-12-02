#include "ViewportStateBuilder.hpp"

ViewportStateBuilder::ViewportStateBuilder(VulkanDevice *device, GraphicsPipelineBuilder *parent)
: GraphicsPipelineBuilder(device, parent)
, _viewportBuilder{ new ViewportBuilder{ this }}
, _scissorBuilder{ new ScissorBuilder{ this }}
{
}

ViewportStateBuilder::ViewportStateBuilder(ViewportStateBuilder *parent)
: GraphicsPipelineBuilder(parent->_device, parent)
{
}

ViewportStateBuilder::~ViewportStateBuilder() {
    delete _viewportBuilder;
    delete _scissorBuilder;
}


ViewportBuilder &ViewportStateBuilder::viewport() {
    return *_viewportBuilder;
}

ScissorBuilder &ViewportStateBuilder::scissor() {
    return *_scissorBuilder;
}

VkPipelineViewportStateCreateInfo& ViewportStateBuilder::buildViewportState() {
    auto& viewports = _viewportBuilder->buildViewports();
    auto& scissors = _scissorBuilder->buildScissors();
    _info = initializers::viewportState(viewports, scissors);
    return _info;
}

ViewportBuilder::ViewportBuilder(ViewportStateBuilder *builder) : ViewportStateBuilder(builder) {
    resetScratchpad();
}

ViewportBuilder &ViewportBuilder::origin(float xValue, float yValue) {
    x(xValue);
    y(yValue);
    return *this;
}


ViewportBuilder &ViewportBuilder::x(float value) {
    _scratchpad.x = value;
    return *this;
}

ViewportBuilder &ViewportBuilder::y(float value) {
    _scratchpad.y = value;
    return *this;
}

ViewportBuilder &ViewportBuilder::dimension(VkExtent2D dim) {
    width(static_cast<float>(dim.width));
    height(static_cast<float>(dim.height));
    return *this;
}

ViewportBuilder &ViewportBuilder::dimension(uint32_t w, uint32_t h) {
    width(static_cast<float>(w));
    height(static_cast<float>(h));
    return *this;
}

ViewportBuilder &ViewportBuilder::width(float value) {
    _scratchpad.width = value;
    return *this;
}

ViewportBuilder &ViewportBuilder::height(float value) {
    _scratchpad.height = value;
    return *this;
}

ViewportBuilder &ViewportBuilder::minDepth(float value) {
    _scratchpad.maxDepth = value;
    return *this;
}

ViewportBuilder &ViewportBuilder::maxDepth(float value) {
    _scratchpad.maxDepth = value;
    return *this;
}

ViewportBuilder &ViewportBuilder::add() {
    if(_scratchpad.width <= 0 || _scratchpad.height <= 0){
        throw std::runtime_error{"viewport width and height required"};
    }
    _viewports.push_back(_scratchpad);
    resetScratchpad();
    return *this;
}

std::vector<VkViewport>& ViewportBuilder::buildViewports() {
    if(_viewports.empty()){
        throw std::runtime_error{"at least one viewport should be provided"};
    }
    return _viewports;
}

void ViewportBuilder::resetScratchpad() {
    _scratchpad = VkViewport{0, 0, 0, 0, 0, 1};
}

bool ViewportBuilder::ready() const{
    return
            (_scratchpad.width > 0 && _scratchpad.height > 0)
            && (_scratchpad.minDepth != _scratchpad.maxDepth);
}

void ViewportBuilder::checkpoint() {
    if(ready()){
        add();
    }
}

ViewportStateBuilder *ViewportBuilder::parent() {
    return dynamic_cast<ViewportStateBuilder*>(_parent);
}

ViewportBuilder &ViewportBuilder::viewport() {
    checkpoint();
    return *this;
}

ScissorBuilder &ViewportBuilder::scissor() {
    checkpoint();
    return parent()->scissor();
}

ScissorBuilder::ScissorBuilder(ViewportStateBuilder *builder)
: ViewportStateBuilder(builder)
{
    resetScratchpad();
}

ScissorBuilder &ScissorBuilder::offset(int32_t x, int32_t y) {
    _scratchpad.offset = VkOffset2D{x, y};
    return *this;
}

ScissorBuilder &ScissorBuilder::extent(int32_t width, int32_t height) {
    _scratchpad.extent.width = width;
    _scratchpad.extent.height = height;
    return *this;
}

ScissorBuilder &ScissorBuilder::extent(VkExtent2D value) {
    _scratchpad.extent = value;
    return *this;
}

ScissorBuilder &ScissorBuilder::add() {
    if(_scratchpad.extent.width <= 0 || _scratchpad.extent.height <= 0){
        throw std::runtime_error{"scissor width and height required"};
    }
    _scissors.push_back(_scratchpad);
    resetScratchpad();
    return *this;
}

void ScissorBuilder::resetScratchpad() {
    _scratchpad = { {0u, 0u}, {0u, 0u}};
}

std::vector<VkRect2D>& ScissorBuilder::buildScissors() {
    return _scissors;
}

void ScissorBuilder::checkpoint() {
    if(ready()){
        add();
    }
}

ViewportStateBuilder *ScissorBuilder::parent() {
    return dynamic_cast<ViewportStateBuilder*>(_parent);
}

bool ScissorBuilder::ready() const {
    return _scratchpad.extent.width > 0 && _scratchpad.extent.height > 0;
}

ViewportBuilder &ScissorBuilder::viewport() {
    checkpoint();
    return parent()->viewport();
}

ScissorBuilder &ScissorBuilder::scissor() {
    checkpoint();
    return *this;
}
