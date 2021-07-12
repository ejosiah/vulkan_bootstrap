
#include <PointGenerator.hpp>

PointGenerator::PointGenerator(VulkanDevice *device, VulkanDescriptorPool *pool,
                               VulkanDescriptorSetLayout *pointDescriptorSetLayout, BoundingBox domain,
                               float spacing, PointGeneratorType genType)
        : ComputePipelines{ device }
        , pool { pool }
        , pointDescriptorSetLayout{ pointDescriptorSetLayout }
        , constants{ domain.min, spacing, domain.max, genType }
{
}

void PointGenerator::init() {
    initBuffers();
    createDescriptorSetLayout();
    createDescriptorSet();
    createPipelines();
}

std::vector<PipelineMetaData> PointGenerator::pipelineMetaData() {
    return {
            {
                    shaderName(),
                    fmt::format("../../data/shaders/sph/{}.comp.spv", shaderName()),
                    {  &setLayout, pointDescriptorSetLayout },
                    { { VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(constants)}}
            }
    };
}

void PointGenerator::initBuffers() {
    atomicCounterBuffer = device->createBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, sizeof(int));
}

void PointGenerator::createDescriptorSetLayout() {
    std::vector<VkDescriptorSetLayoutBinding> bindings(1);

    bindings[0].binding = 1;
    bindings[0].descriptorCount = 1;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    setLayout = device->createDescriptorSetLayout(bindings);
}

void PointGenerator::createDescriptorSet() {
    descriptorSet = pool->allocate({ setLayout }).front();

    auto writes = initializers::writeDescriptorSets<1>();

    VkDescriptorBufferInfo atomicInfo{ atomicCounterBuffer, 0, VK_WHOLE_SIZE};
    writes[0].dstSet = descriptorSet;
    writes[0].dstBinding = 1;
    writes[0].descriptorCount = 1;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[0].pBufferInfo = &atomicInfo;

    device->updateDescriptorSets(writes);
}

void PointGenerator::execute(VkCommandBuffer commandBuffer, VkDescriptorSet pointDescriptorSet) {
    static std::array<VkDescriptorSet, 2> sets;
    sets[0] = descriptorSet;
    sets[1] = pointDescriptorSet;

    static int startIndex = 0;
    vkCmdUpdateBuffer(commandBuffer, atomicCounterBuffer, 0, sizeof(0), &startIndex);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline(shaderName()));
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, layout(shaderName()), 0, COUNT(sets), sets.data(), 0,
                            nullptr);
    vkCmdPushConstants(commandBuffer, layout(shaderName()), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(constants), &constants);
    vkCmdDispatch(commandBuffer, 16, 16, 16);
}

void PointGenerator::setPointGenerator(PointGeneratorType genType) {
    constants.genType = int(genType);
}

void PointGenerator::setSpacing(float spacing) {
    constants.spacing = spacing;
}

int PointGenerator::numParticles() {
    return atomicCounterBuffer.get<int>(0);
}
