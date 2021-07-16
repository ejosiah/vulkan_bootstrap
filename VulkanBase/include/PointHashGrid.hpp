#pragma once

#include "VulkanDevice.h"
#include "ComputePipelins.hpp"

constexpr VkDeviceSize LIST_HEAP_SIZE = (1 << 20) * 50; // 50MB TODO calculate based on density and kernel radius
static constexpr int ITEMS_PER_WORKGROUP = 8 << 10;

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

    PointHashGrid(VulkanDevice* device, VulkanDescriptorPool* descriptorPool, VulkanDescriptorSetLayout* particleDescriptorSetLayout, glm::vec3 resolution, float gridSpacing);

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

    void buildHashGrid(VkCommandBuffer commandBuffer, VkDescriptorSet particleDescriptorSet);

    void generateHashGrid(VkCommandBuffer commandBuffer, int pass);

    void generateNeighbourList(VkCommandBuffer commandBuffer);

    void generateNeighbourList(VkCommandBuffer commandBuffer, int pass);


    std::vector<PipelineMetaData> pipelineMetaData() override;

    void updateParticleDescriptorSet(VkDescriptorSet newDescriptorSet);

    void setNumParticles(int numParticles);
public:


    static void addBufferMemoryBarriers(VkCommandBuffer commandBuffer, const std::vector<VulkanBuffer*>& buffers);

    VulkanDevice* device{};
    VulkanDescriptorPool* descriptorPool{};
    VulkanDescriptorSetLayout* particleDescriptorSetLayout;
    VulkanDescriptorSetLayout gridDescriptorSetLayout{};
    VulkanDescriptorSetLayout bucketSizeSetLayout{};
    VkDescriptorSet gridDescriptorSet{};
    VkDescriptorSet bucketSizeDescriptorSet{};
    VkDescriptorSet bucketSizeOffsetDescriptorSet{};
    VulkanBuffer bucketSizeBuffer{};
    VulkanBuffer bucketSizeOffsetBuffer{};
    VulkanBuffer bucketBuffer{};
    VulkanBuffer nextBufferIndexBuffer{};
    uint32_t bufferOffsetAlignment{};
    VkDescriptorSet particleDescriptorSet;

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