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
    enum class Button : int {
        ONE = 0,
        TWO = 1,
        THREE = 2,
        FOUR = 3,
        FIVE = 4,
        SIX = 5,
        SEVEN = 6,
        EIGHT = 7,

        LEFT = 0,
        RIGHT = 1,
        MIDDLE = 2,

        NONE = std::numeric_limits<int>::max(),
    };

    enum MoveCode : int {
        LEFT = 8,
        RIGHT = 9,
        UP = 10,
        DOWN = 11,
        WHEEL_UP = 12,
        WHEEL_DOWN = 13,
    };

    enum class State{
        PRESS,
        RELEASE,
        NONE
    };

    Button button = Button::NONE;
    State state = State::NONE;
    glm::vec2 pos;
    glm::vec2 scrollOffset;

    void reset(){
        button = Button::NONE;
        state = State::NONE;
    }

    [[nodiscard]]
    bool leftButtonPressed() const {
        return button == Button::LEFT && state == State::PRESS;
    }

    [[nodiscard]]
    bool rightButtonPressed() const {
        return button == Button::RIGHT && state == State::PRESS;
    }
};

struct ResizeEvent{
    glm::vec2 extent;
};

struct ShutdownEvent;

using KeyPressListener = std::function<void(const KeyEvent&)>;
using KeyReleaseListener = KeyPressListener;
using MouseMoveListener = std::function<void(const MouseEvent&)>;
using MouseClickListener = MouseMoveListener;
using MousePressListener = MouseMoveListener;
using MouseReleaseListener = MouseMoveListener;
using MouseWheelMovedListener = MouseMoveListener;
using WindowResizeListener = std::function<void(const ResizeEvent&)>;
using ShutdownEventListener = std::function<void(const ShutdownEvent&)>;