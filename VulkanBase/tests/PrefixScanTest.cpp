#include "VulkanFixture.hpp"

constexpr int ITEMS_PER_WORKGROUP = 8 << 10;

class PrefixScanTest : public VulkanFixture{
protected:

    struct {
        int itemsPerWorkGroup = ITEMS_PER_WORKGROUP;
        int N = 0;
    } constants;

    VkDescriptorSet descriptorSet;
    VkDescriptorSet sumScanDescriptorSet;
    VulkanDescriptorSetLayout setLayout;
    VulkanBuffer buffer;
    VulkanBuffer sumsBuffer;
    uint32_t bufferOffsetAlignment;

    void postVulkanInit() override {
        bufferOffsetAlignment = device.getLimits().minStorageBufferOffsetAlignment;
    }

    void createDescriptorSet(const VulkanDescriptorSetLayout& layout){
        auto sets = descriptorPool.allocate({ layout, layout });
        descriptorSet = sets.front();
        sumScanDescriptorSet = sets.back();
    }

    std::vector<PipelineMetaData> pipelineMetaData() final {
        std::array<VkDescriptorSetLayoutBinding, 2> bindings{};

        bindings[0].binding = 0;
        bindings[0].descriptorCount = 1;
        bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        bindings[1].binding = 1;
        bindings[1].descriptorCount = 1;
        bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;


        setLayout = device.createDescriptorSetLayout(bindings);
        createDescriptorSet(setLayout);


        PipelineMetaData metaData{
                "prefix_scan",
                "../../data/shaders/prefix_scan/scan.comp.spv",
                { &setLayout }
        };


        std::vector<PipelineMetaData> metas;
        metas.push_back(metaData);

        metaData = {
                "add",
                "../../data/shaders/prefix_scan/add.comp.spv",
                { &setLayout },
                { { VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(constants)} }
        };

        metas.push_back(metaData);
        return metas;
    }

    template<typename T>
    void fillBuffer(std::vector<T> data){
        buffer = device.createCpuVisibleBuffer(data.data(), sizeof(T) * data.size(), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
        VkDeviceSize sumsSize = (std::abs(int(data.size() - 1))/ITEMS_PER_WORKGROUP + 1) * sizeof(T);  // FIXME
        sumsSize = alignedSize(sumsSize, bufferOffsetAlignment) + sizeof(T);
        sumsBuffer = device.createBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, sumsSize);
        constants.N = data.size();
        updateDescriptorSet();
    }

    void updateDescriptorSet(){
        VkDescriptorBufferInfo info{ buffer, 0, VK_WHOLE_SIZE};
        auto writes = initializers::writeDescriptorSets<4>(descriptorSet);
        writes[0].dstSet = descriptorSet;
        writes[0].dstBinding = 0;
        writes[0].descriptorCount = 1;
        writes[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes[0].pBufferInfo = &info;

        VkDescriptorBufferInfo sumsInfo{ sumsBuffer, 0, sumsBuffer.size - sizeof(int)}; // TODO FIXME
        writes[1].dstSet = descriptorSet;
        writes[1].dstBinding = 1;
        writes[1].descriptorCount = 1;
        writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes[1].pBufferInfo = &sumsInfo;

        // for sum scan
        writes[2].dstSet = sumScanDescriptorSet;
        writes[2].dstBinding = 0;
        writes[2].descriptorCount = 1;
        writes[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes[2].pBufferInfo = &sumsInfo;

        VkDescriptorBufferInfo sumsSumInfo{ sumsBuffer, sumsBuffer.size - sizeof(int), VK_WHOLE_SIZE}; // TODO FIXME
        writes[3].dstSet = sumScanDescriptorSet;
        writes[3].dstBinding = 1;
        writes[3].descriptorCount = 1;
        writes[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes[3].pBufferInfo = &sumsSumInfo;

        device.updateDescriptorSets(writes);
    }

    void addBufferMemoryBarriers(VkCommandBuffer commandBuffer, const std::vector<VulkanBuffer*>& buffers){
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

    void gpuScan(){
        int size = buffer.size/sizeof(int);
        int numWorkGroups = std::abs(size - 1)/ITEMS_PER_WORKGROUP + 1;
        execute([&](auto commandBuffer){

            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline("prefix_scan"));
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, layout("prefix_scan"), 0, 1, &descriptorSet, 0, nullptr);
            vkCmdDispatch(commandBuffer, numWorkGroups, 1, 1);

            if(numWorkGroups > 1){
                addBufferMemoryBarriers(commandBuffer, {&buffer, &sumsBuffer});
                vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, layout("prefix_scan"), 0, 1, &sumScanDescriptorSet, 0, nullptr);
                vkCmdDispatch(commandBuffer, 1, 1, 1);

                addBufferMemoryBarriers(commandBuffer, { &buffer, &sumsBuffer });
                vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline("add"));
                vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, layout("add"), 0, 1, &descriptorSet, 0, nullptr);
                vkCmdPushConstants(commandBuffer, layout("add"), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(constants), &constants);
                vkCmdDispatch(commandBuffer, numWorkGroups, 1, 1);
            }
        });
    }

    void assertScan(const std::vector<int>& expected){
        buffer.map<int>([&](auto actual){
            for(int i = 0; i < expected.size(); i++){
                ASSERT_EQ(expected[i], actual[i] ) << fmt::format("expected {} at index {} but got {}", expected[i], i, actual[i]);
            }
        });
    }
};


TEST_F(PrefixScanTest, ScanWithSingleWorkGroup){
    std::vector<int> data(ITEMS_PER_WORKGROUP);
    auto rng = rngFunc<int>(0, 100, 1 << 20);

    std::generate(begin(data), end(data), [&]{ return rng(); });
    fillBuffer(data);
    std::exclusive_scan(begin(data), end(data), begin(data), 0);

    gpuScan();
    assertScan(data);

}

TEST_F(PrefixScanTest, ScanDataItemsLessThanWorkGroupSize){
    std::vector<int> data(ITEMS_PER_WORKGROUP/2);
    auto rng = rngFunc<int>(0, 100, 1 << 20);
    std::generate(begin(data), end(data), [&]{ return rng(); });
    fillBuffer(data);
    std::exclusive_scan(begin(data), end(data), begin(data), 0);

    gpuScan();
    assertScan(data);
}

TEST_F(PrefixScanTest, ScanNonPowerOfDataItems){
    std::vector<int> data(55555);
    auto rng = rngFunc<int>(0, 100, 1 << 20);
    std::generate(begin(data), end(data), [&]{ return rng(); });
    fillBuffer(data);
    std::exclusive_scan(begin(data), end(data), begin(data), 0);

    gpuScan();
    assertScan(data);
}

TEST_F(PrefixScanTest, ScanWithMutipleWorkGroups){
    std::vector<int> data(ITEMS_PER_WORKGROUP + 1);
    auto rng = rngFunc<int>(0, 100, 1 << 20);
    std::generate(begin(data), end(data), [&]{ return rng(); });

    fillBuffer(data);
    std::exclusive_scan(begin(data), end(data), begin(data), 0);
    gpuScan();
    assertScan(data);
}
