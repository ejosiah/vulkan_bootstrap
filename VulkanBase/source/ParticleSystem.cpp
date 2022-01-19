#include "ParticleSystem.hpp"

ParticleSystem::ParticleSystem(VulkanDevice *device, VulkanDescriptorPool *descriptorPool) : ComputePipelines(device) {

}

void ParticleSystem::createDescriptorSetLayouts() {
    std::vector<VkDescriptorSetLayoutBinding> bindings(1);
    bindings[0].binding = 0;
    bindings[0].descriptorCount = 1;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    descriptorSetLayout = device->createDescriptorSetLayout(bindings);
}

void ParticleSystem::createDescriptorSets() {
    descriptorPool->allocate({ descriptorSetLayout, descriptorSetLayout }, descriptorSets);

    auto writes = initializers::writeDescriptorSets<2>();

    VkDescriptorBufferInfo info0{ buffers[0], 0, VK_WHOLE_SIZE};
    writes[0].dstSet = descriptorSets[0];
    writes[0].dstBinding = 0;
    writes[0].descriptorCount = 1;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[0].pBufferInfo = &info0;

    VkDescriptorBufferInfo info1{ buffers[1], 0, VK_WHOLE_SIZE};
    writes[1].dstSet = descriptorSets[1];
    writes[1].dstBinding = 0;
    writes[1].descriptorCount = 1;
    writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[1].pBufferInfo = &info1;

    device->updateDescriptorSets(writes);
}

std::vector<PipelineMetaData> ParticleSystem::pipelineMetaData() {
    return {
            {}
    };
}

void ParticleSystem::createGraphicsPipeline() {

}
