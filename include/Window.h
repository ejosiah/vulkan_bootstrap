#pragma once

#include "common.h"
#include "keys.h"
#include "events.h"

class Window {
public:
    Window(std::string_view title, int width, int height, bool fullscreen = false)
    : title(title)
    , width(width)
    , height(height)
    , fullscreen(fullscreen)
    {}

    inline void addMouseClickListener(MouseClickListener&& listener){
        mouseClickListeners.push_back(listener);
    }

    inline void addMouseMoveListener(MouseMoveListener&& listener){
        mouseMoveListeners.push_back(listener);
    }

    inline void addKeyPressListener(KeyPressListener&& listener){
        keyPressListeners.push_back(listener);
    }

    inline void addKeyReleaseListener(KeyReleaseListener&& listener){
        keyReleaseListeners.push_back(listener);
    }

    inline void addWindowResizeListeners(WindowResizeListener&& listener){
        windowResizeListeners.push_back(listener);
    }

protected:
    virtual void initWindow();

    inline void fireWindowResize(const ResizeEvent& event){
        for(auto& listener : windowResizeListeners){
            listener(event);
        }
    }

    inline void fireMouseClick(const MouseEvent& event){
        for(auto& listener : mouseClickListeners){
            listener(event);
        }
    }

    inline void fireMouseMove(const MouseEvent& event){
        for(auto& listener : mouseMoveListeners){
            listener(event);
        }
    }

    inline void fireKeyPress(const KeyEvent& event){
        for(auto& listener : keyPressListeners){
            listener(event);
        }
    }

    inline void fireKeyRelease(const KeyEvent& event){
        for(auto& listener : keyReleaseListeners){
            listener(event);
        }
    }

    static void onError(int error, const char* msg);

    static void onResize(GLFWwindow* window, int width, int height);

    static void onMouseClick(GLFWwindow* window, int button, int action, int mods);

    static void onMouseMove(GLFWwindow* window, double x, double y);

    static void onKeyPress(GLFWwindow* window, int key, int scancode, int action, int mods);

    static Window& getSelf(GLFWwindow* window){
        return *reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
    }

protected:
    std::string title;
    int width;
    int height;
    bool resized = false;
    bool fullscreen;
    GLFWwindow* window = nullptr;
    MouseEvent mouseEvent{};
    KeyEvent keyEvent{};
    ResizeEvent resizeEvent{};

private:
    std::vector<MouseClickListener> mouseClickListeners;
    std::vector<MouseMoveListener> mouseMoveListeners;
    std::vector<KeyPressListener> keyPressListeners;
    std::vector<KeyReleaseListener> keyReleaseListeners;
    std::vector<WindowResizeListener> windowResizeListeners;
};
