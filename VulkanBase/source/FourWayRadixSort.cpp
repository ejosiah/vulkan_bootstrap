#include "FourWayRadixSort.hpp"
#include "vulkan_util.h"

FourWayRadixSort::FourWayRadixSort(VulkanDevice *device, uint maxElementsPerWorkGroup, bool debug)
    : GpuSort(device)
    , debug(debug)
{
    constData[0] = constData[1] = maxElementsPerWorkGroup;
}

void FourWayRadixSort::init() {
    createDescriptorPool();
    createDescriptorSetLayouts();
    createDescriptorSets();
    createPipelines();
    prefixSum = PrefixSum{device};
    prefixSum.init();
    initProfiler();
}

void FourWayRadixSort::operator()(VkCommandBuffer commandBuffer, VulkanBuffer &buffer) {
    numWorkGroups = calculateNumWorkGroups(buffer);
    constants.data_length = buffer.size/sizeof(uint);
    updateDataDescriptorSets(buffer);

    for(constants.shift_width  = 0; constants.shift_width  <= 30; constants.shift_width += 2){
        localSort(commandBuffer);
        scan(commandBuffer);
        globalShuffle(commandBuffer);
    }
}

void FourWayRadixSort::localSort(VkCommandBuffer commandBuffer) {
    static std::array<VkDescriptorSet, 3> sets{};
    sets[0] = dataDescriptorSets[DATA_IN];
    sets[1] = dataDescriptorSets[DATA_OUT];
    sets[2] = scanDescriptorSet;
    auto query = fmt::format("{}_{}", "local_sort", constants.shift_width/2);
    profiler.profile(query, commandBuffer, [&]{
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, layout("local_sort"), 0, COUNT(sets), sets.data(), 0, VK_NULL_HANDLE);
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline("local_sort"));
        vkCmdPushConstants(commandBuffer, layout("local_sort"), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(constants), &constants);
        vkCmdDispatch(commandBuffer, numWorkGroups, 1, 1);
        addComputeBufferMemoryBarriers(commandBuffer, { &blockSumBuffer });
    });
}

void FourWayRadixSort::scan(VkCommandBuffer commandBuffer) {
    auto query = fmt::format("{}_{}", "prefix_sum", constants.shift_width/2);
    profiler.profile(query, commandBuffer, [&]{
        prefixSum(commandBuffer, blockSumBuffer);
        addComputeBufferMemoryBarriers(commandBuffer, { dataBuffers[DATA_IN], dataBuffers[DATA_OUT], &prefixSumBuffer, &blockSumBuffer });
    });
}

void FourWayRadixSort::globalShuffle(VkCommandBuffer commandBuffer) {
    static std::array<VkDescriptorSet, 3> sets{};
    sets[0] = dataDescriptorSets[DATA_OUT];
    sets[1] = dataDescriptorSets[DATA_IN];
    sets[2] = scanDescriptorSet;

    auto query = fmt::format("{}_{}", "global_shuffle", constants.shift_width/2);
    profiler.profile(query, commandBuffer, [&]{
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, layout("global_shuffle"), 0, COUNT(sets), sets.data(), 0, VK_NULL_HANDLE);
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline("global_shuffle"));
        vkCmdPushConstants(commandBuffer, layout("global_shuffle"), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(constants), &constants);
        vkCmdDispatch(commandBuffer, numWorkGroups, 1, 1);
        if(constants.shift_width < 30){
            addComputeBufferMemoryBarriers(commandBuffer, { dataBuffers[DATA_IN], dataBuffers[DATA_OUT], &prefixSumBuffer, &blockSumBuffer });
        }
    });
}

std::vector<PipelineMetaData> FourWayRadixSort::pipelineMetaData() {
    return {
            {
                "local_sort",
                "../../data/shaders/radix_sort_4_way/local_sort.comp.spv",
                { &dataLayoutSet, &dataLayoutSet, &scanLayoutSet },
                { {VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(constants)} },
                {
                    {{0, 0, sizeof(uint)}, {1, sizeof(uint), sizeof(uint)}},
                    constData.data(),
                    BYTE_SIZE(constData)
                }
            },
            {
                "global_shuffle",
                "../../data/shaders/radix_sort_4_way/global_shuffle.comp.spv",
                { &dataLayoutSet, &dataLayoutSet, &scanLayoutSet },
                { {VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(constants)} },
                {
                    {{0, 0, sizeof(uint)}, {1, sizeof(uint), sizeof(uint)}},
                    constData.data(),
                    BYTE_SIZE(constData)
                }
            }
    };
}

void FourWayRadixSort::createDescriptorPool() {
    constexpr uint maxSets = 3;
    std::vector<VkDescriptorPoolSize> poolSizes{
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, maxSets * 10}
    };

    descriptorPool = device->createDescriptorPool(maxSets, poolSizes);
}

void FourWayRadixSort::createDescriptorSetLayouts() {
    dataLayoutSet =
        device->descriptorSetLayoutBuilder()
            .binding(0)
                .descriptorCount(1)
                .descriptorType(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
                .shaderStages(VK_SHADER_STAGE_COMPUTE_BIT)
            .createLayout();

    scanLayoutSet =
        device->descriptorSetLayoutBuilder()
            .binding(0)
                .descriptorCount(1)
                .descriptorType(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
                .shaderStages(VK_SHADER_STAGE_COMPUTE_BIT)
            .binding(1)
                .descriptorCount(1)
                .descriptorType(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
                .shaderStages(VK_SHADER_STAGE_COMPUTE_BIT)
            .createLayout();
}

void FourWayRadixSort::createDescriptorSets() {
    auto sets = descriptorPool.allocate({ dataLayoutSet, dataLayoutSet, scanLayoutSet });
    dataDescriptorSets[0] = sets[0];
    dataDescriptorSets[1] = sets[1];
    scanDescriptorSet = sets[2];
}

void FourWayRadixSort::updateDataDescriptorSets(VulkanBuffer &buffer) {
    scratchBuffer = device->createBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, buffer.size);
    prefixSumBuffer = device->createBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, buffer.size);
    blockSumBuffer = device->createBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, buffer.size);
    dataBuffers[0] = &buffer;
    dataBuffers[1] = &scratchBuffer;

    auto writes = initializers::writeDescriptorSets<4>();

    VkDescriptorBufferInfo dataInInfo{ buffer, 0, VK_WHOLE_SIZE};
    writes[0].dstSet = dataDescriptorSets[0];
    writes[0].dstBinding = 0;
    writes[0].descriptorCount = 1;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[0].pBufferInfo = &dataInInfo;

    VkDescriptorBufferInfo dataOutInfo{ scratchBuffer, 0, VK_WHOLE_SIZE};
    writes[1].dstSet = dataDescriptorSets[1];
    writes[1].dstBinding = 0;
    writes[1].descriptorCount = 1;
    writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[1].pBufferInfo = &dataOutInfo;

    VkDescriptorBufferInfo prefixSumInfo{ prefixSumBuffer, 0, VK_WHOLE_SIZE};
    writes[2].dstSet = scanDescriptorSet;
    writes[2].dstBinding = 0;
    writes[2].descriptorCount = 1;
    writes[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[2].pBufferInfo = &prefixSumInfo;

    VkDescriptorBufferInfo blockSumInfo{ blockSumBuffer, 0, VK_WHOLE_SIZE};
    writes[3].dstSet = scanDescriptorSet;
    writes[3].dstBinding = 1;
    writes[3].descriptorCount = 1;
    writes[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[3].pBufferInfo = &blockSumInfo;

    device->updateDescriptorSets(writes);

    prefixSum.updateDataDescriptorSets(blockSumBuffer);
}

uint FourWayRadixSort::calculateNumWorkGroups(VulkanBuffer& buffer) {
    int numItems = static_cast<int>(buffer.size/sizeof(uint));
    uint itemsPerWorkGroup = constData[0];
    return static_cast<uint>(std::abs(numItems - 1)/itemsPerWorkGroup + 1);
}

void FourWayRadixSort::initProfiler() {
    if(debug){
        profiler = Profiler{ device, 3 * 32};
        profiler.addGroup("local_sort", 16);
        profiler.addGroup("prefix_sum", 16);
        profiler.addGroup("global_shuffle", 16);
    }
}
