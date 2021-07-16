#include "Collider.hpp"

Collider::Collider(VulkanDevice *device,
                   VulkanDescriptorPool *descriptorPool,
                   VulkanDescriptorSetLayout *particleDescriptorSetLayout,
                   BoxSurface domain, float radius)
                   : ComputePipelines{ device }
                   , descriptorPool{ descriptorPool }
                   , particleDescriptorSetLayout{ particleDescriptorSetLayout}
                   , domain{ domain }
{
    constants.radius = radius;
}

void Collider::init() {
    createBuffer();
    createDescriptorSetLayout();
    createDescriptorSet();
    createPipelines();
}

void Collider::createBuffer() {
    buffer = device->createDeviceLocalBuffer(&domain, sizeof(domain), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
}

std::vector<PipelineMetaData> Collider::pipelineMetaData() {
    return {
            {
                "collider",
                "../../data/shaders/sph/collision_resolver.comp.spv",
                    { particleDescriptorSetLayout, &surfaceSetLayout},
                    {{VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(constants)}}
            }
    };
}

void Collider::createDescriptorSetLayout() {
    std::vector<VkDescriptorSetLayoutBinding> bindings(1);
    bindings[0].binding = 0;
    bindings[0].descriptorCount = 1;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    surfaceSetLayout = device->createDescriptorSetLayout(bindings);
}

void Collider::createDescriptorSet() {
   surfaceDescriptorSet = descriptorPool->allocate({ surfaceSetLayout}).front();
   
    auto writes = initializers::writeDescriptorSets<1>(surfaceDescriptorSet);
    VkDescriptorBufferInfo info{ buffer, 0, VK_WHOLE_SIZE };
    writes[0].dstBinding = 0;
    writes[0].descriptorCount = 1;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[0].pBufferInfo = &info;
   
   device->updateDescriptorSets(writes);
}

void Collider::operator()(VkCommandBuffer commandBuffer, VkDescriptorSet particleDescriptorSet) {
    static std::array<VkDescriptorSet, 2> sets;
    sets[0] = particleDescriptorSet;
    sets[1] = surfaceDescriptorSet;

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline("collider"));
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, layout("collider"), 0, COUNT(sets), sets.data(), 0,
                            nullptr);
    vkCmdPushConstants(commandBuffer, layout("collider"), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(constants), &constants);
    vkCmdDispatch(commandBuffer, workGroupSize(constants.numParticles, PARTICLES_PER_INVOCATION), 1, 1);

}

void Collider::setNumParticles(uint32_t numParticles) {
    constants.numParticles = numParticles;
}

