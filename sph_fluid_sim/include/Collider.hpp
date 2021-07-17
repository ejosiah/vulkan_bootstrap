#pragma once

#include "ComputePipelins.hpp"
#include "VulkanDevice.h"
#include "model.hpp"

struct Collider : public ComputePipelines {

    Collider() = default;

    Collider(VulkanDevice* device,
             VulkanDescriptorPool* descriptorPool,
             VulkanDescriptorSetLayout* particleDescriptorSetLayout,
             BoxSurface domain, float radius = 1.0f);

    void init();

    void createBuffer();

    std::vector<PipelineMetaData> pipelineMetaData() override;

    void setNumParticles(uint32_t numParticles);

    void createDescriptorSetLayout();

    void createDescriptorSet();

    void operator()(VkCommandBuffer commandBuffer, VkDescriptorSet particleDescriptorSet);

    VulkanDescriptorPool *descriptorPool{nullptr};
    VulkanDescriptorSetLayout *particleDescriptorSetLayout{nullptr};

    VulkanBuffer buffer;
    VulkanDescriptorSetLayout surfaceSetLayout;
    VkDescriptorSet surfaceDescriptorSet{VK_NULL_HANDLE};
    BoxSurface domain;
    struct {
        uint32_t numParticles{0};
        float radius{0};
        float restitutionCoefficient{1};
        float frictionCoefficient{1};
    } constants;
};