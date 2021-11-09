#pragma once

#include "Texture.h"
#include "VulkanBaseApp.h"
#include "VulkanDevice.h"

struct SkyBox{
    Texture texture;
    struct {
        VulkanBuffer vertices;
        VulkanBuffer indices;
    } cube;

    VulkanPipeline* pipeline{ nullptr };
    VulkanPipelineLayout* layout{ nullptr };
    VkDescriptorSet descriptorSet{ VK_NULL_HANDLE };

    static void init(VulkanBaseApp* app);

    static void createPipeline();

    static void create(SkyBox& skyBox, const std::string& directory, const std::array<std::string, 6>& faceNames);

    static void updateDescriptorSet(SkyBox& skyBox);

    static void load(Texture& skyboxTexture, const std::string& directory, const std::array<std::string, 6>& faceNames);

    static VulkanDevice& device();

    static void createDescriptorSetLayout();

    static void createDescriptorPool();

    static void destroy();

private:
    static VulkanDevice* g_device;
    static VulkanPipeline g_pipeline;
    static VulkanPipelineLayout g_layout;
    static VulkanDescriptorSetLayout g_descriptorSetLayout;
    static VkDescriptorSet g_descriptorSet;
    static VulkanBaseApp* g_app;
    static VulkanDescriptorPool g_descriptorPool;
};
