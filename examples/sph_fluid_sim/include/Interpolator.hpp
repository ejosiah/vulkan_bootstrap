#pragma once

#include "ComputePipelins.hpp"
#include "VulkanDevice.h"
#include "model.hpp"
#include "ForceDescriptor.hpp"

struct Interpolator : public ComputePipelines{

    static constexpr int DENSITY = 1 << 0;
    static constexpr int PRESSURE = 1 << 1;
    static constexpr int COLOR = 1 << 2;

    Interpolator() = default;

    Interpolator(VulkanDevice* device,
                 VulkanDescriptorSetLayout* particleDescriptorLayout,
                 ForceDescriptor* forceDescriptor,
                 VulkanDescriptorSetLayout* neighbourListLayout,
                 VulkanDescriptorSetLayout* neighbourListSizeLayout,
                 VulkanDescriptorSetLayout* neighbourListOffsetLayout,
                 float radius, float invMass);

    void init();

    std::vector<PipelineMetaData> pipelineMetaData() override;

    void setNumParticles(int numParticles);

    void setNeighbourDescriptors(VkDescriptorSet* sets);

    void updateDensity(VkCommandBuffer commandBuffer, VkDescriptorSet particleDescriptorSet);

    void updatePressure(VkCommandBuffer commandBuffer, VkDescriptorSet particleDescriptorSet);

    void updateColor(VkCommandBuffer commandBuffer, VkDescriptorSet particleDescriptorSet);

    void updateField(VkCommandBuffer commandBuffer, VkDescriptorSet particleDescriptorSet, int field);

    VulkanDescriptorSetLayout* particleDescriptorLayout;
    VulkanDescriptorSetLayout* neighbourListLayout;
    VulkanDescriptorSetLayout* neighbourListSizeLayout;
    VulkanDescriptorSetLayout* neighbourListOffsetLayout;
    std::array<VkDescriptorSet, 5> descriptorSets{};
    ForceDescriptor* forceDescriptor{nullptr };

    struct {
        int numParticles{0};
        float radius{1.0};
        float invMass{invMass};
        int field{DENSITY};
    } constants;
};