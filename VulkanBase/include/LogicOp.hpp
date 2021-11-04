#pragma once
#include <vulkan/vulkan.h>

template <typename Caller>
struct LogicOp{
    explicit LogicOp(Caller* caller = nullptr)
    : _caller{caller}
    , logicOp{VK_LOGIC_OP_CLEAR}
    , enabled{ false }
    {}

    Caller& enable(){
        enabled = true;
        return *_caller;
    }

    Caller& disable(){
        enabled = false;
        return *_caller;
    }

    Caller& Clear() {
        logicOp = VK_LOGIC_OP_CLEAR;
        return *_caller;
    }

    Caller& And(){
        logicOp = VK_LOGIC_OP_AND;
        return *_caller;
    }

    Caller& AndReverse(){
        logicOp = VK_LOGIC_OP_AND_REVERSE;
        return *_caller;
    }

    Caller& Copy(){
        logicOp = VK_LOGIC_OP_COPY;
        return *_caller;
    }

    Caller& AndInverted(){
        logicOp = VK_LOGIC_OP_AND_INVERTED;
        return *_caller;
    }

    Caller& NoOp(){
        logicOp = VK_LOGIC_OP_NO_OP;
        return *_caller;
    }

    Caller& Xor(){
        logicOp = VK_LOGIC_OP_XOR;
        return *_caller;
    }

    Caller& Or(){
        logicOp = VK_LOGIC_OP_OR;
        return *_caller;
    }

    Caller& Equivalent(){
        logicOp = VK_LOGIC_OP_EQUIVALENT;
        return *_caller;
    }

    Caller& Invert(){
        logicOp = VK_LOGIC_OP_INVERT;
        return *_caller;
    }

    Caller& OrReverse(){
        logicOp = VK_LOGIC_OP_OR_REVERSE;
        return *_caller;
    }

    Caller& CopyInverted(){
        logicOp = VK_LOGIC_OP_COPY_INVERTED;
        return *_caller;
    }

    Caller& OrInverted(){
        logicOp = VK_LOGIC_OP_OR_INVERTED;
        return *_caller;
    }

    Caller& Nand(){
        logicOp = VK_LOGIC_OP_NAND;
        return *_caller;
    }

    Caller& Set(){
        logicOp = VK_LOGIC_OP_SET;
        return *_caller;
    }

    VkLogicOp logicOp;
    bool enabled;
private:
    Caller* _caller;
};