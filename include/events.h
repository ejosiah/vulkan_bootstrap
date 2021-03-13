#pragma once

#include <glm/vec2.hpp>
#include <functional>
#include "common.h"
#include "keys.h"

using KeyModiferFlags = Flags;

enum KeyModifierFlagBits : unsigned int{
    NONE = 0,
    SHIFT = 0x0001,
    CONTROL = 0x0002,
    ALT = 0x0004,
    SUPER = 0x0008,
    CAPS_LOCK = 0x0016,
    NUM_LOCK = 0x0032
};

struct KeyEvent{
    Key key;
    KeyModiferFlags modifierFlags;

    [[nodiscard]]
    inline int getCode() const{
        return static_cast<int>(key);
    }

    [[nodiscard]]
    inline int getChar() const{
        return static_cast<char>(key);
    }
};

struct MouseEvent{
    enum class Button{
        NONE,
        LEFT,
        MIDDLE,
        RIGHT,
    };

    enum class State{
        PRESS,
        RELEASE,
        NONE
    };

    Button button;
    State state;
    glm::vec2 pos;
    int scrollAmount;
};

struct ResizeEvent{
    glm::vec2 extent;
};

struct ShutdownEvent;

using KeyPressListener = std::function<void(const KeyEvent&)>;
using KeyReleaseListener = KeyPressListener;
using MouseMoveListener = std::function<void(const MouseEvent&)>;
using MouseClickListener = MouseMoveListener;
using WindowResizeListener = std::function<void(const ResizeEvent&)>;
using ShutdownEventListener = std::function<void(const ShutdownEvent&)>;