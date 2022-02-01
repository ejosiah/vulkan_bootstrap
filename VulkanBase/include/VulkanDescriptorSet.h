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
        createInfo.flags = flags;
        createInfo.maxSets = maxSet;
        createInfo.poolSizeCount = COUNT(poolSizes);
        createInfo.pPoolSizes = poolSizes.data();

        ERR_GUARD_VULKAN(vkCreateDescriptorPool(device, &createInfo, nullptr, &pool));
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
    inline std::vector<VkDescriptorSet> allocate(const std::vector<VkDescriptorSetLayout>& layouts) const {
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = pool;
        allocInfo.descriptorSetCount = COUNT(layouts);
        allocInfo.pSetLayouts = layouts.data();

        std::vector<VkDescriptorSet> sets(layouts.size());
        vkAllocateDescriptorSets(device, &allocInfo, sets.data());

        return sets;
    }

    template<typename DescriptorSets>
    inline void allocate(const std::vector<VkDescriptorSetLayout>& layouts, DescriptorSets& descriptorSets){
        assert(descriptorSets.size() >= layouts.size());

        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = pool;
        allocInfo.descriptorSetCount = COUNT(layouts);
        allocInfo.pSetLayouts = layouts.data();

        vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data());
    }

    inline void free(VkDescriptorSet set) const {
       vkFreeDescriptorSets(device, pool, 1, &set);
    }

    inline void free(const std::vector<VkDescriptorSet>& sets) const {
        vkFreeDescriptorSets(device, pool ,COUNT(sets), sets.data());
    }


    operator VkDescriptorPool() const {
        return pool;
    }

    VkDevice device = VK_NULL_HANDLE;
    VkDescriptorPool pool = VK_NULL_HANDLE;
};