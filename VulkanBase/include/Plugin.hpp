#pragma once
#include "common.h"
#include "events.h"
#include "VulkanInstance.h"
#include "VulkanDevice.h"
#include "VulkanSwapChain.h"
#include "Window.h"


struct PluginData{
    VulkanInstance* instance = nullptr;
    VulkanDevice* device = nullptr;
    VulkanRenderPass* renderPass = nullptr;
    VulkanSwapChain* swapChain = nullptr;
    GLFWwindow * window = nullptr;
    uint32_t* currentImageIndex = nullptr;
    VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;
};

class Plugin{
public:
    virtual ~Plugin() = default;

    void set(PluginData data);

    [[nodiscard]]
    virtual std::vector<const char*> instanceExtensions() const {
        return {};
    }

    [[nodiscard]]
    virtual std::vector<const char*> validationLayers() const {
        return {};
    }

    [[nodiscard]]
    virtual std::vector<const char*> deviceExtensions() const {
        return {};
    }

    [[nodiscard]]
    virtual void* nextChain() const {
        return nullptr;
    }

    virtual void preInit() {

    }

    virtual void init() = 0;

    [[nodiscard]]
    virtual std::string name() const = 0;

    virtual MouseClickListener mouseClickListener(){
        return [](const MouseEvent&){};
    };

    virtual MousePressListener mousePressListener(){
        return [](const MouseEvent&){};
    };

    virtual MouseReleaseListener mouseReleaseListener(){
        return [](const MouseEvent&){};
    };

    virtual MouseMoveListener mouseMoveListener(){
        return [](const MouseEvent&){};
    };

    virtual MouseWheelMovedListener mouseWheelMoveListener(){
        return [](const MouseEvent&){};
    };

    virtual KeyPressListener keyPressListener(){
        return [](const KeyEvent&){};
    };

    virtual KeyReleaseListener keyReleaseListener(){
        return [](const KeyEvent&){};
    };

    virtual WindowResizeListener windowResizeListener(){
        return [](const ResizeEvent&){};
    };

    virtual void draw(VkCommandBuffer commandBuffer){}

    virtual std::vector<VkCommandBuffer> draw(VkCommandBufferInheritanceInfo inheritanceInfo){
        return {};
    };

    virtual void update(float time) {}

    virtual void newFrame() {};

    virtual void cleanup(){};

    virtual void onSwapChainDispose(){};

    virtual void onSwapChainRecreation(){}

protected:
    PluginData data{};
    Settings settings{};
};