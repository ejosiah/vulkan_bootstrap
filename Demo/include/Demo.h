#pragma once

#include "VulkanBaseApp.h"
#include "VulkanModel.h"

struct Material{
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
    float shininess;
};

struct SpaceShip{
    VulkanBuffer vertices;
    VulkanBuffer indices;
    std::vector<vkn::Primitive> primitives;
    std::vector<Material> materials;

    static constexpr VkPushConstantRange pushConstantRange(){
        return { VK_SHADER_STAGE_FRAGMENT_BIT, 0u, sizeof(Material) };
    }
};

struct Floor{
    VulkanBuffer vertices;
    VulkanBuffer indices;
    VulkanDescriptorSetLayout descriptorSetLayout;
    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
};

class Demo : public VulkanBaseApp{
protected:
    void initApp() override;

    void loadSpaceShip();

    void loadFloor();

    void onSwapChainDispose() override;

    void onSwapChainRecreation() override;

    VkCommandBuffer *buildCommandBuffers(uint32_t imageIndex, uint32_t &numCommandBuffers) override;

    void update(float time) override;

    void checkAppInputs() override;

    void cleanup() override;

private:
    SpaceShip spaceShip;
    Floor floor;
    std::unique_ptr<BaseCameraController> cameraController;
    struct {
        VulkanPipeline spaceShip;
        VulkanPipeline floor;
    } pipelines;
    bool displayHelp = false;
};