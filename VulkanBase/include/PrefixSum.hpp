#pragma once

#include "ComputePipelins.hpp"
#include "DescriptorSetBuilder.hpp"

class PrefixSum : public ComputePipelines{
public:
    PrefixSum(VulkanDevice* device = nullptr, VulkanCommandPool* commandPool = nullptr);

    void init();

    std::vector<PipelineMetaData> pipelineMetaData() override;

    void operator()(VkCommandBuffer commandBuffer, VulkanBuffer& buffer);

    template<typename Itr>
    void scan(const Itr _first, const Itr _last){
        VkDeviceSize size = sizeof(decltype(*_first)) * std::distance(_first, _last);
        void* source = reinterpret_cast<void*>(&*_first);
        VulkanBuffer buffer = device->createCpuVisibleBuffer(source, size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
        updateDataDescriptorSets(buffer);
        _commandPool = _commandPool ? _commandPool : const_cast<VulkanCommandPool*>(&device->graphicsCommandPool());
        _commandPool->oneTimeCommand([&buffer, this](auto cmdBuffer) {
            operator()(cmdBuffer, buffer);
        });
        void* result = buffer.map();
        std::memcpy(source, result, size);
        buffer.unmap();
    }

    void createDescriptorPool();

    void updateDataDescriptorSets(VulkanBuffer& buffer);

protected:
    static constexpr int ITEMS_PER_WORKGROUP = 8 << 10;

    void createDescriptorSet();

    void addBufferMemoryBarriers(VkCommandBuffer commandBuffer, const std::vector<VulkanBuffer*>& buffers);

private:
    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
    VkDescriptorSet sumScanDescriptorSet = VK_NULL_HANDLE;
    VulkanDescriptorSetLayout setLayout;
    VulkanBuffer sumsBuffer;
    uint32_t bufferOffsetAlignment;
    VulkanDescriptorPool descriptorPool;
    VulkanCommandPool* _commandPool{};

    struct {
        int itemsPerWorkGroup = ITEMS_PER_WORKGROUP;
        int N = 0;
    } constants;

};
