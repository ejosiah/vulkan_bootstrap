#pragma once

#include "GraphicsPipelineBuilder.hpp"
#include "LogicOp.hpp"
#include "BlendFactor.hpp"
#include "BlendOp.hpp"

class ColorBlendAttachmentStateBuilder;

class ColorBlendStateBuilder : public GraphicsPipelineBuilder{
public:
    ColorBlendStateBuilder(VulkanDevice* device, GraphicsPipelineBuilder* parent);

    ~ColorBlendStateBuilder() override;

    explicit ColorBlendStateBuilder(ColorBlendStateBuilder* parent);

    ColorBlendStateBuilder& blendConstants(float r, float g, float b, float a);

    virtual ColorBlendAttachmentStateBuilder& attachment();

    inline LogicOp<ColorBlendStateBuilder>& logicOp(){
        return _logicOp;
    }

    VkPipelineColorBlendStateCreateInfo& buildColorBlendState();

private:
    VkPipelineColorBlendStateCreateInfo _info{ VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
    LogicOp<ColorBlendStateBuilder> _logicOp{};
    ColorBlendAttachmentStateBuilder* _colorBlendAttachmentStateBuilder{ nullptr };
};

class ColorBlendAttachmentStateBuilder : public ColorBlendStateBuilder{
public:
    explicit ColorBlendAttachmentStateBuilder(ColorBlendStateBuilder* parent);

    ColorBlendAttachmentStateBuilder& enableBlend();

    ColorBlendAttachmentStateBuilder& disableBlend();

    inline BlendFactor<ColorBlendAttachmentStateBuilder>& srcColorBlendFactor(){
        dirty();
        return _srcColorBlendFactor;
    }

    inline BlendFactor<ColorBlendAttachmentStateBuilder>& dstColorBlendFactor(){
        dirty();
        return _dstColorBlendFactor;
    }


    inline BlendFactor<ColorBlendAttachmentStateBuilder>& srcAlphaBlendFactor(){
        dirty();
        return _srcAlphaBlendFactor;
    }

    inline BlendFactor<ColorBlendAttachmentStateBuilder>& dstAlphaBlendFactor(){
        dirty();
        return _dstAlphaBlendFactor;
    }


    inline BlendOp<ColorBlendAttachmentStateBuilder>& colorBlendOp(){
        dirty();
        return _colorBlendOp;
    }

    inline BlendOp<ColorBlendAttachmentStateBuilder>& alphaBlendOp(){
        dirty();
        return _alphaBlendOp;
    }

    ColorBlendAttachmentStateBuilder& colorWriteMask(VkColorComponentFlags flags);

    ColorBlendAttachmentStateBuilder& add();

    ColorBlendAttachmentStateBuilder &attachment() override;


    std::vector<VkPipelineColorBlendAttachmentState>& buildColorBlendAttachmentState();

private:
    inline void dirty(){
        _dirty = true;
    }

    void resetScratchpad();

private:
    std::vector<VkPipelineColorBlendAttachmentState> _states;
    BlendFactor<ColorBlendAttachmentStateBuilder> _srcColorBlendFactor{};
    BlendFactor<ColorBlendAttachmentStateBuilder> _dstColorBlendFactor{};
    BlendFactor<ColorBlendAttachmentStateBuilder> _srcAlphaBlendFactor{};
    BlendFactor<ColorBlendAttachmentStateBuilder> _dstAlphaBlendFactor{};
    BlendOp<ColorBlendAttachmentStateBuilder> _colorBlendOp{};
    BlendOp<ColorBlendAttachmentStateBuilder> _alphaBlendOp{};
    VkPipelineColorBlendAttachmentState _scratchpad;
    bool _dirty = false;
};