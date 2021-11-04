#pragma once
#include <vulkan/vulkan.h>

template<typename Caller>
struct BlendFactor{

    explicit BlendFactor(Caller* caller = nullptr)
    : _caller{ caller }
    , blendFactor{VK_BLEND_FACTOR_ZERO }
    {}

    Caller& zero(){
        blendFactor = VK_BLEND_FACTOR_ZERO;
        return *_caller;
    }

    Caller& one(){
        blendFactor = VK_BLEND_FACTOR_ONE;
        return *_caller;
    }

    Caller& srcColor(){
        blendFactor = VK_BLEND_FACTOR_SRC_COLOR;
        return *_caller;
    }

    Caller& oneMinusSrcColor(){
        blendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
        return *_caller;
    }

    Caller& dstColor(){
        blendFactor = VK_BLEND_FACTOR_DST_COLOR;
        return *_caller;
    }

    Caller& oneMinusDstColor(){
        blendFactor = VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
        return *_caller;
    }

    Caller& srcAlpha(){
        blendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        return *_caller;
    }

    Caller& oneMinusSrcAlpha(){
        blendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        return *_caller;
    }

    Caller& dstAlpha(){
        blendFactor = VK_BLEND_FACTOR_DST_ALPHA;
        return *_caller;
    }

    Caller& oneMinusDstAlpha(){
        blendFactor = VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
    }

    Caller& constantColor(){
        blendFactor = VK_BLEND_FACTOR_CONSTANT_COLOR;
        return *_caller;
    }

    Caller& oneMinusConstantColor(){
        blendFactor = VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
        return *_caller;
    }

    Caller& constantAlpha(){
        blendFactor = VK_BLEND_FACTOR_CONSTANT_ALPHA;
        return *_caller;
    }

    Caller& oneMinusConstantAlpha(){
        blendFactor = VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA;
        return *_caller;
    }


    Caller& srcAlphaSaturate(){
        blendFactor = VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
        return *_caller;
    }

    Caller& src1Color(){
        blendFactor = VK_BLEND_FACTOR_SRC1_COLOR;
        return *_caller;
    }

    Caller& oneMinusSrc1Color(){
        blendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR;
        return *_caller;
    }
    Caller& src1Alpha(){
        blendFactor = VK_BLEND_FACTOR_SRC1_ALPHA;
        return *_caller;
    }

    Caller& oneMinusSrc1Alpha(){
        blendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA;
        return *_caller;
    }

    Caller* _caller;
    VkBlendFactor blendFactor;
};