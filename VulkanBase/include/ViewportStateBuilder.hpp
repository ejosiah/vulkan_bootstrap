#pragma once

#include "GraphicsPipelineBuilder.hpp"

class ViewportBuilder;
class ScissorBuilder;

class ViewportStateBuilder : public GraphicsPipelineBuilder{
public:
    ViewportStateBuilder(VulkanDevice* device, GraphicsPipelineBuilder* builder);

    explicit ViewportStateBuilder(ViewportStateBuilder* parent);

    ~ViewportStateBuilder();

    virtual ViewportBuilder& viewport();

    virtual ScissorBuilder& scissor();

    VkPipelineViewportStateCreateInfo& buildViewportState();

protected:
    ViewportBuilder* _viewportBuilder{ nullptr };
    ScissorBuilder* _scissorBuilder{ nullptr };
    VkPipelineViewportStateCreateInfo _info{};
};

class ViewportBuilder : public ViewportStateBuilder{
public:
    explicit ViewportBuilder(ViewportStateBuilder* builder);

    ViewportBuilder& origin(float xValue, float yValue);

    ViewportBuilder& x(float value);

    ViewportBuilder& y(float value);

    ViewportBuilder& width(float value);

    ViewportBuilder& height(float value);

    ViewportBuilder& dimension(VkExtent2D dim);

    ViewportBuilder& minDepth(float value);

    ViewportBuilder& maxDepth(float value);

    ViewportBuilder& add();

    ViewportStateBuilder* parent();

    ViewportBuilder &viewport() override;

    ScissorBuilder &scissor() override;

    void checkpoint();

    void resetScratchpad();

    [[nodiscard]]
    bool ready() const;

    std::vector<VkViewport>& buildViewports();

private:
    std::vector<VkViewport> _viewports{};
    VkViewport _scratchpad{};
};

class ScissorBuilder : public ViewportStateBuilder{
public:
    explicit ScissorBuilder(ViewportStateBuilder* builder);

    ScissorBuilder& offset(int32_t x, int32_t y);

    ScissorBuilder& extent(int32_t width, int32_t height);

    ScissorBuilder& extent(VkExtent2D value);

    ScissorBuilder& add();

    void resetScratchpad();

    std::vector<VkRect2D>& buildScissors();

    [[nodiscard]]
    bool ready() const;

    void checkpoint();

    ViewportBuilder &viewport() override;

    ScissorBuilder &scissor() override;

    ViewportStateBuilder* parent();

private:
    std::vector<VkRect2D> _scissors;
    VkRect2D _scratchpad{};
};