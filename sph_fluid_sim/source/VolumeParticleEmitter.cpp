#include "VolumeParticleEmitter.hpp"


VolumeParticleEmitter::VolumeParticleEmitter(VulkanDevice* device, VulkanDescriptorPool* pool, VulkanDescriptorSetLayout *particleDescriptorSetLayout, Sdf& sdf, float spacing, PointGeneratorType genType)
: PointGenerator{ device, pool, particleDescriptorSetLayout, sdf.domain, spacing, genType }
, texture{ &sdf.texture }
{

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
    PointGenerator::createDescriptorSet();

    auto writes = initializers::writeDescriptorSets<1>();

    VkDescriptorImageInfo sdfInfo{ texture->sampler, texture->imageView, VK_IMAGE_LAYOUT_GENERAL };
    writes[0].dstSet = descriptorSet;
    writes[0].dstBinding = 0;
    writes[0].descriptorCount = 1;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[0].pImageInfo = &sdfInfo;

    device->updateDescriptorSets(writes);
}
