#include "VulkanFixture.hpp"
#include "DescriptorSetBuilder.hpp"
#include "FourWayRadixSort.hpp"

class FourWayRadixLocalSortFixture : public VulkanFixture{
protected:
    std::vector<PipelineMetaData> pipelineMetaData() override {
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
                }
        };
    }

    void postVulkanInit() override {
        initBuffers();
        createDescriptorSet();
    }

    void initBuffers(){
        dataInBuffer = device.createCpuVisibleBuffer(elements.data(), BYTE_SIZE(elements), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
        dataOutBuffer = device.createBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, BYTE_SIZE(elements));
        prefixSumBuffer = device.createBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, BYTE_SIZE(elements));
        blockSumBuffer = device.createBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, BYTE_SIZE(elements));
    }

    void createDescriptorSet(){
        dataLayoutSet = device.descriptorSetLayoutBuilder()
                .binding(0)
                    .descriptorCount(1)
                    .descriptorType(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
                    .shaderStages(VK_SHADER_STAGE_COMPUTE_BIT)
                .createLayout();

        scanLayoutSet = device.descriptorSetLayoutBuilder()
                .binding(0)
                    .descriptorCount(1)
                    .descriptorType(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
                    .shaderStages(VK_SHADER_STAGE_COMPUTE_BIT)
                .binding(1)
                    .descriptorCount(1)
                    .descriptorType(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
                    .shaderStages(VK_SHADER_STAGE_COMPUTE_BIT)
                .createLayout();

        auto sets = descriptorPool.allocate({ dataLayoutSet, dataLayoutSet, scanLayoutSet });
        dataDescriptorSets[0] = sets[0];
        dataDescriptorSets[1] = sets[1];
        scanDescriptorSet = sets[2];

        auto writes = initializers::writeDescriptorSets<4>();

        VkDescriptorBufferInfo dataInInfo{ dataInBuffer, 0, VK_WHOLE_SIZE};
        writes[0].dstSet = dataDescriptorSets[0];
        writes[0].dstBinding = 0;
        writes[0].descriptorCount = 1;
        writes[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes[0].pBufferInfo = &dataInInfo;

        VkDescriptorBufferInfo dataOutInfo{ dataOutBuffer, 0, VK_WHOLE_SIZE};
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

        device.updateDescriptorSets(writes);

    }

    void localSort(){
        execute([&](auto commandBuffer){
            static std::array<VkDescriptorSet, 3> sets{};
            sets[0] = dataDescriptorSets[0];
            sets[1] = dataDescriptorSets[1];
            sets[2] = scanDescriptorSet;
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, layout("local_sort"), 0, COUNT(sets), sets.data(), 0, VK_NULL_HANDLE);
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline("local_sort"));
            vkCmdPushConstants(commandBuffer, layout("local_sort"), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(constants), &constants);
            vkCmdDispatch(commandBuffer, 5, 1, 1);
        });
    }


    std::vector<uint> elements{1 , 2, 0, 3, 0, 1, 1, 0, 3, 3, 3, 2, 1, 2, 2, 0, 2, 0, 0, 2};
    VulkanBuffer dataInBuffer;
    VulkanBuffer dataOutBuffer;
    VulkanBuffer prefixSumBuffer;
    VulkanBuffer blockSumBuffer;
    VulkanDescriptorSetLayout scanLayoutSet;
    VulkanDescriptorSetLayout dataLayoutSet;
    VkDescriptorSet scanDescriptorSet = VK_NULL_HANDLE;
    std::array<VkDescriptorSet, 2> dataDescriptorSets{};
    struct{
        uint shift_width{0};
        uint data_length{20};
    } constants{};

    static constexpr uint MAX_ELEMENTS_PER_WORKGROUP = 4;
    std::array<uint32_t, 2> constData{MAX_ELEMENTS_PER_WORKGROUP, MAX_ELEMENTS_PER_WORKGROUP};

};

TEST_F(FourWayRadixLocalSortFixture, localRadixSort){
    localSort();

    std::vector<uint> expectedPrefixSum{0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 2, 0, 0, 0, 1, 0, 0, 0, 1, 1};
    prefixSumBuffer.map<uint>([&](auto actualPrefixSum){
        for(int i = 0; i < 20; i++){
            fmt::print("{} ", actualPrefixSum[i]);
        }
        for(int i = 0; i < 20; i++){
            ERR_GUARD_VULKAN_EQ(expectedPrefixSum[i], actualPrefixSum[i]);
        }
    });
    fmt::print("\n");

    std::vector<uint> expectedBlockSum{1, 2, 0, 1, 2, 1, 2, 0, 1, 0, 1, 0, 1, 2, 2, 1, 0, 3, 0, 0};
    blockSumBuffer.map<uint>([&](auto actualBlockSum){
        for(int i = 0; i < 20; i++){
            fmt::print("{} ", actualBlockSum[i]);
        }
        for(int i = 0; i < 20; i++){
            ERR_GUARD_VULKAN_EQ(expectedBlockSum[i], actualBlockSum[i]);
        }
    });
    fmt::print("\n");
    std::vector<uint> expectedData{0, 1, 2, 3, 0, 0, 1, 1, 2, 3, 3, 3, 0, 1, 2, 2, 0, 0, 2, 2};


    dataOutBuffer.map<uint>([&](auto actualData){
        for(int i = 0; i < 20; i++){
            fmt::print("{} ", actualData[i]);
        }
        for(int i = 0; i < 20; i++){
            ERR_GUARD_VULKAN_EQ(expectedData[i], actualData[i]);
        }
    });
}