#pragma once
#include "VulkanImage.h"
#include "VulkanPipelineLayout.h"
#include "VulkanRAII.h"
#include "VulkanDevice.h"
#include "VulkanShaderModule.h"
#include "VulkanSwapChain.h"
#include "VulkanBaseApp.h"

struct Canvas{

    Canvas() = default;

    Canvas(const VulkanBaseApp* application, VkImageUsageFlags usage, VkFormat fmt = VK_FORMAT_R32G32B32A32_SFLOAT);

    Canvas& init();

    void createBuffer();

    void recreate();

    void disposeImage();

    void draw(VkCommandBuffer commandBuffer);

    void createDescriptorSetLayout();

    void createDescriptorPool();

    void createDescriptorSet();

    void createPipeline();

    void createImageStorage();

    VulkanBuffer buffer;
    VulkanBuffer colorBuffer;

    const VulkanBaseApp* app;
    VkImageUsageFlags usageFlags = VK_IMAGE_USAGE_SAMPLED_BIT;
    VulkanImage image;
    VulkanImageView imageView;
    VulkanSampler sampler;
    VkFormat format = VK_FORMAT_UNDEFINED;

    std::vector<VkDescriptorSetLayoutBinding> layoutBindings;
    VulkanPipelineLayout pipelineLayout;
    VulkanPipeline pipeline;
    VulkanDescriptorSetLayout descriptorSetLayout;
    VulkanDescriptorPool descriptorPool;
    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
};