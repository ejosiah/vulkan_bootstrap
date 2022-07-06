#pragma once

#include "common.h"
#include "VulkanDevice.h"
#include "VulkanShaderModule.h"
#include "VulkanExtensions.h"

struct SpecializationConstants{
    std::vector<VkSpecializationMapEntry> entries{};
    void* data = nullptr;
    size_t dataSize = 0;
};

struct PipelineMetaData{
    std::string name;
    std::string shadePath;
    std::vector<VulkanDescriptorSetLayout*> layouts;
    std::vector<VkPushConstantRange> ranges;
    SpecializationConstants specializationConstants{};
};

struct Pipeline{
    VulkanPipeline pipeline;
    VulkanPipelineLayout layout;
};

class ComputePipelines {
public:
    explicit ComputePipelines(VulkanDevice* device = nullptr);

    void createPipelines();

    virtual std::vector<PipelineMetaData> pipelineMetaData();

    VkPipeline pipeline(const std::string& name) const;

    VkPipelineLayout layout(const std::string& name) const;

protected:
    VulkanDevice* device{};
    mutable std::map<std::string, Pipeline> pipelines{};
};