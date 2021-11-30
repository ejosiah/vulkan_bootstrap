#pragma once

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