#pragma once

#include "common.h"
#include "Builder.hpp"
#include "VulkanDevice.h"
#include "VulkanRAII.h"
#include "VulkanInitializers.h"

class GraphicsPipelineBuilder : public Builder {
public:
    explicit GraphicsPipelineBuilder(VulkanDevice* device);

    GraphicsPipelineBuilder(VulkanDevice* device, GraphicsPipelineBuilder* parent);

    GraphicsPipelineBuilder() = default;

    virtual ~GraphicsPipelineBuilder();

    virtual ShaderStageBuilder& shaderStage();

    virtual VertexInputStateBuilder& vertexInputState();

    virtual InputAssemblyStateBuilder& inputAssemblyState();

    virtual ViewportStateBuilder& viewportState();

    virtual RasterizationStateBuilder& rasterizationState();

    virtual DepthStencilStateBuilder& depthStencilState();

    virtual ColorBlendStateBuilder& colorBlendState();

    virtual MultisampleStateBuilder& multisampleState();

    virtual PipelineLayoutBuilder& layout();

    GraphicsPipelineBuilder& allowDerivatives();

    GraphicsPipelineBuilder& setDerivatives();

    GraphicsPipelineBuilder& subpass(uint32_t value);

    GraphicsPipelineBuilder& layout(VulkanPipelineLayout&  aLayout);

    GraphicsPipelineBuilder& renderPass(VkRenderPass  aRenderPass);

    GraphicsPipelineBuilder& name(const std::string& value);

    GraphicsPipelineBuilder& reuse();

    GraphicsPipelineBuilder& basePipeline(VulkanPipeline& pipeline);

    [[nodiscard]]
    GraphicsPipelineBuilder *parent() override;

    VulkanPipeline build();

    VulkanPipeline build(VulkanPipelineLayout& pipelineLayout);

    VkGraphicsPipelineCreateInfo createInfo();

protected:
    VkPipelineCache _pipelineCache = VK_NULL_HANDLE;
    VkPipelineCreateFlags _flags = 0;
    VkRenderPass _renderPass = VK_NULL_HANDLE;
    VulkanPipelineLayout* _pipelineLayout = nullptr;
    VulkanPipelineLayout _pipelineLayoutOwned;
    uint32_t _subpass = 0;
    std::string _name;

private:
    ShaderStageBuilder* _shaderStageBuilder = nullptr;
    VertexInputStateBuilder* _vertexInputStateBuilder = nullptr;
    InputAssemblyStateBuilder* _inputAssemblyStateBuilder = nullptr;
    PipelineLayoutBuilder* _pipelineLayoutBuilder = nullptr;
    ViewportStateBuilder* _viewportStateBuilder = nullptr;
    RasterizationStateBuilder* _rasterizationStateBuilder = nullptr;
    MultisampleStateBuilder* _multisampleStateBuilder = nullptr;
    DepthStencilStateBuilder* _depthStencilStateBuilder = nullptr;
    ColorBlendStateBuilder* _colorBlendStateBuilder = nullptr ;
    VulkanPipeline* _basePipeline = nullptr;

};

#include "ShaderStageBuilder.hpp"
#include "VertexInputStateBuilder.hpp"
#include "InputAssemblyStateBuilder.hpp"
#include "PipelineLayoutBuilder.hpp"
#include "ViewportStateBuilder.hpp"
#include "RasterizationStateBuilder.hpp"
#include "MultisampleStateBuilder.hpp"
#include "DepthStencilStateBuilder.hpp"
#include "ColorBlendStateBuilder.hpp"