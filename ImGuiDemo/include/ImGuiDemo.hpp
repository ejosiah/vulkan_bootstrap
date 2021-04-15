#pragma once

#include "VulkanBaseApp.h"
#include "ImGuiPlugin.hpp"

class ImGuiDemo : public VulkanBaseApp{
public:
    ImGuiDemo(const Settings& settings);

protected:
    void initApp() override;

    void onSwapChainDispose() override;

    void onSwapChainRecreation() override;

    VkCommandBuffer *buildCommandBuffers(uint32_t imageIndex, uint32_t &numCommandBuffers) override;

private:
    VulkanCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;
    glm::vec4 clearColor{0.45f, 0.55f, 0.60f, 1.0f};
    bool show_another_window = true;
    bool show_demo_window = true;
};