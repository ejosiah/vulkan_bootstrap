#pragma once
#include "common.h"
#include "events.h"
#include "VulkanInstance.h"
#include "VulkanDevice.h"
#include "VulkanSwapChain.h"

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

    virtual void init() = 0;

    [[nodiscard]]
    virtual std::string name() const = 0;

    [[nodiscard]]
    virtual bool willRender() const {
        return false;
    };

    virtual MouseClickListener mouseClickListener(){
        return {};
    };

    virtual MousePressListener mousePressListener(){
        return {};
    };

    virtual MouseReleaseListener mouseReleaseListener(){
        return {};
    };

    virtual MouseMoveListener mouseMoveListener(){
        return {};
    };

    virtual MouseWheelMovedListener mouseWheelMoveListener(){
        return {};
    };

    virtual KeyPressListener keyPressListener(){
        return {};
    };

    virtual KeyReleaseListener keyReleaseListener(){
        return {};
    };

    virtual WindowResizeListener windowResizeListener(){
        return {};
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