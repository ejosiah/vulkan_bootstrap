#pragma once

#include "ComputePipelins.hpp"
#include "Texture.h"
#include "primitives.h"

enum PointGeneratorType{
 GRID_POINT_GENERATOR = 0,
 BCC_LATTICE_POINT_GENERATOR = 1,
 FCC_LATTICE_POINT_GENERATOR  = 2
};

using BoundingBox = std::tuple<glm::vec3, glm::vec3>;

class VolumeParticleEmitter : public ComputePipelines{
public:
    VolumeParticleEmitter() = default;

    explicit VolumeParticleEmitter(VulkanDevice* device, VulkanDescriptorPool* pool, VulkanDescriptorSetLayout* particleDescriptorSetLayout, Texture* texture, BoundingBox bounds, float spacing, PointGeneratorType genType);

    void init();

    std::vector<PipelineMetaData> pipelineMetaData() final;

    void createDescriptorSetLayout();

    void createDescriptorSet();

    void execute(VkCommandBuffer commandBuffer, VkDescriptorSet particleDescriptorSet);

    void setPointGenerator(PointGeneratorType genType);

    void setSpacing(float space);


private:
    Texture* texture{nullptr};

    VulkanDescriptorPool* pool;
    VulkanDevice* device;
    VkDescriptorSet descriptorSet;
    VulkanDescriptorSetLayout setLayout;
    VulkanDescriptorSetLayout* particleDescriptorSetLayout;

    struct{
        glm::vec3 boundingBoxLowerCorner;
        float spacing;
        glm::vec3 boundingBoxUpperCorner;
        int genType;
    } constants;
};