#include "PointHashGrid.hpp"

PointHashGrid::PointHashGrid(VulkanDevice* device, VulkanDescriptorPool* descriptorPool, VulkanBuffer* particleBuffer, glm::vec3 resolution, float gridSpacing)
:
  ComputePipelines(device),  
  device(device),
  particleBuffer(particleBuffer),
  descriptorPool(descriptorPool)
{
    constants.resolution = resolution;
    constants.gridSpacing = gridSpacing;
    constants.numParticles = particleBuffer->size/sizeof(Particle);
    bufferOffsetAlignment = device->getLimits().minStorageBufferOffsetAlignment;
    init();
}

void PointHashGrid::init() {
    updateGridBuffer();
    createDescriptorSetLayouts();
    createPrefixScanDescriptorSetLayouts();
    createDescriptorSets();
    updateDescriptorSet();
    updateScanDescriptorSet();
    createPipelines();
}

void PointHashGrid::createDescriptorSetLayouts() {
    std::vector<VkDescriptorSetLayoutBinding> bindings(1);
    bindings[0].binding = 0;
    bindings[0].descriptorCount = 1;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    particleDescriptorSetLayout = device->createDescriptorSetLayout(bindings);

    bindings.resize(2);
    bindings[0].binding = 0;
    bindings[0].descriptorCount = 1;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    bindings[1].binding = 1;
    bindings[1].descriptorCount = 1;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    gridDescriptorSetLayout = device->createDescriptorSetLayout(bindings);

    bindings.resize(1);
    bindings[0].binding = 0;
    bindings[0].descriptorCount = 1;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    bucketSizeSetLayout = device->createDescriptorSetLayout(bindings);

    // unit test layout
    bindings.resize(2);
    bindings[0].binding = 0;
    bindings[0].descriptorCount = 1;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    bindings[1].binding = 1;
    bindings[1].descriptorCount = 1;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    unitTestDescriptorSetLayout = device->createDescriptorSetLayout(bindings);
}

void PointHashGrid::createPrefixScanDescriptorSetLayouts() {
    std::array<VkDescriptorSetLayoutBinding, 2> bindings{};

    bindings[0].binding = 0;
    bindings[0].descriptorCount = 1;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    bindings[1].binding = 1;
    bindings[1].descriptorCount = 1;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    prefixScan.setLayout = device->createDescriptorSetLayout(bindings);
}

void PointHashGrid::createDescriptorSets() {
    auto sets = descriptorPool->allocate({particleDescriptorSetLayout,
                                         gridDescriptorSetLayout,
                                         prefixScan.setLayout, prefixScan.setLayout, bucketSizeSetLayout, bucketSizeSetLayout, unitTestDescriptorSetLayout } );
    particleDescriptorSet = sets[0];
    gridDescriptorSet = sets[1];
    prefixScan.descriptorSet = sets[2];
    prefixScan.sumScanDescriptorSet = sets[3];
    bucketSizeDescriptorSet = sets[4];
    bucketSizeOffsetDescriptorSet = sets[5];
    unitTestDescriptorSet = sets[6];
}

void PointHashGrid::updateDescriptorSet() {
    auto writes = initializers::writeDescriptorSets<6>();

    // particle descriptor set
    VkDescriptorBufferInfo info0{*particleBuffer, 0, VK_WHOLE_SIZE};
    writes[0].dstSet = particleDescriptorSet;
    writes[0].dstBinding = 0;
    writes[0].descriptorCount = 1;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[0].pBufferInfo = &info0;

    // grid descriptor set
    VkDescriptorBufferInfo  nextBucketIndexInfo{nextBufferIndexBuffer, 0, VK_WHOLE_SIZE };
    writes[1].dstSet = gridDescriptorSet;
    writes[1].dstBinding = 1;
    writes[1].descriptorCount = 1;
    writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[1].pBufferInfo = &nextBucketIndexInfo;

    // bucket size / offset
    VkDescriptorBufferInfo  bucketSizeInfo{bucketSizeBuffer, 0, VK_WHOLE_SIZE };
    writes[2].dstSet = bucketSizeDescriptorSet;
    writes[2].dstBinding = 0;
    writes[2].descriptorCount = 1;
    writes[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[2].pBufferInfo = &bucketSizeInfo;

    VkDescriptorBufferInfo  bucketOffsetInfo{bucketSizeOffsetBuffer, 0, VK_WHOLE_SIZE };
    writes[3].dstSet = bucketSizeOffsetDescriptorSet;
    writes[3].dstBinding = 0;
    writes[3].descriptorCount = 1;
    writes[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[3].pBufferInfo = &bucketOffsetInfo;

    // unit test writes
    writes[4].dstSet = unitTestDescriptorSet;
    writes[4].dstBinding = 0;
    writes[4].descriptorCount = 1;
    writes[4].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[4].pBufferInfo = &info0;

    VkDescriptorBufferInfo  nearByKeysInfo{nearByKeys, 0, VK_WHOLE_SIZE };
    writes[5].dstSet = unitTestDescriptorSet;
    writes[5].dstBinding = 1;
    writes[5].descriptorCount = 1;
    writes[5].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[5].pBufferInfo = &nearByKeysInfo;

    device->updateDescriptorSets(writes);
    updateBucketDescriptor();
}

void PointHashGrid::updateBucketDescriptor() {
    auto writes = initializers::writeDescriptorSets<1>();
    VkDescriptorBufferInfo bucketInfo{bucketBuffer, 0, VK_WHOLE_SIZE };
    writes[0].dstSet = gridDescriptorSet;
    writes[0].dstBinding = 0;
    writes[0].descriptorCount = 1;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[0].pBufferInfo = &bucketInfo;

    device->updateDescriptorSets(writes);
}

void PointHashGrid::updateGridBuffer() {
    auto res = constants.resolution;
    VkDeviceSize gridSize = res.x * res.y * res.z * sizeof(uint32_t);

    nextBufferIndexBuffer = device->createBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, gridSize);
    bucketSizeBuffer = device->createBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, gridSize);
    bucketSizeOffsetBuffer = device->createBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, gridSize);
    bucketBuffer = device->createBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, gridSize);
    nearByKeys = device->createBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, sizeof(int) * 8);
    updateScanBuffer();
}

void PointHashGrid::updateScanBuffer() {
    VkDeviceSize sumsSize = (std::abs(int(constants.numParticles - 1))/ITEMS_PER_WORKGROUP + 1) * sizeof(int);
    sumsSize = alignedSize(sumsSize, bufferOffsetAlignment) + sizeof(int);
    prefixScan.sumsBuffer = device->createBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, sumsSize);
    prefixScan.constants.N = constants.numParticles/sizeof(int);
}

void PointHashGrid::updateScanDescriptorSet() {
    VkDescriptorBufferInfo info{bucketSizeOffsetBuffer, 0, VK_WHOLE_SIZE};
    auto writes = initializers::writeDescriptorSets<4>(prefixScan.descriptorSet);
    writes[0].dstBinding = 0;
    writes[0].descriptorCount = 1;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[0].pBufferInfo = &info;

    VkDescriptorBufferInfo sumsInfo{ prefixScan.sumsBuffer, 0, prefixScan.sumsBuffer.size - sizeof(int)};
    writes[1].dstBinding = 1;
    writes[1].descriptorCount = 1;
    writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[1].pBufferInfo = &sumsInfo;

    // for sum scan
    writes[2].dstSet = prefixScan.sumScanDescriptorSet;
    writes[2].dstBinding = 0;
    writes[2].descriptorCount = 1;
    writes[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[2].pBufferInfo = &sumsInfo;

    VkDescriptorBufferInfo sumsSumInfo{ prefixScan.sumsBuffer, prefixScan.sumsBuffer.size - sizeof(int), VK_WHOLE_SIZE}; // TODO FIXME
    writes[3].dstSet = prefixScan.sumScanDescriptorSet;
    writes[3].dstBinding = 1;
    writes[3].descriptorCount = 1;
    writes[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[3].pBufferInfo = &sumsSumInfo;

    device->updateDescriptorSets(writes);
}

void PointHashGrid::scan(VkCommandBuffer commandBuffer) {
    int size = constants.numParticles;
    int numWorkGroups = std::abs(size - 1)/ITEMS_PER_WORKGROUP + 1;

    VkBufferCopy region{0, 0, bucketSizeBuffer.size};
    vkCmdCopyBuffer(commandBuffer, bucketSizeBuffer, bucketSizeOffsetBuffer, 1, &region);

    addBufferMemoryBarriers(commandBuffer, {&bucketSizeBuffer});  // make sure grid build for pass 0 finished
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline("prefix_scan"));
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, layout("prefix_scan"), 0, 1, &prefixScan.descriptorSet, 0, nullptr);
    vkCmdDispatch(commandBuffer, numWorkGroups, 1, 1);

    if(numWorkGroups > 1){
        addBufferMemoryBarriers(commandBuffer, {&bucketSizeOffsetBuffer, &prefixScan.sumsBuffer});
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, layout("prefix_scan"), 0, 1, &prefixScan.sumScanDescriptorSet, 0, nullptr);
        vkCmdDispatch(commandBuffer, 1, 1, 1);

        addBufferMemoryBarriers(commandBuffer, {&bucketSizeOffsetBuffer, &prefixScan.sumsBuffer });
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline("add"));
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, layout("add"), 0, 1, &prefixScan.descriptorSet, 0, nullptr);
        vkCmdPushConstants(commandBuffer, layout("add"), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(prefixScan.constants), &prefixScan.constants);
        vkCmdDispatch(commandBuffer, numWorkGroups, 1, 1);
    }
    // make sure bucketSize before write finished before grid build pass 1
    addBufferMemoryBarriers(commandBuffer, {&bucketSizeOffsetBuffer});
}

void PointHashGrid::buildHashGrid(VkCommandBuffer commandBuffer) {
    generateHashGrid(commandBuffer, 0);
    scan(commandBuffer);
    generateHashGrid(commandBuffer, 1);
}

void PointHashGrid::generateHashGrid(VkCommandBuffer commandBuffer, int pass) {
    // TODO ADD buffer barriers
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline("point_hash_grid_builder"));

    constants.pass = pass;
    vkCmdPushConstants(commandBuffer, layout("point_hash_grid_builder"), VK_SHADER_STAGE_COMPUTE_BIT, 0,
                       sizeof(constants), &constants);

    static std::array<VkDescriptorSet, 3> sets;
    sets[0] = particleDescriptorSet;
    sets[1] = gridDescriptorSet;
    sets[2] = (pass%2) ? bucketSizeOffsetDescriptorSet : bucketSizeDescriptorSet;

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, layout("point_hash_grid_builder"), 0, COUNT(sets), sets.data(), 0, nullptr);
    vkCmdDispatch(commandBuffer, (constants.numParticles - 1)/1024 + 1, 1, 1);
}

void PointHashGrid::addBufferMemoryBarriers(VkCommandBuffer commandBuffer, const std::vector<VulkanBuffer *> &buffers) {
    std::vector<VkBufferMemoryBarrier> barriers(buffers.size());

    for(int i = 0; i < buffers.size(); i++) {
        barriers[i].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        barriers[i].srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        barriers[i].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        barriers[i].offset = 0;
        barriers[i].buffer = *buffers[i];
        barriers[i].size = buffers[i]->size;
    }

    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0,
                         nullptr, COUNT(barriers), barriers.data(), 0, nullptr);
}

std::vector<PipelineMetaData> PointHashGrid::pipelineMetaData() {
    return {
            {
                    "point_hash_grid_builder",
                    "../../data/shaders/sph/point_hash_grid_builder.comp.spv",
                    { &particleDescriptorSetLayout, &gridDescriptorSetLayout, &bucketSizeSetLayout},
                    {{VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(constants)}}
            },
            {
                    "prefix_scan",
                    "../../data/shaders/sph/scan.comp.spv",
                    { &prefixScan.setLayout }
            },
            {
                    "add",
                    "../../data/shaders/sph/add.comp.spv",
                    { &prefixScan.setLayout },
                    { { VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(prefixScan.constants)} }
            },
            {
                    "point_hash_grid_unit_test",
                    "../../data/shaders/test/point_hash_grid_test.comp.spv",
                    { &unitTestDescriptorSetLayout},
                    {{VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(constants)}}
            }
    };
}
