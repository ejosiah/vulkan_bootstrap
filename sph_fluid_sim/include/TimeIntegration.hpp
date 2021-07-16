#pragma once

#include "ComputePipelins.hpp"
#include "VulkanDevice.h"

struct TimeIntegration : public ComputePipelines{

    TimeIntegration() = default;

    TimeIntegration(VulkanDevice* device, VulkanDescriptorSetLayout* particleLayout, VulkanDescriptorSetLayout*  forceLayout);

    void init();

    std::vector<PipelineMetaData> pipelineMetaData() override;

    void operator ()(VkCommandBuffer commandBuffer, std::array<VkDescriptorSet, 2>& particleDescriptorSets, VkDescriptorSet forceSet);

    void update(uint32_t numParticles, float time);

    VulkanDescriptorSetLayout* particleLayout{nullptr};
    VulkanDescriptorSetLayout* forceLayout{nullptr };

    struct {
        uint32_t numParticles{0};
        float time{0.0};
        float invMass;
    } constants;
};