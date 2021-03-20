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

    explicit Action(std::string_view name = "", Behavior behavior = Behavior::NORMAL)
    : name(name)
    , behavior(behavior)
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

    inline bool isHeld(){
        return state == State::HELD;
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

    static inline Behavior detectInitialPressOnly(){
        return Behavior::DETECT_INITIAL_PRESS_ONLY;
    }

    static inline Behavior normal(){
        return Behavior::NORMAL;
    }
};

struct Mouse{
    glm::vec2 position{0};
    glm::vec2 relativePosition{0};
};

class InputManager{
public:
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
            setCenter(w * 0.5f, h * 0.5f);
            recenter();
        }
    }

    Action& mapToKey(Key key, std::string_view name = "", Action::Behavior behavior = Action::Behavior::NORMAL){
        int index = static_cast<int>(key);
        keyActions.insert(std::make_pair(index, Action{name, behavior}));

        return keyActions[index];
    }

    Action& mapToMouse(int mouseCode, std::string_view name = "", Action::Behavior behavior = Action::Behavior::NORMAL) { // TODO use enum
        mouseActions.insert(std::make_pair(mouseCode, Action{name, behavior}));

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
    }

    void onKeyRelease(const KeyEvent& event){
        int key = static_cast<int>(event.key);
        auto itr = keyActions.find(key);
        if(itr != end(keyActions)){
            itr->second.release();
        }
    }

    void onMousePress(const MouseEvent& event){

        int key = static_cast<int>(event.button);
        auto itr = mouseActions.find(key);
        if(itr != end(mouseActions)){
            itr->second.press();
        }
    }

    void onMouseRelease(const MouseEvent& event){
        int key = static_cast<int>(event.button);
        auto itr = mouseActions.find(key);
        if(itr != end(mouseActions)){
            itr->second.release();
        }
    }

    void onResize(const ResizeEvent& event){
        setCenter(event.extent.x * 0.5f, event.extent.y * 0.5f);
    }

    void setCenter(float x, float y){
        center.x = x;
        center.y = y;
    }

    void onMouseMove(const MouseEvent& event){
        mouse.position.x = event.pos.x;
        mouse.position.y = event.pos.y;
        mouse.relativePosition.x += event.pos.x - center.x;
        mouse.relativePosition.y += center.y - event.pos.y;

        mouseHelper(MouseEvent::MoveCode::LEFT, MouseEvent::MoveCode::RIGHT, mouse.relativePosition.x);
        mouseHelper(MouseEvent::MoveCode::DOWN, MouseEvent::MoveCode::UP, mouse.relativePosition.y);
    }

    void onMouseWheelMove(const MouseEvent& event){
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

public:
    [[nodiscard]]
    const Mouse& getMouse() const {
        return mouse;
    }
};