#include "Interpolator.hpp"

Interpolator::Interpolator(VulkanDevice *device,
                           VulkanDescriptorSetLayout *particleDescriptorLayout,
                           ForceDescriptor* forceDescriptor,
                           VulkanDescriptorSetLayout *neighbourListLayout,
                           VulkanDescriptorSetLayout *neighbourListSizeLayout,
                           VulkanDescriptorSetLayout *neighbourListOffsetLayout,
                           float radius, float invMass)
                           : ComputePipelines{ device }
                           , particleDescriptorLayout{ particleDescriptorLayout }
                           , forceDescriptor{ forceDescriptor }
                           , neighbourListLayout{ neighbourListLayout }
                           , neighbourListSizeLayout{ neighbourListSizeLayout }
                           , neighbourListOffsetLayout{ neighbourListOffsetLayout }
{
    constants.radius = radius;
    constants.invMass = invMass;
}

void Interpolator::init() {
    createPipelines();
}

std::vector<PipelineMetaData> Interpolator::pipelineMetaData() {
    return {
            {
                "interpolator",
                "../../data/shaders/sph/interpolate_values.comp.spv",
                    { particleDescriptorLayout, &forceDescriptor->layoutSet, neighbourListLayout,
                      neighbourListSizeLayout, neighbourListOffsetLayout},
                    { { VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(constants) }}
            }
    };
}

void Interpolator::setNumParticles(int numParticles) {
    constants.numParticles = numParticles;
}


void Interpolator::updateDensity(VkCommandBuffer commandBuffer, VkDescriptorSet particleDescriptorSet) {
    updateField(commandBuffer, particleDescriptorSet, DENSITY);
}

void Interpolator::updatePressure(VkCommandBuffer commandBuffer, VkDescriptorSet particleDescriptorSet) {
    updateField(commandBuffer, particleDescriptorSet, PRESSURE);
}

void Interpolator::updateColor(VkCommandBuffer commandBuffer, VkDescriptorSet particleDescriptorSet) {
    updateField(commandBuffer, particleDescriptorSet, COLOR);
}



void Interpolator::setNeighbourDescriptors(VkDescriptorSet* sets) {
    assert(sets);
    auto dest = begin(descriptorSets);
    std::advance(dest, 2);
    std::copy(sets, sets + 3, dest);
}

void Interpolator::updateField(VkCommandBuffer commandBuffer, VkDescriptorSet particleDescriptorSet, int field) {
    assert(constants.numParticles > 0);
    constants.field = field;
    descriptorSets[0] = particleDescriptorSet;
    descriptorSets[1] = forceDescriptor->set;
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline("interpolator"));
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, layout("interpolator")
                            , 0, COUNT(descriptorSets), descriptorSets.data(), 0, nullptr);
    vkCmdPushConstants(commandBuffer, layout("interpolator"), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(constants), &constants);
    vkCmdDispatch(commandBuffer, (constants.numParticles - 1)/1024 + 1, 1, 1);
}




