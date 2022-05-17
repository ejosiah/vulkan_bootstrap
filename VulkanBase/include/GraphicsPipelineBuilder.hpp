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

    virtual TessellationStateBuilder& tessellationState();

    virtual ViewportStateBuilder& viewportState();

    virtual RasterizationStateBuilder& rasterizationState();

    virtual DepthStencilStateBuilder& depthStencilState();

    virtual ColorBlendStateBuilder& colorBlendState();

    virtual MultisampleStateBuilder& multisampleState();

    virtual PipelineLayoutBuilder& layout();

    virtual DynamicStateBuilder& dynamicState();

    GraphicsPipelineBuilder& allowDerivatives();

    GraphicsPipelineBuilder& setDerivatives();

    GraphicsPipelineBuilder& subpass(uint32_t value);

    GraphicsPipelineBuilder& layout(VulkanPipelineLayout&  aLayout);

    GraphicsPipelineBuilder& renderPass(VkRenderPass  aRenderPass);

    GraphicsPipelineBuilder& name(const std::string& value);

    GraphicsPipelineBuilder& reuse();

    GraphicsPipelineBuilder& basePipeline(VulkanPipeline& pipeline);

    GraphicsPipelineBuilder& pipelineCache(VkPipelineCache pCache);

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
    std::unique_ptr<ShaderStageBuilder> _shaderStageBuilder = nullptr;
    std::unique_ptr<VertexInputStateBuilder> _vertexInputStateBuilder = nullptr;
    std::unique_ptr<InputAssemblyStateBuilder> _inputAssemblyStateBuilder = nullptr;
    std::unique_ptr<PipelineLayoutBuilder> _pipelineLayoutBuilder = nullptr;
    std::unique_ptr<ViewportStateBuilder> _viewportStateBuilder = nullptr;
    std::unique_ptr<RasterizationStateBuilder> _rasterizationStateBuilder = nullptr;
    std::unique_ptr<MultisampleStateBuilder> _multisampleStateBuilder = nullptr;
    std::unique_ptr<DepthStencilStateBuilder> _depthStencilStateBuilder = nullptr;
    std::unique_ptr<ColorBlendStateBuilder> _colorBlendStateBuilder = nullptr ;
    std::unique_ptr<DynamicStateBuilder> _dynamicStateBuilder = nullptr;
    std::unique_ptr<TessellationStateBuilder> _tessellationStateBuilder = nullptr;
    VulkanPipeline* _basePipeline = nullptr;
    VkPipelineCache pipelineCache_ = VK_NULL_HANDLE;

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
#include "DynamicStateBuilder.hpp"
#include "TessellationStateBuilder.hpp"