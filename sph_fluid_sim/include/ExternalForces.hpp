#pragma once

#include "ComputePipelins.hpp"
#include "VulkanDevice.h"

struct ForceDescriptor{

    ForceDescriptor() = default;

    ForceDescriptor(VulkanDevice *device, VulkanDescriptorPool* descriptorPool);

    void init();

    void createDescriptorSetLayout();

    void setNumParticles(uint32_t numParticles);

    void createDescriptorSet();

    int numParticles{0};
    VkDescriptorSet set{ VK_NULL_HANDLE };
    VulkanDescriptorSetLayout layoutSet;
    VulkanBuffer forceBuffer;
    VulkanDescriptorPool* descriptorPool{ nullptr };
    VulkanDevice *device{ nullptr };
};

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