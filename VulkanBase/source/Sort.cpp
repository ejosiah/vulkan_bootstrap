#include "Sort.hpp"
#include "vulkan_util.h"

RadixSort::RadixSort(VulkanDevice *device, bool debug)
: GpuSort(device)
, debug(debug)
{

}

void RadixSort::init() {
    createDescriptorPool();
    createDescriptorSetLayouts();
    createDescriptorSets();
    createPipelines();
    createProfiler();

}

void RadixSort::createDescriptorPool() {
    constexpr uint maxSets = 3;
    std::vector<VkDescriptorPoolSize> poolSizes{
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, maxSets * 10}
    };

    descriptorPool = device->createDescriptorPool(maxSets, poolSizes);
}

void RadixSort::createDescriptorSetLayouts() {
    std::vector<VkDescriptorSetLayoutBinding> bindings(1);

    bindings[DATA].binding = DATA;
    bindings[DATA].descriptorCount = NUM_DATA_ELEMENTS;
    bindings[DATA].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[DATA].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    dataSetLayout = device->createDescriptorSetLayout(bindings);

    bindings.resize(2);
    bindings[COUNTS].binding = COUNTS;
    bindings[COUNTS].descriptorCount = NUM_DATA_ELEMENTS;
    bindings[COUNTS].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[COUNTS].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    bindings[SUMS].binding = SUMS;
    bindings[SUMS].descriptorCount = NUM_DATA_ELEMENTS;
    bindings[SUMS].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[SUMS].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    countsSetLayout = device->createDescriptorSetLayout(bindings);
}

void RadixSort::createDescriptorSets() {
    auto sets = descriptorPool.allocate({dataSetLayout, dataSetLayout, countsSetLayout});
    dataDescriptorSets[DATA_IN] = sets[0];
    dataDescriptorSets[DATA_OUT] = sets[1];
    countsDescriptorSet = sets[2];

    device->setName<VK_OBJECT_TYPE_DESCRIPTOR_SET>("data_in", dataDescriptorSets[DATA_IN]);
    device->setName<VK_OBJECT_TYPE_DESCRIPTOR_SET>("data_out", dataDescriptorSets[DATA_OUT]);
    device->setName<VK_OBJECT_TYPE_DESCRIPTOR_SET>("counts", countsDescriptorSet);
    device->setName<VK_OBJECT_TYPE_DESCRIPTOR_SET>("add_value", countsDescriptorSet);
}


std::vector<PipelineMetaData> RadixSort::pipelineMetaData() {
    return {
            {
                "count_radices",
                "../../data/shaders/radix_sort_li_grand/count_radices.comp.spv",
                    {&dataSetLayout, &countsSetLayout},
                    { {VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(constants)}}
            },
            {
                "prefix_sum",
                 "../../data/shaders/radix_sort_li_grand/prefix_sum.comp.spv",
                    { &countsSetLayout },
                    { {VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(constants)}}
            },
            {
                "reorder",
                    "../../data/shaders/radix_sort_li_grand/reorder.comp.spv",
                    {&dataSetLayout, &dataSetLayout, &countsSetLayout},
                    { {VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(constants)}}
            }
    };
}

VulkanBuffer RadixSort::sortWithIndices(VkCommandBuffer commandBuffer, VulkanBuffer &buffer) {
    operator()(commandBuffer, buffer);
    return std::move(indexBuffers[DATA_IN]);
}

void RadixSort::operator()(VkCommandBuffer commandBuffer, VulkanBuffer &buffer) {

    updateConstants(buffer);
    updateDataDescriptorSets(buffer);

    static std::array<VkDescriptorSet, 2> localDataSets;
    std::copy(begin(dataDescriptorSets), end(dataDescriptorSets), begin(localDataSets));
    dataBuffers[DATA_IN] = &buffer;
    dataBuffers[DATA_OUT] = &dataScratchBuffer;

    for(auto block = 0; block < PASSES; block++){
        constants.block = block;
        count(commandBuffer, localDataSets[DATA_IN]);
        prefixSum(commandBuffer);
        reorder(commandBuffer, localDataSets);
        std::swap(localDataSets[DATA_IN], localDataSets[DATA_OUT]);
    }

}

void RadixSort::updateConstants(VulkanBuffer& buffer) {
    workGroupCount = numWorkGroups(buffer);
    constants.Num_Elements = buffer.size/sizeof(uint);
    constants.Num_Groups_per_WorkGroup = NUM_THREADS_PER_BLOCK / WORD_SIZE;
    constants.Num_Elements_per_WorkGroup = nearestMultiple(constants.Num_Elements / workGroupCount , NUM_THREADS_PER_BLOCK);
    constants.Num_Elements_Per_Group = constants.Num_Elements_per_WorkGroup / constants.Num_Groups_per_WorkGroup;
    constants.Num_Radices_Per_WorkGroup = RADIX / workGroupCount;
    constants.Num_Groups = workGroupCount * constants.Num_Groups_per_WorkGroup;

}
void RadixSort::updateDataDescriptorSets(VulkanBuffer &dataBuffer) {
    assert(workGroupCount > 0);
    auto minOffset = device->getLimits().minStorageBufferOffsetAlignment;

    auto dataWrites = initializers::writeDescriptorSets<2>();

    std::array<VkDescriptorBufferInfo, 1> dataInfos{};
    dataInfos[VALUE] = {dataBuffer, 0, dataBuffer.size};

    dataWrites[DATA_IN].dstSet = dataDescriptorSets[DATA_IN];
    dataWrites[DATA_IN].dstBinding = DATA;
    dataWrites[DATA_IN].descriptorCount = NUM_DATA_ELEMENTS;
    dataWrites[DATA_IN].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    dataWrites[DATA_IN].pBufferInfo = dataInfos.data();

    dataScratchBuffer = device->createBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, dataBuffer.size);
    std::array<VkDescriptorBufferInfo, 1> dataOutInfos{};
    dataOutInfos[VALUE] = {dataScratchBuffer, 0, dataScratchBuffer.size};

    dataWrites[DATA_OUT].dstSet = dataDescriptorSets[DATA_OUT];
    dataWrites[DATA_OUT].dstBinding = DATA;
    dataWrites[DATA_OUT].descriptorCount = NUM_DATA_ELEMENTS;
    dataWrites[DATA_OUT].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    dataWrites[DATA_OUT].pBufferInfo = dataOutInfos.data();

    device->updateDescriptorSets(dataWrites);

    VkDeviceSize countsSize = RADIX * workGroupCount * NUM_GROUPS_PER_WORKGROUP * sizeof(uint);
    countsBuffer = device->createBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_ONLY, countsSize);
    auto countWrites = initializers::writeDescriptorSets<2>();
    
    VkDescriptorBufferInfo countsInfo{countsBuffer, 0, countsBuffer.size };
    countWrites[COUNTS].dstSet = countsDescriptorSet;
    countWrites[COUNTS].dstBinding = COUNTS;
    countWrites[COUNTS].descriptorCount = 1;
    countWrites[COUNTS].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    countWrites[COUNTS].pBufferInfo = &countsInfo;

    VkDeviceSize sumSize = (RADIX + 1) * sizeof(uint);
    sumBuffer = device->createBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_ONLY, sumSize);
    VkDescriptorBufferInfo sumInfo{sumBuffer, 0, sumBuffer.size};
    countWrites[SUMS].dstSet = countsDescriptorSet;
    countWrites[SUMS].dstBinding = SUMS;
    countWrites[SUMS].descriptorCount = 1;
    countWrites[SUMS].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    countWrites[SUMS].pBufferInfo = &sumInfo;

    device->updateDescriptorSets(countWrites);
}

uint RadixSort::numWorkGroups(VulkanBuffer &buffer) {
    const uint Num_Elements = buffer.size/sizeof(uint);
    return std::min(std::max(1u, Num_Elements/ ELEMENTS_PER_WG), MAX_WORKGROUPS);
}

void RadixSort::count(VkCommandBuffer commandBuffer, VkDescriptorSet dataDescriptorSet) {
    static std::array<VkDescriptorSet, 2> sets{};
    sets[0] = dataDescriptorSet;
    sets[1] = countsDescriptorSet;
    auto query = fmt::format("{}_{}", "count", constants.block);
    profiler.profile(query, commandBuffer, [&]{
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline("count_radices"));
        vkCmdPushConstants(commandBuffer, layout("count_radices"), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(constants), &constants);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, layout("count_radices"), 0, COUNT(sets), sets.data(), 0, VK_NULL_HANDLE);
        vkCmdDispatch(commandBuffer, workGroupCount, 1, 1);
        addComputeBufferMemoryBarriers(commandBuffer, { dataBuffers[DATA_IN], &countsBuffer });
    });

}

void RadixSort::prefixSum(VkCommandBuffer commandBuffer) {
    auto query = fmt::format("{}_{}", "prefix_sum", constants.block);
    profiler.profile(query, commandBuffer,  [&] {
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline("prefix_sum"));
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, layout("prefix_sum"), 0, 1,  &countsDescriptorSet, 0, VK_NULL_HANDLE);
        vkCmdPushConstants(commandBuffer, layout("prefix_sum"), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(constants), &constants);
        vkCmdDispatch(commandBuffer, workGroupCount, 1, 1);
        addComputeBufferMemoryBarriers(commandBuffer,{&countsBuffer, &sumBuffer, dataBuffers[DATA_IN], dataBuffers[DATA_OUT]});
    });
}

void RadixSort::reorder(VkCommandBuffer commandBuffer, std::array<VkDescriptorSet, 2> &dataDescriptorSets) {
    static std::array<VkDescriptorSet, 3> sets{};
    sets[0] = dataDescriptorSets[DATA_IN];
    sets[1] = dataDescriptorSets[DATA_OUT];
    sets[2] = countsDescriptorSet;
    auto query = fmt::format("{}_{}", "reorder", constants.block);
    profiler.profile(query, commandBuffer, [&] {
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline("reorder"));
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, layout("reorder"), 0, COUNT(sets), sets.data(), 0, VK_NULL_HANDLE);
        vkCmdPushConstants(commandBuffer, layout("reorder"), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(constants), &constants);
        vkCmdDispatch(commandBuffer, workGroupCount, 1, 1);
        if (constants.block < PASSES - 1) {
            addComputeBufferMemoryBarriers(commandBuffer, {dataBuffers[DATA_IN], dataBuffers[DATA_OUT]});
        }
    });
}

void RadixSort::createProfiler() {
    if(debug) {
        profiler = Profiler{device, PASSES * 6};
        profiler.addGroup("count", PASSES);
        profiler.addGroup("prefix_sum", PASSES);
        profiler.addGroup("reorder", PASSES);
    }
}

void RadixSort::commitProfiler() {
    profiler.commit();
}



