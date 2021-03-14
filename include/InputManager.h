#pragma once

#include "common.h"
#include "keys.h"
#include "events.h"
#include "Window.h"

struct Action{
    enum class Behavior{
        NORMAL, DETECT_INITIAL_PRESS_ONLY
    };

    enum class State{
        RELEASED,
        PRESSED,
        HELD
    };

    explicit Action(std::string_view name = "")
    : name(name)
    , behavior(Behavior::NORMAL)
    , state(State::RELEASED)
    , amount(0)
    {}

    std::string name;
    Behavior behavior;
    State state;
    int amount;

    inline void reset(){
        state = State::RELEASED;
        amount = 0;
    }

    inline void press(){
        press(1);
    }

    inline void press(int amt){
        if(state != State::HELD){
            this->amount += amt;
            state = State::PRESSED;
        }
    }

    inline void release(){
        state = State::RELEASED;
    }

    inline bool isPressed(){
        return getAmount() != 0;
    }

    inline int getAmount(){
        int res = amount;
        if(res != 0){
            if(state == State::RELEASED){
                amount = 0;
            }else if(behavior == Behavior::DETECT_INITIAL_PRESS_ONLY){
                state = State::HELD;
                amount = 0;
            }
        }
        return res;
    }

    bool operator==(const Action& other) const {
        return other.name == name;
    }
};

struct Mouse{
    glm::vec2 position{0};
    glm::vec2 relativePosition{0};
};

class InputManager{
protected:
    InputManager(bool relativeMouseMode = false)
    : relativeMouseMode(relativeMouseMode)
    {}

    void initInputMgr(Window& window){
        window.addKeyPressListener([&](const KeyEvent& e){ onKeyPress(e); });
        window.addKeyReleaseListener([&](const KeyEvent& e){ onKeyRelease(e); });
        window.addMouseMoveListener([&](const MouseEvent& e){ onMouseMove(e); });
        window.addMousePressListener([&](const MouseEvent& e){ onMousePress(e); });
        window.addMouseReleaseListener([&](const MouseEvent& e){ onMouseRelease(e); });
        window.addWindowResizeListeners([&](const ResizeEvent& e){ onResize(e); });
        window.addMouseWheelMoveListener([&](const MouseEvent& e){ onMouseWheelMove(e); });

        glfwWindow = window.getGlfwWindow();
        if(relativeMouseMode){
            glfwSetInputMode(glfwWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            int w, h;
            glfwGetFramebufferSize(glfwWindow, &w, &h);
            spdlog::info("framebuffer size [{}, {}]", w, h);
            setCenter(w * 0.5f, h * 0.5f);
            recenter();
        }
    }

    Action& mapToKey(Key key){
        int index = static_cast<int>(key);
        keyActions.insert(std::make_pair(index, Action{}));

        return keyActions[index];
    }

    Action& mapToMouse(int mouseCode) { // TODO use enum
        mouseActions.insert(std::make_pair(mouseCode, Action{}));

        return mouseActions[mouseCode];
    }

    void clear(Action& action){
        std::vector<int> keys;
        for(auto& [key, mappedAction] : keyActions){
            if(action == mappedAction){
                keys.push_back(key);
            }
        }
        for(auto key : keys) keyActions.erase(key);

        keys.clear();
        for(auto& [key, mappedAction] : mouseActions){
            if(action == mappedAction){
                keys.push_back(key);
            }
        }
        for(auto key : keys) mouseActions.erase(key);

        action.reset();
    }

    std::vector<std::string_view> getMappings(const Action& action){
        std::vector<std::string_view> mappings;

        // TODO return key names for mappings
        return mappings;
    }

    void clear(){
        keyActions.clear();
        mouseActions.clear();
    }

    void resetAllActions(){
        for(auto& [_, action] : keyActions) action.reset();
        for(auto& [_, action] : mouseActions) action.reset();
    }

    void onKeyPress(const KeyEvent& event){
        int key = static_cast<int>(event.key);
        auto itr = keyActions.find(key);
        if(itr != end(keyActions)){
            itr->second.press();
        }
        spdlog::info("key pressed: {}", static_cast<char>(key));
    }

    void onKeyRelease(const KeyEvent& event){
        int key = static_cast<int>(event.key);
        auto itr = keyActions.find(key);
        if(itr != end(keyActions)){
            itr->second.release();
        }
        spdlog::info("key released: {}", static_cast<char>(key));
    }

    void onMousePress(const MouseEvent& event){

        int key = static_cast<int>(event.button);
        auto itr = mouseActions.find(key);
        if(itr != end(mouseActions)){
            itr->second.press();
        }
//        spdlog::info("mouse button pressed: {}", static_cast<char>(key));
    }

    void onMouseRelease(const MouseEvent& event){
        int key = static_cast<int>(event.button);
        spdlog::info("mouse button released: {}", static_cast<char>(key));
        auto itr = mouseActions.find(key);
        if(itr != end(mouseActions)){
            itr->second.release();
        }
    }

    void onResize(const ResizeEvent& event){
        setCenter(event.extent.x * 0.5f, event.extent.y * 0.5f);
 //       spdlog::info("window resized [{}, {}]", event.extent.x, event.extent.y);
    }

    void setCenter(float x, float y){
        spdlog::error("set center to [{}, {}]", x, y);
        center.x = x;
        center.y = y;
    }

    void onMouseMove(const MouseEvent& event){
        mouse.position.x = event.pos.x;
        mouse.position.y = event.pos.y;
        mouse.relativePosition.x += event.pos.x - center.x;
        mouse.relativePosition.y += center.y - event.pos.y;

//        spdlog::info("mouse move pos: [{}, {}], relativePos: [{}, {}]"
//                     , mouse.position.x
//                     , mouse.position.y
//                     , mouse.relativePosition.x
//                     , mouse.relativePosition.y);
        mouseHelper(MouseEvent::MoveCode::LEFT, MouseEvent::MoveCode::RIGHT, mouse.relativePosition.x);
        mouseHelper(MouseEvent::MoveCode::DOWN, MouseEvent::MoveCode::UP, mouse.relativePosition.y);
    }

    void onMouseWheelMove(const MouseEvent& event){
        spdlog::info("mouse wheel moved xOffset: {}, yOffset: {}", event.scrollOffset.x, event.scrollOffset.y);
        mouseHelper(MouseEvent::MoveCode::WHEEL_DOWN, MouseEvent::MoveCode::WHEEL_UP, event.scrollOffset.y);
    }

    void setRelativeMouseMode(bool mode){
        relativeMouseMode = mode;
        assert(glfwWindow);
        if(relativeMouseMode){
            glfwSetInputMode(glfwWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }else{
            glfwSetInputMode(glfwWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }


    void recenter(){
        assert(glfwWindow);
        if(relativeMouseMode){
            glfwSetCursorPos(glfwWindow, center.x, center.y);
            mouse.position.x = center.x;
            mouse.position.y = center.y;
            mouse.relativePosition.x = 0;
            mouse.relativePosition.y = 0;
        }
    }

    void mouseHelper(int codeNeg, int codePos, int amount){
        Action* action = nullptr;
        if(amount < 0){
            auto itr = mouseActions.find(codeNeg);
            if(itr != end(mouseActions)){
                action = &itr->second;
            }
        } else{
            auto itr = mouseActions.find(codePos);
            if(itr != end(mouseActions)){
                action = &itr->second;
            }
        }
        if(action){
            action->press(std::abs(amount));
            action->release();
        }
    }

    std::map<int, Action> keyActions;
    std::map<int, Action> mouseActions;
    glm::vec2 center{0};
    bool relativeMouseMode;
    Mouse mouse;
private:
    GLFWwindow* glfwWindow = nullptr;
};