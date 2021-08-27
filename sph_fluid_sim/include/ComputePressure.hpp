#pragma once

#include "ComputePipelins.hpp"
#include "VulkanDevice.h"


struct ComputePressure : public ComputePipelines{

    ComputePressure() = default;

    ComputePressure(VulkanDevice* device,
                    VulkanDescriptorSetLayout* particleLayout,
                    float eosExponent, float targetDensity,
                    float speedOfSound, float negativePressureScale);

    std::vector<PipelineMetaData> pipelineMetaData() final;

    void init();

    void setNumParticles(int numParticles);

    void operator ()(VkCommandBuffer commandBuffer, VkDescriptorSet particleDescriptorSet);

    VulkanDescriptorSetLayout* particleLayout{nullptr};

    struct {
        int numParticles{0};
        float eosExponent{7};
        float targetDensity{1000};
        float speedOfSound{100};
        float negativePressureScale{0};
    } constants;
};