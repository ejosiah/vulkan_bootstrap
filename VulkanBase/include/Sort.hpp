#pragma once

#include "ComputePipelins.hpp"

class Sort : public ComputePipelines{
public:
    explicit Sort(VulkanDevice* device = nullptr) : ComputePipelines(device){};

    virtual void init() {};

    virtual void operator()(VkCommandBuffer commandBuffer, VulkanBuffer& buffer) = 0;
};

class StableSort : public Sort{
public:
    explicit StableSort(VulkanDevice* device = nullptr) : Sort(device){}
};

class RadixSort : public StableSort{
    static constexpr uint WORLD_SIZE = 32;
    static constexpr uint BLOCK_SIZE = 8;
    static constexpr uint RADIX = 256;
    static constexpr uint PASSES = WORLD_SIZE/BLOCK_SIZE;
    static constexpr uint DATA_IN = 0;
    static constexpr uint DATA_OUT = 1;
    static constexpr uint DATA = 0;
    static constexpr uint ADD_IN = 0;
    static constexpr uint ADD_OUT = 1;
    static constexpr uint VALUE = 0;
    static constexpr uint COUNTS = 0;
    static constexpr uint SUMS = 1;
    static constexpr uint NUM_DATA_ELEMENTS = 1;
    static constexpr uint ELEMENTS_PER_WG = 1 << 14;
    static constexpr uint MAX_WORKGROUPS = 64;
    static constexpr uint NUM_THREADS_PER_BLOCK = 1024;
    static constexpr uint NUM_GROUPS_PER_WORKGROUP = NUM_THREADS_PER_BLOCK / WORLD_SIZE;
public:
    explicit RadixSort(VulkanDevice* device = nullptr);

    void init() override;

    void createDescriptorPool();

    void createDescriptorSetLayouts();

    void createDescriptorSets();

    std::vector<PipelineMetaData> pipelineMetaData() override;

    void operator()(VkCommandBuffer commandBuffer, VulkanBuffer &buffer) override;

    void updateConstants(VulkanBuffer& buffer);

    static uint numWorkGroups(VulkanBuffer& buffer);

    void count(VkCommandBuffer commandBuffer, VkDescriptorSet dataDescriptorSet);

    void prefixSum(VkCommandBuffer commandBuffer);

    void reorder(VkCommandBuffer commandBuffer, std::array<VkDescriptorSet, 2>& dataDescriptorSets);

    void updateDataDescriptorSets(VulkanBuffer& inBuffer);

public:
    VulkanDescriptorPool descriptorPool;
    VulkanDescriptorSetLayout dataSetLayout;
    VulkanDescriptorSetLayout countsSetLayout;
    std::array<VkDescriptorSet, 2> dataDescriptorSets;
    VkDescriptorSet countsDescriptorSet;
    VulkanBuffer countsBuffer;
    VulkanBuffer sumBuffer;
    std::array<VulkanBuffer*, 2> dataBuffers;
    VulkanBuffer dataScratchBuffer;
    uint workGroupCount = 0;

    struct {
        uint block;
        uint R = WORLD_SIZE;
        uint Radix = RADIX;
        uint Num_Groups_per_WorkGroup;
        uint Num_Elements_per_WorkGroup;
        uint Num_Elements_Per_Group;
        uint Num_Elements;
        uint Num_Radices_Per_WorkGroup;
        uint Num_Groups;
    } constants;
};