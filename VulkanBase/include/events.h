#pragma once

#include <glm/vec2.hpp>
#include <functional>
#include "common.h"
#include "keys.h"

using KeyModiferFlags = Flags;

enum KeyModifierFlagBits : unsigned int{
    NONE = 0,
    SHIFT = GLFW_MOD_SHIFT,
    CONTROL = GLFW_MOD_CONTROL,
    ALT = GLFW_MOD_ALT,
    SUPER = GLFW_MOD_SUPER,
    CAPS_LOCK = GLFW_MOD_CAPS_LOCK,
    NUM_LOCK = GLFW_MOD_NUM_LOCK
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

const KeyPressListener EMPTY_KEY_PRESS_LISTENER = [](const auto& _){};
const KeyPressListener EMPTY_KEY_RELEASE_LISTENER = EMPTY_KEY_PRESS_LISTENER;
const MouseMoveListener EMPTY_MOUSE_MOVE_LISTENER = [](const auto& _){};
const MouseClickListener EMPTY_MOUSE_CLICK_LISTENER = EMPTY_MOUSE_MOVE_LISTENER;
const MousePressListener EMPTY_MOUSE_PRESS_LISTENER = EMPTY_MOUSE_MOVE_LISTENER;
const MouseReleaseListener EMPTY_MOUSE_RELEASE_LISTENER = EMPTY_MOUSE_MOVE_LISTENER;
const MouseWheelMovedListener EMPTY_MOUSE_WHEEL_MOVED_LISTENER = EMPTY_MOUSE_MOVE_LISTENER;
const WindowResizeListener EMPTY_WINDOW_RESIZE_LISTENER = [](const auto& _){};
const ShutdownEventListener EMPTY_SHUTDOWN_EVENT_LISTENER = [](const auto& _){};