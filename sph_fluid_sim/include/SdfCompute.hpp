#pragma once

#include "model.hpp"
#include "ComputePipelins.hpp"

class SdfCompute : public ComputePipelines{
public:
    SdfCompute() = default;

    SdfCompute(VulkanDevice* device, VulkanDescriptorPool* pool, BoundingBox domain, std::string sdfPath, glm::uvec3 resolution);

    void init();

    std::vector<PipelineMetaData> pipelineMetaData() final;

    void createDescriptorSetLayout();

    void createDescriptorSet();

    void createImage();

    void createBuffers();

    void execute(VkCommandBuffer commandBuffer);

    Sdf sdf;

private:
    VulkanDescriptorPool* pool{ nullptr };
    VulkanDescriptorSetLayout setLayout;
    VulkanBuffer buffer;
    VkDescriptorSet descriptorSet{ VK_NULL_HANDLE };
    std::string sdfPath;
    BoundingBox domain;
    glm::uvec3 resolution{1};
};