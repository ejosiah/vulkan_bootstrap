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

struct PointGenerator : public ComputePipelines{
    PointGenerator() = default;

    explicit PointGenerator(VulkanDevice* device, VulkanDescriptorPool* pool, VulkanDescriptorSetLayout* pointDescriptorSetLayout,
                            BoundingBox domain, float spacing = 1.0, PointGeneratorType genType = BCC_LATTICE_POINT_GENERATOR);

     void init();

    [[nodiscard]]
    virtual std::string shaderName() const{
        return "point_generator";
    }


    std::vector<PipelineMetaData> pipelineMetaData() final;

    void initBuffers();

    virtual void createDescriptorSetLayout();

    virtual void createDescriptorSet();

    void execute(VkCommandBuffer commandBuffer, VkDescriptorSet particleDescriptorSet);

    void setPointGenerator(PointGeneratorType genType);

    void setSpacing(float spacing);

    [[nodiscard]]
    int numParticles();

    VulkanDescriptorPool* pool{ nullptr};
    VkDescriptorSet descriptorSet{ VK_NULL_HANDLE };
    VulkanDescriptorSetLayout setLayout;
    VulkanDescriptorSetLayout* pointDescriptorSetLayout{ nullptr };
    VulkanBuffer atomicCounterBuffer;

    struct{
        glm::vec3 boundingBoxLowerCorner{0};
        float spacing{1.0f};
        glm::vec3 boundingBoxUpperCorner{1};
        int genType{ GRID_POINT_GENERATOR };
        float jitter{0.0f};
    } constants;

};