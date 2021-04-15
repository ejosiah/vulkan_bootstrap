#pragma once

#include "common.h"

struct VulkanDescriptorPool{

    DISABLE_COPY(VulkanDescriptorPool)

    VulkanDescriptorPool() = default;

    template<typename PoolSizes>
    VulkanDescriptorPool(VkDevice device, uint32_t maxSet, const PoolSizes& poolSizes, VkDescriptorPoolCreateFlags flags = 0)
            :device(device)
    {
        VkDescriptorPoolCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        createInfo.flags = 0;
        createInfo.maxSets = maxSet;
        createInfo.poolSizeCount = COUNT(poolSizes);
        createInfo.pPoolSizes = poolSizes.data();

        ASSERT(vkCreateDescriptorPool(device, &createInfo, nullptr, &pool));
    }

    VulkanDescriptorPool(VulkanDescriptorPool&& source) noexcept {
        operator=(static_cast<VulkanDescriptorPool&&>(source));
    }

    ~VulkanDescriptorPool(){
        if(pool){
            vkDestroyDescriptorPool(device, pool, VK_NULL_HANDLE);
        }
    }

    VulkanDescriptorPool& operator=(VulkanDescriptorPool&& source) noexcept {
        if(this == &source) return *this;

        this->device = source.device;
        this->pool = source.pool;

        source.pool = VK_NULL_HANDLE;

        return *this;
    }

    [[nodiscard]]
    std::vector<VkDescriptorSet> allocate(const std::vector<VkDescriptorSetLayout>& layouts) const {
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = pool;
        allocInfo.descriptorSetCount = COUNT(layouts);
        allocInfo.pSetLayouts = layouts.data();

        std::vector<VkDescriptorSet> sets(layouts.size());
        vkAllocateDescriptorSets(device, &allocInfo, sets.data());

        return sets;
    }

    void free(VkDescriptorSet set){
       vkFreeDescriptorSets(device, pool, 1, &set);
    }

    void free(const std::vector<VkDescriptorSet>& sets) const {
        vkFreeDescriptorSets(device, pool ,COUNT(sets), sets.data());
    }

    operator VkDescriptorPool() const {
        return pool;
    }

    VkDevice device = VK_NULL_HANDLE;
    VkDescriptorPool pool = VK_NULL_HANDLE;
};