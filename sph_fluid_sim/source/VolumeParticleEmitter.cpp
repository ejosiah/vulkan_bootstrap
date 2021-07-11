#include "VolumeParticleEmitter.hpp"


VolumeParticleEmitter::VolumeParticleEmitter(VulkanDevice* device, VulkanDescriptorPool* pool, VulkanDescriptorSetLayout *particleDescriptorSetLayout, Sdf& sdf, float spacing, PointGeneratorType genType)
                                             : ComputePipelines{ device }
                                             , pool { pool }
                                             , particleDescriptorSetLayout{ particleDescriptorSetLayout }
                                             , texture{ &sdf.texture }
                                             , constants{ sdf.domain.min, spacing, sdf.domain.max, genType }
{
    init();
}

void VolumeParticleEmitter::init() {
    initBuffers();
    createDescriptorSetLayout();
    createDescriptorSet();
    createPipelines();
}

void VolumeParticleEmitter::initBuffers() {
    atomicCounterBuffer = device->createBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, sizeof(int));
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
    descriptorSet = pool->allocate({ setLayout }).front();

    auto writes = initializers::writeDescriptorSets<2>();

    VkDescriptorImageInfo sdfInfo{ texture->sampler, texture->imageView, VK_IMAGE_LAYOUT_GENERAL };
    writes[0].dstSet = descriptorSet;
    writes[0].dstBinding = 0;
    writes[0].descriptorCount = 1;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[0].pImageInfo = &sdfInfo;

    VkDescriptorBufferInfo atomicInfo{ atomicCounterBuffer, 0, VK_WHOLE_SIZE};
    writes[1].dstSet = descriptorSet;
    writes[1].dstBinding = 1;
    writes[1].descriptorCount = 1;
    writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[1].pBufferInfo = &atomicInfo;

    device->updateDescriptorSets(writes);
}

std::vector<PipelineMetaData> VolumeParticleEmitter::pipelineMetaData() {
    return {
        {
            "volume_particle_emitter",
            "../../data/shaders/sph/volume_emitter.comp.spv",
            {  &setLayout, particleDescriptorSetLayout },
            { { VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(constants)}}
        }
    };
}

void VolumeParticleEmitter::execute(VkCommandBuffer commandBuffer, VkDescriptorSet particleDescriptorSet) {
    static std::array<VkDescriptorSet, 2> sets;
    sets[0] = descriptorSet;
    sets[1] = particleDescriptorSet;

    static int startIndex = 0;
    vkCmdUpdateBuffer(commandBuffer, atomicCounterBuffer, 0, sizeof(0), &startIndex);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline("volume_particle_emitter"));
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, layout("volume_particle_emitter"), 0, COUNT(sets), sets.data(), 0,
                            nullptr);
    vkCmdPushConstants(commandBuffer, layout("volume_particle_emitter"), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(constants), &constants);
    vkCmdDispatch(commandBuffer, 16, 16, 16);

}

void VolumeParticleEmitter::setPointGenerator(PointGeneratorType genType) {
    constants.genType = int(genType);
}

void VolumeParticleEmitter::setSpacing(float spacing) {
    constants.spacing = spacing;
}

int VolumeParticleEmitter::numParticles() {
    return atomicCounterBuffer.get<int>(0);
}
