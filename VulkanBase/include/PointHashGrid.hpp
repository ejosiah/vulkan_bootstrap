#pragma once

#include "common.h"
#include "VulkanDevice.h"
#include "ComputePipelins.hpp"

constexpr int ITEMS_PER_WORKGROUP = 8 << 10;

struct Particle{
    glm::vec4 position{0};
    glm::vec4 color{0};
    glm::vec3 velocity{0};
    float invMass{0};
};

class PointHashGrid : public ComputePipelines{
public:
    PointHashGrid() = default;

    PointHashGrid(VulkanDevice* device, VulkanDescriptorPool* descriptorPool, VulkanBuffer* particleBuffer, glm::vec3 resolution, float gridSpacing);

    void init();

    void createDescriptorSetLayouts();

    void createPrefixScanDescriptorSetLayouts();

    void createDescriptorSets();

    void updateDescriptorSet();

    void updateBucketDescriptor();

    void updateGridBuffer();

    void updateScanBuffer();

    void updateScanDescriptorSet();

    void scan(VkCommandBuffer commandBuffer);

    void buildHashGrid(VkCommandBuffer commandBuffer);

    void generateHashGrid(VkCommandBuffer commandBuffer, int pass);

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

    struct {
        glm::vec3 resolution{1};
        float gridSpacing{1};
        uint32_t pass{0};
        uint32_t numParticles{0};
    } constants{};

    struct {
        VkDescriptorSet descriptorSet{};
        VkDescriptorSet sumScanDescriptorSet{};
        VulkanDescriptorSetLayout setLayout{};
        VulkanBuffer sumsBuffer{};
        struct {
            int itemsPerWorkGroup = 8 << 10;
            int N = 0;
        } constants{};
    } prefixScan{};
};