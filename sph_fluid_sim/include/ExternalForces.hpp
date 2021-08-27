#pragma once

#include "ComputePipelins.hpp"
#include "VulkanDevice.h"
#include "ForceDescriptor.hpp"

struct ExternalForces : public ComputePipelines{

    ExternalForces() = default;

    ExternalForces(VulkanDevice* device, ForceDescriptor* forceDescriptor, VulkanDescriptorSetLayout* particleLayout, float drag = 0.0f);

    std::vector<PipelineMetaData> pipelineMetaData() final;

    void init();

    void setNumParticles(int numParticles);

    void update(uint32_t numParticles, float time);

    void operator ()(VkCommandBuffer commandBuffer, VkDescriptorSet particleDescriptorSet);

    VulkanDescriptorSetLayout* particleLayout{nullptr};
    ForceDescriptor* forceDescriptor{nullptr };

    struct {
        glm::vec3 gravity{0, -9.8, 0};
        uint32_t numParticles{0};
        float drag{1e-4};
        float time{0.0};
        float invMass{0.0};
    } constants;
};