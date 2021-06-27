#include "ComputePipelins.hpp"

ComputePipelines::ComputePipelines(VulkanDevice *device): device(device) {

}

void ComputePipelines::createPipelines() {
    for(auto& metaData : pipelineMetaData()){
        auto shaderModule = VulkanShaderModule{ metaData.shadePath, *device};
        auto stage = initializers::shaderStage({ shaderModule, VK_SHADER_STAGE_COMPUTE_BIT});
        Pipeline pipeline;
        std::vector<VkDescriptorSetLayout> setLayouts;
        for(auto& layout : metaData.layouts){
            setLayouts.push_back(layout->handle);
        }
        pipeline.layout = device->createPipelineLayout(setLayouts, metaData.ranges);

        auto createInfo = initializers::computePipelineCreateInfo();
        createInfo.stage = stage;
        createInfo.layout = pipeline.layout;

        pipeline.pipeline = device->createComputePipeline(createInfo);
        pipelines.insert(std::make_pair(metaData.name, std::move(pipeline)));
    }
}

std::vector<PipelineMetaData> ComputePipelines::pipelineMetaData() {
    return {};
}


VkPipeline ComputePipelines::pipeline(const std::string& name){
    assert(pipelines.find(name) != end(pipelines));
    return pipelines[name].pipeline;
}

VkPipelineLayout ComputePipelines::layout(const std::string& name){
    assert(pipelines.find(name) != end(pipelines));
    return pipelines[name].layout;
}