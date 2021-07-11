#pragma once

#include "ComputePipelins.hpp"
#include "Texture.h"
#include "primitives.h"
#include "model.hpp"

enum PointGeneratorType{
 GRID_POINT_GENERATOR = 0,
 BCC_LATTICE_POINT_GENERATOR = 1,
 FCC_LATTICE_POINT_GENERATOR  = 2
};

class VolumeParticleEmitter : public ComputePipelines{
public:
    VolumeParticleEmitter() = default;

    explicit VolumeParticleEmitter(VulkanDevice* device, VulkanDescriptorPool* pool, VulkanDescriptorSetLayout* particleDescriptorSetLayout
                                   , Sdf& sdf, float spacing = 1.0, PointGeneratorType genType = BCC_LATTICE_POINT_GENERATOR);

    void init();

    std::vector<PipelineMetaData> pipelineMetaData() final;

    void initBuffers();

    void createDescriptorSetLayout();

    void createDescriptorSet();

    void execute(VkCommandBuffer commandBuffer, VkDescriptorSet particleDescriptorSet);

    void setPointGenerator(PointGeneratorType genType);

    void setSpacing(float space);

    [[nodiscard]]
    int numParticles();

private:
    Texture* texture{nullptr};

    VulkanDescriptorPool* pool{ nullptr};
    VkDescriptorSet descriptorSet{ VK_NULL_HANDLE };
    VulkanDescriptorSetLayout setLayout;
    VulkanDescriptorSetLayout* particleDescriptorSetLayout{ nullptr };
    VulkanBuffer atomicCounterBuffer;

    struct{
        glm::vec3 boundingBoxLowerCorner{0};
        float spacing{1.0f};
        glm::vec3 boundingBoxUpperCorner{1};
        int genType{ GRID_POINT_GENERATOR };
        float jitter{1.0f};
    } constants;
};