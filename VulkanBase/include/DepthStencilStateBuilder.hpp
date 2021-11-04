#pragma once

#include "GraphicsPipelineBuilder.hpp"

class StencilOpStateBuilder;

class DepthStencilStateBuilder : public GraphicsPipelineBuilder{
public:
    DepthStencilStateBuilder(VulkanDevice* device, GraphicsPipelineBuilder* parent);

    explicit DepthStencilStateBuilder(DepthStencilStateBuilder* parent);

    DepthStencilStateBuilder& enableDepthTest();

    DepthStencilStateBuilder& disableDepthTest();

    DepthStencilStateBuilder& enableDepthWrite();

    DepthStencilStateBuilder& disableDepthWrite();

    virtual DepthStencilStateBuilder& compareOpNever();

    virtual DepthStencilStateBuilder& compareOpLess();

    virtual DepthStencilStateBuilder& compareOpEqual();

    virtual DepthStencilStateBuilder& compareOpLessOrEqual();

    virtual DepthStencilStateBuilder& compareOpGreater();

    virtual DepthStencilStateBuilder& compareOpGreaterOrEqual();

    virtual DepthStencilStateBuilder& compareOpNotEqual();

    virtual DepthStencilStateBuilder& compareOpAlways();

    DepthStencilStateBuilder& enableDepthBoundsTest();

    DepthStencilStateBuilder& disableDepthBoundsTest();

    DepthStencilStateBuilder& enableStencilTest();

    DepthStencilStateBuilder& disableStencilTest();

    StencilOpStateBuilder& front();

    StencilOpStateBuilder& back();

    DepthStencilStateBuilder& minDepthBounds(float value);

    DepthStencilStateBuilder& maxDepthBounds(float value);

    VkPipelineDepthStencilStateCreateInfo& buildDepthStencilState();

private:
    VkPipelineDepthStencilStateCreateInfo _info;
    StencilOpStateBuilder* _front = nullptr;
    StencilOpStateBuilder* _back = nullptr;
};

class StencilOpStateBuilder : public DepthStencilStateBuilder{
public:
    explicit StencilOpStateBuilder(DepthStencilStateBuilder* parent);

    StencilOpStateBuilder& failOpKeep();

    StencilOpStateBuilder& failOpZero();

    StencilOpStateBuilder& failOpReplace();

    StencilOpStateBuilder& failOpIncrementAndClamp();

    StencilOpStateBuilder& failOpDecrementAndClamp();

    StencilOpStateBuilder& failOpInvert();

    StencilOpStateBuilder& failOpIncrementAndWrap();

    StencilOpStateBuilder& failOpDecrementAndWrap();

    StencilOpStateBuilder& passOpKeep();

    StencilOpStateBuilder& passOpZero();

    StencilOpStateBuilder& passOpReplace();

    StencilOpStateBuilder& passOpIncrementAndClamp();

    StencilOpStateBuilder& passOpDecrementAndClamp();

    StencilOpStateBuilder& passOpInvert();

    StencilOpStateBuilder& passOpIncrementAndWrap();

    StencilOpStateBuilder& passOpDecrementAndWrap();

    StencilOpStateBuilder& depthFailOpKeep();

    StencilOpStateBuilder& depthFailOpZero();

    StencilOpStateBuilder& depthFailOpReplace();

    StencilOpStateBuilder& depthFailOpIncrementAndClamp();

    StencilOpStateBuilder& depthFailOpDecrementAndClamp();

    StencilOpStateBuilder& depthFailOpInvert();

    StencilOpStateBuilder& depthFailOpIncrementAndWrap();

    StencilOpStateBuilder& depthFailOpDecrementAndWrap();

    StencilOpStateBuilder &compareOpNever() override;

    StencilOpStateBuilder &compareOpLess() override;

    StencilOpStateBuilder &compareOpEqual() override;

    StencilOpStateBuilder &compareOpLessOrEqual() override;

    StencilOpStateBuilder &compareOpGreater() override;

    StencilOpStateBuilder &compareOpGreaterOrEqual() override;

    StencilOpStateBuilder &compareOpNotEqual() override;

    StencilOpStateBuilder &compareOpAlways() override;

    StencilOpStateBuilder& compareMask(uint32_t value);

    StencilOpStateBuilder& writeMask(uint32_t value);

    StencilOpStateBuilder& reference(uint32_t value);

    VkStencilOpState buildStencilOpState();

    VkStencilOpState _stencilOpState;
};