#include "ExternalForces.hpp"

ExternalForces::ExternalForces(VulkanDevice* device,
                                ForceDescriptor* forceDescriptor,
                               VulkanDescriptorSetLayout *particleLayout, float drag)
                               : ComputePipelines{ device }
                               , forceDescriptor{forceDescriptor}
                               , particleLayout{ particleLayout }
{
    constants.drag = drag;
}

std::vector<PipelineMetaData> ExternalForces::pipelineMetaData() {
    return {
            {
                "external_forces",
                "../../data/shaders/sph/external_forces.comp.spv",
                { particleLayout, &forceDescriptor->layoutSet},
                { {VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(constants)} }
            }
    };
}

void ExternalForces::init() {
    createPipelines();
}


void ExternalForces::update(uint32_t numParticles, float time) {
    constants.time = time;
    setNumParticles(numParticles);
}

void ExternalForces::setNumParticles(int numParticles) {
    constants.numParticles = numParticles;
}


void ExternalForces::operator()(VkCommandBuffer commandBuffer, VkDescriptorSet particleDescriptorSet) {
    static std::array<VkDescriptorSet, 2> sets;
    sets[0] = particleDescriptorSet;
    sets[1] = forceDescriptor->set;
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline("external_forces"));
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, layout("external_forces"), 0, COUNT(sets), sets.data(), 0,
                            nullptr);
    vkCmdPushConstants(commandBuffer, layout("external_forces"), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(constants), &constants);
    vkCmdDispatch(commandBuffer, (constants.numParticles - 1)/1024 + 1, 1, 1);
}

ForceDescriptor::ForceDescriptor(VulkanDevice *device, VulkanDescriptorPool *descriptorPool)
: device{ device }
,descriptorPool{ descriptorPool }
{
    init();
}

void ForceDescriptor::init() {
    createDescriptorSetLayout();
}

void ForceDescriptor::createDescriptorSetLayout() {
    std::vector<VkDescriptorSetLayoutBinding> bindings(1);

    bindings[0].binding = 0;
    bindings[0].descriptorCount = 1;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    layoutSet = device->createDescriptorSetLayout(bindings);
}

void ForceDescriptor::setNumParticles(uint32_t numParticles) {
    if(this->numParticles == numParticles) return;
    this->numParticles = numParticles;
    forceBuffer = device->createBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, sizeof(glm::vec3) * numParticles);
    createDescriptorSet();
}

void ForceDescriptor::createDescriptorSet() {
    set = descriptorPool->allocate({ layoutSet }).front();

    auto writes = initializers::writeDescriptorSets<1>(set);
    VkDescriptorBufferInfo info = { forceBuffer, 0, VK_WHOLE_SIZE };
    writes[0].dstBinding = 0;
    writes[0].descriptorCount = 1;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[0].pBufferInfo = &info;

    device->updateDescriptorSets(writes);
}


