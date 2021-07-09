#include "VolumeParticleEmitter.hpp"


VolumeParticleEmitter::VolumeParticleEmitter(VulkanDevice* device, VulkanDescriptorPool* pool, VulkanDescriptorSetLayout *particleDescriptorSetLayout, Texture *texture,
                                             BoundingBox bounds, float spacing, PointGeneratorType genType)
                                             : device{ device }
                                             , pool { pool }
                                             , particleDescriptorSetLayout{ particleDescriptorSetLayout }
                                             , texture{ texture }
                                             , constants{ get<0>(bounds), spacing, get<1>(bounds), genType }
{
    init();
}

void VolumeParticleEmitter::init() {
    createDescriptorSetLayout();
    createDescriptorSet();
    createPipelines();
}


std::vector<PipelineMetaData> VolumeParticleEmitter::pipelineMetaData() {
    return {
        {
            "volume_particle_emitter",
            "../../data/sph/volume_emitter.comp.spv",
            {  &setLayout, particleDescriptorSetLayout },
            { { VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(constants)}}
        }
    };
}

void VolumeParticleEmitter::execute(VkCommandBuffer commandBuffer, VkDescriptorSet particleDescriptorSet) {

}

void VolumeParticleEmitter::setPointGenerator(PointGeneratorType genType) {
    constants.genType = int(genType);
}

void VolumeParticleEmitter::setSpacing(float spacing) {
    constants.spacing = spacing;
}

void VolumeParticleEmitter::createDescriptorSetLayout() {
    std::vector<VkDescriptorSetLayoutBinding> bindings(2);
    bindings[0].binding = 0;
    bindings[0].descriptorCount = 1;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    bindings[1].binding = 1;
    bindings[1].descriptorCount = 1;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    setLayout = device->createDescriptorSetLayout(bindings);

}

void VolumeParticleEmitter::createDescriptorSet() {

}
