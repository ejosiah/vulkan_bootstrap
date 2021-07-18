#pragma once

#include "common.h"
#include "VulkanDevice.h"


template<typename EntryType>
struct DeviceLinkedList{

    static constexpr int ATOMIC_COUNTER_BINDING = 0;
    static constexpr int HEAD_POINTER_BINDING = 1;
    static constexpr int NODE_BINDING = 2;
    static constexpr int NUM_DESCRIPTORS = 3;

    struct Node{
        EntryType entry;
        uint32_t next;
    };

    DeviceLinkedList() = default;

    DeviceLinkedList(VulkanDevice* device, VulkanDescriptorPool* descriptorPool, uint32_t maxListSize, uint32_t headCount = 1u)
            : device{device}
            , descriptorPool{descriptorPool}
            , headCount{ headCount }
            , maxListSize{ maxListSize }
    {}

    void init(){
        assert(device && descriptorPool);
        initBuffers();
        createDescriptorSetLayout();
        createDescriptorSet();
    }

    void initBuffers(){
        counterBuffer = device->createBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, sizeof(uint32_t) * headCount);
        headBuffer = device->createBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, sizeof(uint32_t) * headCount);
        nodeBuffer = device->createBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, sizeof(Node) * maxListSize);
    }

    void createDescriptorSetLayout(){
        std::vector<VkDescriptorSetLayoutBinding> bindings(NUM_DESCRIPTORS);

        bindings[ATOMIC_COUNTER_BINDING].binding = ATOMIC_COUNTER_BINDING;
        bindings[ATOMIC_COUNTER_BINDING].descriptorCount = 1;
        bindings[ATOMIC_COUNTER_BINDING].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[ATOMIC_COUNTER_BINDING].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;  // TODO make configurable

        bindings[HEAD_POINTER_BINDING].binding = HEAD_POINTER_BINDING;
        bindings[HEAD_POINTER_BINDING].descriptorCount = 1;
        bindings[HEAD_POINTER_BINDING].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[HEAD_POINTER_BINDING].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;  // TODO make configurable

        bindings[NODE_BINDING].binding = NODE_BINDING;
        bindings[NODE_BINDING].descriptorCount = 1;
        bindings[NODE_BINDING].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[NODE_BINDING].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;  // TODO make configurable

        setLayout = device->createDescriptorSetLayout(bindings);
    }

    void createDescriptorSet(){
        descriptorSet = descriptorPool->allocate({ setLayout }).front();

        auto writes = initializers::writeDescriptorSets<NUM_DESCRIPTORS>(descriptorSet);

        VkDescriptorBufferInfo counterInfo{ counterBuffer, 0, VK_WHOLE_SIZE };
        writes[ATOMIC_COUNTER_BINDING].dstBinding = ATOMIC_COUNTER_BINDING;
        writes[ATOMIC_COUNTER_BINDING].descriptorCount = 1;
        writes[ATOMIC_COUNTER_BINDING].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes[ATOMIC_COUNTER_BINDING].pBufferInfo = &counterInfo;

        VkDescriptorBufferInfo headInfo{ headBuffer, 0, VK_WHOLE_SIZE };
        writes[HEAD_POINTER_BINDING].dstBinding = HEAD_POINTER_BINDING;
        writes[HEAD_POINTER_BINDING].descriptorCount = 1;
        writes[HEAD_POINTER_BINDING].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes[HEAD_POINTER_BINDING].pBufferInfo = &headInfo;

        VkDescriptorBufferInfo nodeInfo{ nodeBuffer, 0, VK_WHOLE_SIZE };
        writes[NODE_BINDING].dstBinding = NODE_BINDING;
        writes[NODE_BINDING].descriptorCount = 1;
        writes[NODE_BINDING].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes[NODE_BINDING].pBufferInfo = &nodeInfo;

        device->updateDescriptorSets(writes);

    }

    [[nodiscard]]
    int size()  {
        return counterBuffer.get<int>(0);
    }

    VulkanDevice* device{ nullptr };
    VulkanDescriptorPool* descriptorPool{nullptr };
    VulkanDescriptorSetLayout setLayout;
    VkDescriptorSet descriptorSet{VK_NULL_HANDLE};
    VulkanBuffer counterBuffer;
    VulkanBuffer headBuffer;
    VulkanBuffer nodeBuffer;
    uint32_t headCount{0};
    uint32_t maxListSize{0};


};