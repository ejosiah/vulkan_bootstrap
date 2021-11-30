#include "ComputePressure.hpp"

ComputePressure::ComputePressure(VulkanDevice *device,
                                 VulkanDescriptorSetLayout *particleLayout,
                                 float eosExponent, float targetDensity,
                                 float speedOfSound, float negativePressureScale)
                                 : ComputePipelines{ device }
                                 , particleLayout{ particleLayout }
{
    constants.eosExponent = eosExponent;
    constants.speedOfSound = speedOfSound;
    constants.targetDensity = targetDensity;
    constants.negativePressureScale = negativePressureScale;
}

void ComputePressure::init() {
    createPipelines();
}

std::vector<PipelineMetaData> ComputePressure::pipelineMetaData() {
    return {
            {
                    "compute_pressure",
                    "../../data/shaders/sph/compute_pressure.comp.spv",
                    { particleLayout },
                    { {VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(constants)} }
            }
    };
}

void ComputePressure::setNumParticles(int numParticles) {
    constants.numParticles = numParticles;
}


void ComputePressure::operator()(VkCommandBuffer commandBuffer, VkDescriptorSet particleDescriptorSet) {
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline("compute_pressure"));
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, layout("compute_pressure"), 0, 1, &particleDescriptorSet, 0,
                            nullptr);
    vkCmdPushConstants(commandBuffer, layout("compute_pressure"), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(constants), &constants);
    vkCmdDispatch(commandBuffer, (constants.numParticles - 1)/1024 + 1, 1, 1);
}
