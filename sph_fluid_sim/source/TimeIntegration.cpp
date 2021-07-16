
#include <vector>
#include <ComputePipelins.hpp>
#include <TimeIntegration.hpp>


TimeIntegration::TimeIntegration(VulkanDevice* device, VulkanDescriptorSetLayout *particleLayout, VulkanDescriptorSetLayout *forceLayout)
: ComputePipelines{ device }
, particleLayout{ particleLayout}
, forceLayout{ forceLayout }
{

}

void TimeIntegration::init() {
    createPipelines();
}


std::vector <PipelineMetaData> TimeIntegration::pipelineMetaData() {
    return {
        {
            "time_integration",
            "../../data/shaders/sph/integrate.comp.spv",
                { particleLayout, particleLayout, forceLayout},
                { {VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(constants)}}
        }
    };
}

void TimeIntegration::operator()(VkCommandBuffer commandBuffer, std::array<VkDescriptorSet, 2>& particleDescriptorSets, VkDescriptorSet forceSet) {
    static std::array<VkDescriptorSet, 3> sets;
    sets[0] = particleDescriptorSets[0];
    sets[1] = particleDescriptorSets[1];
    sets[2] = forceSet;

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline("time_integration"));
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, layout("time_integration"), 0, COUNT(sets), sets.data(), 0, nullptr);
    vkCmdPushConstants(commandBuffer, layout("time_integration"), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(constants), &constants);
    vkCmdDispatch(commandBuffer, (constants.numParticles - 1)/1024 + 1, 1, 1);
}

void TimeIntegration::update(uint32_t numParticles, float time) {
    constants.numParticles = numParticles;
    constants.time = time;
}


