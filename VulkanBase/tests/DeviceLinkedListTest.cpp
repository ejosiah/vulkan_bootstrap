#include "VulkanFixture.hpp"
#include "DeviceLinkedList.hpp"

struct DeviceLinkedListFixture : public VulkanFixture{
protected:
    std::vector<PipelineMetaData> pipelineMetaData() override {
        return {
                {
                    "simple_linked_list",
                    "../../data/shaders/test/simple_linked_list.comp.spv",
                        { &sourceDataLayoutSet, &linkedList.setLayout }
                }
        };
    }

    void postVulkanInit() override {
        createSourceDescriptorSetLayout();
        linkedList = DeviceLinkedList<int>{ &device, &descriptorPool, 20};
        linkedList.init();
    }

    void createSourceDescriptorSetLayout(){
        std::vector<VkDescriptorSetLayoutBinding> bindings(1);
        bindings[0].binding = 0;
        bindings[0].descriptorCount = 1;
        bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        sourceDataLayoutSet = device.createDescriptorSetLayout(bindings);
    }

    void updateSourceDataDescriptorSet(void* data, VkDeviceSize size){
        sourceDataSet = descriptorPool.allocate({ sourceDataLayoutSet }).front();

        buffer = device.createDeviceLocalBuffer(data, size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
        auto writes = initializers::writeDescriptorSets<1>(sourceDataSet);
        VkDescriptorBufferInfo info{buffer, 0, VK_WHOLE_SIZE};
        writes[0].dstBinding = 0;
        writes[0].descriptorCount = 1;
        writes[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes[0].pBufferInfo = &info;

        device.updateDescriptorSets(writes);
    }

    VulkanDescriptorSetLayout sourceDataLayoutSet;
    VkDescriptorSet sourceDataSet;
    VulkanBuffer buffer;
    DeviceLinkedList<int> linkedList;

    void createLinkedList(){
        std::array<VkDescriptorSet, 2> sets{ sourceDataSet, linkedList.descriptorSet};
        execute([&](auto commandBuffer){
           vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline("simple_linked_list"));
           vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, layout("simple_linked_list"), 0, 2, sets.data(), 0,
                                   nullptr);
           vkCmdDispatch(commandBuffer, 1, 1, 1);
        });
    }

    template<typename Element>
    void deviceListContainsAll(std::vector<Element> data){
        auto listSize = linkedList.size();
        bool containsAll = true;
        linkedList.headBuffer.map<int>([&](auto headPtr){
            int head = headPtr[0];
            linkedList.nodeBuffer.map<typename DeviceLinkedList<Element>::Node>([&](auto nodePtr){
                for(auto& datum : data){
                    typename DeviceLinkedList<Element>::Node node = nodePtr[head];
                    bool found = false;
                    for(int i = 0; i < listSize; i++){
                        found = node.entry == datum;
//                        spdlog::error("Node[entry: {}, next: {}]", node.entry, node.next);
                        if(found) break;
                        node = nodePtr[node.next];
                    }
                    containsAll = containsAll && found;
                }
            });
        });
        ASSERT_TRUE(containsAll);
    }

    template<size_t Size>
    std::vector<int> generateRandomUniqueInts(){
        auto rng = rngFunc(-20, 20, 1 << 20);
        std::vector<int> res;
        while(res.size() != Size){
            auto item = rng();
            if(std::find(begin(res), end(res), item) != end(res)){
                continue;
            }
            res.push_back(item);
        }
        return res;
    }
};


TEST_F(DeviceLinkedListFixture, createAValidDeviceLinkedList){
    std::vector<int> expected = generateRandomUniqueInts<20>();
    updateSourceDataDescriptorSet(expected.data(), expected.size() * sizeof(int));
    createLinkedList();
    ASSERT_EQ(expected.size(), linkedList.size());

    deviceListContainsAll(expected);
}

TEST_F(DeviceLinkedListFixture, createMultipleDeviceLinkedList){
    FAIL() << "Not yet implemented!";
}