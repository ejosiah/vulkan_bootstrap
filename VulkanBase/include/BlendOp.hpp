#pragma once

#include <vulkan/vulkan.h>

template<typename Caller>
struct BlendOp{
    explicit BlendOp(Caller* caller = nullptr)
    : _caller{ caller }
    , blendOp{VK_BLEND_OP_ADD}
    {
    }

    Caller& add(){
        blendOp = VK_BLEND_OP_ADD;
        return *_caller;
    }

    Caller& subtract(){
        blendOp = VK_BLEND_OP_SUBTRACT;
        return *_caller;
    }

    Caller& reverseSubtract(){
        blendOp = VK_BLEND_OP_REVERSE_SUBTRACT;
        return *_caller;
    }

    Caller& min(){
        blendOp = VK_BLEND_OP_MIN;
        return *_caller;
    }

    Caller& max(){
        blendOp = VK_BLEND_OP_MAX;
        return *_caller;
    }

    Caller* _caller;
    VkBlendOp blendOp;
};