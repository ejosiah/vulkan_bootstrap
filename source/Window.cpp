#include "Window.h"


void Window::onKeyPress(GLFWwindow *window, int key, int scancode, int action, int mods) {
    auto& self = getSelf(window);
    auto& keyEvent = self.keyEvent;

    // TODO move to input mgr;
    if(key == GLFW_KEY_ESCAPE){
        glfwSetWindowShouldClose(window, GLFW_TRUE);
        return;
    }

    keyEvent.modifierFlags = KeyModifierFlagBits::NONE;
    if(mods & GLFW_MOD_SHIFT){
        keyEvent.modifierFlags |= KeyModifierFlagBits::SHIFT;
    }
    if(mods & GLFW_MOD_ALT) {
        keyEvent.modifierFlags |= KeyModifierFlagBits::ALT;
    }
    if(mods & GLFW_MOD_CAPS_LOCK){
        keyEvent.modifierFlags |= KeyModifierFlagBits::CAPS_LOCK;
    }
    if(mods & GLFW_MOD_CONTROL){
        keyEvent.modifierFlags |= KeyModifierFlagBits::CONTROL;
    }
    if(mods & GLFW_MOD_NUM_LOCK){
        keyEvent.modifierFlags |= KeyModifierFlagBits::NUM_LOCK;
    }
    if(mods & GLFW_MOD_SUPER){
        keyEvent.modifierFlags |= KeyModifierFlagBits::SUPER;
    }
    keyEvent.key = static_cast<Key>(key);

    if(action == GLFW_PRESS){
        self.fireKeyPress(keyEvent);
    }else if(action == GLFW_RELEASE){
        self.fireKeyRelease(keyEvent);
    }else if(action == GLFW_REPEAT){
        // TODO implement
    }
}

void Window::onResize(GLFWwindow *window, int width, int height) {
    Window* self = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
    self->resized = true;
    self->width = width;
    self->height = height;
}

void Window::initWindow() {
    if(!glfwInit()){
        throw std::runtime_error{"Failed to initialize GFLW"};
    }
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWmonitor* monitor = nullptr;
    if(fullscreen) {
        auto mainMonitor = glfwGetPrimaryMonitor();
        int numMonitors;
        GLFWmonitor **monitors = glfwGetMonitors(&numMonitors);
        for (int i = 0; i < numMonitors; i++) {
            if (monitors[i] == mainMonitor) {
                monitor = monitors[i];
                break;
            }
        }
        auto vidMode = glfwGetVideoMode(monitor);
        width = vidMode->width;
        height = vidMode->height;
    }


    window = glfwCreateWindow(width, height, title.data(), monitor, nullptr);
    glfwSetWindowUserPointer(window, this);
    glfwSetKeyCallback(window, onKeyPress);
    glfwSetFramebufferSizeCallback(window, onResize);
}
