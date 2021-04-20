#include "Window.h"


void Window::onKeyPress(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if(glfwWindowShouldClose(window)) return;

    auto& self = getSelf(window);
    auto& keyEvent = self.keyEvent;

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
    if(glfwWindowShouldClose(window)) return;

    auto& self = getSelf(window);
    self.resized = true;
    self.width = width;
    self.height = height;
    self.resizeEvent.extent.x = static_cast<uint32_t>(width);
    self.resizeEvent.extent.y = static_cast<uint32_t>(height);
    self.fireWindowResize(self.resizeEvent);
}

void Window::initWindow() {
    glfwSetErrorCallback(onError);
    if(!glfwInit()){
        throw std::runtime_error{"Failed to initialize GFLW"};
    }
    if(!glfwVulkanSupported()){
        throw std::runtime_error("Vulkan Not supported");
    }
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    if(fullscreen){
        monitor = glfwGetPrimaryMonitor();
        auto mode = glfwGetVideoMode(monitor);
        if(videoMode.width == 0 || videoMode.height == 0 || videoMode.refreshRate == 0) {
            videoMode.width = width = mode->width;
            videoMode.height = height = mode->height;
            videoMode.refreshRate = mode->refreshRate;
        }
    }

    if(!fullscreen){
        int count;
        auto* modes = glfwGetVideoModes(glfwGetPrimaryMonitor(), &count);
        int maxHeight = std::numeric_limits<int>::min();
        int maxWidth = std::numeric_limits<int>::min();

        for(auto i = 0; i < count; i++){
            auto& mode = modes[i];
            maxWidth = std::max(maxWidth, mode.width);
            maxHeight = std::max(maxHeight, mode.height);
        }
        if(width > maxWidth) width = maxWidth;
        if(height > maxHeight) height = maxHeight;
    }

    window = glfwCreateWindow(width, height, title.data(), monitor, nullptr);

    glfwSetWindowUserPointer(window, this);
    glfwSetKeyCallback(window, onKeyPress);
    glfwSetMouseButtonCallback(window, onMouseClick);
    glfwSetCursorPosCallback(window, onMouseMove);
    glfwSetFramebufferSizeCallback(window, onResize);
    glfwSetScrollCallback(window, onMouseWheelMove);
    glfwSetCursorEnterCallback(window, onCursorEntered);
}

void Window::onMouseClick(GLFWwindow *window, int button, int action, int mods) {
    if(glfwWindowShouldClose(window)) return;

    auto& self = getSelf(window);
    auto& mouseEvent = self.mouseEvent;

    switch(button){
        case GLFW_MOUSE_BUTTON_LEFT:
            mouseEvent.button = MouseEvent::Button::LEFT;
            break;
        case GLFW_MOUSE_BUTTON_RIGHT:
            mouseEvent.button = MouseEvent::Button::RIGHT;
            break;
        case GLFW_MOUSE_BUTTON_MIDDLE:
            mouseEvent.button = MouseEvent::Button::MIDDLE;
            break;
        default:
            mouseEvent.button = static_cast<MouseEvent::Button>(button);
    }

    switch(action){
        case GLFW_PRESS:
            mouseEvent.state = MouseEvent::State::PRESS;
            break;
        case GLFW_RELEASE:
            mouseEvent.state = MouseEvent::State::RELEASE;
            break;
        default:
            mouseEvent.state = MouseEvent::State::NONE;
    }

    if(mouseEvent.state == MouseEvent::State::PRESS){
        self.fireMousePress(mouseEvent);
    }else if(mouseEvent.state == MouseEvent::State::RELEASE){
        self.fireMouseRelease(mouseEvent);
        self.fireMouseClick(mouseEvent);
    }
}

void Window::onMouseMove(GLFWwindow *window, double x, double y) {
    if(glfwWindowShouldClose(window)) return;

    auto& self = getSelf(window);
    self.mouseEvent.pos.x = static_cast<float>(x);
    self.mouseEvent.pos.y = static_cast<float>(y);
    self.fireMouseMove(self.mouseEvent);
}

void Window::onError(int error, const char* msg) {
    spdlog::error("GLFW: id: {}, msg: {}", error, msg);
}

void Window::onMouseWheelMove(GLFWwindow *window, double xOffset, double yOffset) {
    if(glfwWindowShouldClose(window)) return;

    auto& self = getSelf(window);
    auto& mouseEvent = self.mouseEvent;
    mouseEvent.scrollOffset.x = xOffset;
    mouseEvent.scrollOffset.y = yOffset;
    self.fireMouseWheelMove(mouseEvent);
}

void Window::onCursorEntered(GLFWwindow *window, int entered) {
    if(glfwWindowShouldClose(window)) return;
}

bool Window::setFullScreen() {
    if(fullscreen) return false;
    if(!monitor){
        monitor = glfwGetPrimaryMonitor();
        auto mode = glfwGetVideoMode(monitor);
        videoMode.width = mode->width;
        videoMode.height = mode->height;
        videoMode.refreshRate = mode->refreshRate;
    }
    prevWidth = width;
    prevHeight = height;
    width = videoMode.width;
    height = videoMode.height;
    fullscreen = true;
    glfwSetWindowMonitor(window, monitor, 0, 0, width, height, videoMode.refreshRate);
    return true;
}

bool Window::unsetFullScreen() {
    if(!fullscreen || prevWidth == 0 || prevHeight == 0) return false;
    width = prevWidth;
    height = prevHeight;
    prevWidth = 0;
    prevHeight = 0;
    fullscreen = false;
    glfwSetWindowMonitor(window, nullptr, 0, 0, width, height, 0);
    return true;
}

bool Window::isShuttingDown() const{
    return glfwWindowShouldClose(window);
}
