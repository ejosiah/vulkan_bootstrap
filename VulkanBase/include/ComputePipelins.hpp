#pragma once

#include "common.h"
#include "VulkanDevice.h"
#include "VulkanShaderModule.h"
#include "VulkanExtensions.h"

struct PipelineMetaData{
    std::string name;
    std::string shadePath;
    std::vector<VulkanDescriptorSetLayout*> layouts;
    std::vector<VkPushConstantRange> ranges;
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

    VkPipeline pipeline(const std::string& name);

    VkPipelineLayout layout(const std::string& name);

protected:
    VulkanDevice* device{};
    std::map<std::string, Pipeline> pipelines{};
};