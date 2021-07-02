#pragma once

#include "ComputePipelins.hpp"

class ParticleSystem : public ComputePipelines{
public:
    ParticleSystem() = default;

    ParticleSystem(VulkanDevice* device, VulkanDescriptorPool* descriptorPool);

    std::vector<PipelineMetaData> pipelineMetaData() override;

    void init();

    void createDescriptorSetLayouts();

    void createDescriptorSets();

    void createGraphicsPipeline();

    VulkanDevice* device;
    VulkanDescriptorPool* descriptorPool;

    VulkanPipelineLayout layout;
    VulkanPipeline pipeline;
    std::array<VulkanBuffer, 2> buffers;
    std::array<VkDescriptorSet, 2> descriptorSets;
    VulkanDescriptorSetLayout descriptorSetLayout;

    struct {
        glm::vec3 gravity{0, -9.8, 0};
        uint32_t numParticles{ 1024 };
        float drag = 1e-4;
        float time;
    } constants;
};