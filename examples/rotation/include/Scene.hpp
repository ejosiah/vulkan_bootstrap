#pragma once

#include "common.h"
#include "VulkanDevice.h"
#include "VulkanSwapChain.h"
#include <entt/entt.hpp>
#include "VulkanDrawable.hpp"
#include "ImGuiPlugin.hpp"
#include "Entity.hpp"
#include "filemanager.hpp"
#include "GraphicsPipelineBuilder.hpp"
#include "DescriptorSetBuilder.hpp"
#include "primitives.h"
#include "components.h"
#include <glm/gtc/quaternion.hpp>

class Scene {
public:
    friend class RotationDemo;

    Scene(VulkanDevice* device = nullptr, VulkanRenderPass* renderPass = nullptr, VulkanSwapChain* swapChain = nullptr, VulkanDescriptorPool* descriptorPool = nullptr, FileManager* fileManager = nullptr, Mouse* mouse = nullptr)
    :m_fileManager(fileManager)
    , m_mouse(mouse)
    {
        set(device, renderPass, swapChain, descriptorPool);
    }

    void set(VulkanDevice* device, VulkanRenderPass* renderPass, VulkanSwapChain* swapChain, VulkanDescriptorPool* descriptorPool){
        m_device = device;
        m_renderPass = renderPass;
        m_swapChain = swapChain;
        m_descriptorPool = descriptorPool;
    }

    virtual void init() = 0;

    virtual void update(float dt) = 0;


    virtual void render(VkCommandBuffer commandBuffer) = 0;

    virtual void renderUI(VkCommandBuffer commandBuffer) = 0;

    virtual void createRenderPipeline() = 0;

    virtual void checkInputs(){

    }

    virtual void refresh() = 0;

    Entity createEntity(const std::string& name){
        Entity entity{m_registry };
        entity.add<component::Position>();
        entity.add<component::Rotation>();
        entity.add<component::Scale>();
        entity.add<component::Transform>();
        auto& nameTag = entity.add<component::Name>();
        nameTag.value = name.empty() ? fmt::format("{}_{}", "Entity", m_registry.size()) : name;
        return entity;
    }

    byte_string load(const std::string& resource){
        return m_fileManager->load(resource);
    }

protected:
    VulkanDevice* m_device{nullptr};
    VulkanRenderPass* m_renderPass{nullptr};
    VulkanSwapChain* m_swapChain{nullptr};
    VulkanDescriptorPool* m_descriptorPool{nullptr};
    FileManager* m_fileManager{nullptr};
    Mouse* m_mouse{ nullptr };
    entt::registry m_registry;
    struct {
        VulkanPipelineLayout layout;
        VulkanPipeline pipeline;
    } m_render;
};