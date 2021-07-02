#pragma once

#include "VulkanDevice.h"
#include "ComputePipelins.hpp"
#include "particle_model.hpp"

constexpr VkDeviceSize LIST_HEAP_SIZE = (1 << 20) * 50; // 50MB

struct PrefixScan{
    VkDescriptorSet descriptorSet{};
    VkDescriptorSet sumScanDescriptorSet{};
    VulkanDescriptorSetLayout setLayout{};
    VulkanBuffer sumsBuffer{};
    struct {
        int itemsPerWorkGroup = 8 << 10;
        int N = 0;
    } constants{};

    void scan(VkCommandBuffer commandBuffer, VulkanBuffer& buffer, VkPipeline pipeline, VkPipelineLayout layout);
};

class PointHashGrid : public ComputePipelines{
public:
    PointHashGrid() = default;

    PointHashGrid(VulkanDevice* device, VulkanDescriptorPool* descriptorPool, VulkanBuffer* particleBuffer, glm::vec3 resolution, float gridSpacing);

    void init();

    void initNeighbourList();

    void initNeighbourListBuffers();

    void createDescriptorSetLayouts();

    void createNeighbourListSetLayout();

    void createPrefixScanDescriptorSetLayouts();

    void createDescriptorSets();

    void createNeighbourListDescriptorSets();

    void updateDescriptorSet();

    void updateBucketDescriptor();

    void updateGridBuffer();

    void updateScanBuffer();

    void updateScanDescriptorSet();

    void updateNeighbourListScanDescriptorSet();

    void scan(VkCommandBuffer commandBuffer);

    void scanNeighbourList(VkCommandBuffer commandBuffer);

    void buildHashGrid(VkCommandBuffer commandBuffer);

    void generateHashGrid(VkCommandBuffer commandBuffer, int pass);

    void generateNeighbourList(VkCommandBuffer commandBuffer);

    void generateNeighbourList(VkCommandBuffer commandBuffer, int pass);

    std::vector<PipelineMetaData> pipelineMetaData() override;

public:


    static void addBufferMemoryBarriers(VkCommandBuffer commandBuffer, const std::vector<VulkanBuffer*>& buffers);

    VulkanDevice* device{};
    VulkanDescriptorPool* descriptorPool{};

    VulkanDescriptorSetLayout gridDescriptorSetLayout{};
    VulkanDescriptorSetLayout bucketSizeSetLayout{};
    VulkanDescriptorSetLayout particleDescriptorSetLayout{};
    VulkanDescriptorSetLayout unitTestDescriptorSetLayout{};
    VkDescriptorSet gridDescriptorSet{};
    VkDescriptorSet bucketSizeDescriptorSet{};
    VkDescriptorSet bucketSizeOffsetDescriptorSet{};
    VkDescriptorSet particleDescriptorSet{};
    VkDescriptorSet unitTestDescriptorSet{};
    VulkanBuffer bucketSizeBuffer{};
    VulkanBuffer bucketSizeOffsetBuffer{};
    VulkanBuffer bucketBuffer{};
    VulkanBuffer* particleBuffer{};
    VulkanBuffer nextBufferIndexBuffer{};
    VulkanBuffer nearByKeys{};
    uint32_t bufferOffsetAlignment{};

    PrefixScan prefixScan;

    struct {
        VulkanDescriptorSetLayout setLayout;
        VkDescriptorSet descriptorSet;
    } bucket;

    struct {
        glm::vec3 resolution{1};
        float gridSpacing{1};
        uint32_t pass{0};
        uint32_t numParticles{0};
    } constants{};

    struct {
        VkDescriptorSet neighbourSizeDescriptorSet{};
        VkDescriptorSet neighbourSizeOffsetDescriptorSet{};
        VkDescriptorSet descriptorSet;
        VulkanDescriptorSetLayout neighbourSizeSetLayout{};
        VulkanDescriptorSetLayout setLayout{};
        VulkanBuffer neighbourListBuffer;
        VulkanBuffer neighbourSizeBuffer;
        VulkanBuffer neighbourOffsetsBuffer;
        PrefixScan prefixScan;
    } neighbourList;
};