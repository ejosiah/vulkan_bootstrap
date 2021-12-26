#pragma once

#include "common.h"
#include "VulkanDevice.h"
#include "VulkanSwapChain.h"
#include <entt/entt.hpp>
#include "VulkanDrawable.hpp"
#include "ImGuiPlugin.hpp"

class Scene {
public:
    friend class RotationDemo;

    Scene(VulkanDevice* device = nullptr, VulkanRenderPass* renderPass = nullptr, VulkanSwapChain* swapChain = nullptr)
    {
        set(device, renderPass, swapChain);
    }

    void set(VulkanDevice* device, VulkanRenderPass* renderPass, VulkanSwapChain* swapChain){
        m_device = device;
        m_renderPass = renderPass;
        m_swapChain = swapChain;
    }

    virtual void init() = 0;

    virtual void update(float dt) = 0;


    virtual void render(VkCommandBuffer commandBuffer) = 0;

    virtual void renderUI(VkCommandBuffer commandBuffer) = 0;

    virtual void createRenderPipeline() = 0;

    virtual void refresh(){
        createRenderPipeline();
    }

protected:
    VulkanDevice* m_device{nullptr};
    VulkanRenderPass* m_renderPass{nullptr};
    VulkanSwapChain* m_swapChain{nullptr};
    entt::registry m_registry;
    struct {
        VulkanPipelineLayout layout;
        VulkanPipeline pipeline;
    } m_render;

};