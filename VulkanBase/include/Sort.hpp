#pragma once

#include "ComputePipelins.hpp"
#include "VulkanQuery.hpp"


class GpuSort : public ComputePipelines{
public:
    explicit GpuSort(VulkanDevice* device = nullptr) : ComputePipelines(device){};

    virtual void init() {};

    virtual void operator()(VkCommandBuffer commandBuffer, VulkanBuffer& buffer) = 0;

    template<typename Itr>
    void sort(const Itr _first, const Itr _last){
        VkDeviceSize size = sizeof(decltype(*_first)) * std::distance(_first, _last);
        void* source = reinterpret_cast<void*>(&*_first);
        VkBufferUsageFlags flags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        VulkanBuffer buffer = device->createCpuVisibleBuffer(source, size, flags);
        device->graphicsCommandPool().oneTimeCommand([&buffer, this](auto cmdBuffer){
            operator()(cmdBuffer, buffer);
        });
        void* sorted = buffer.map();
        std::memcpy(source, sorted, size);
        buffer.unmap();
    }
};



class RadixSort : public GpuSort{
    // FIXME these constants are based on shared memory size of 8,192
    // FIXME set values based on device memory limits
    static constexpr uint WORD_SIZE = 32;
    static constexpr uint BLOCK_SIZE = 8;
    static constexpr uint RADIX = 256;
    static constexpr uint PASSES = WORD_SIZE / BLOCK_SIZE;
    static constexpr uint DATA_IN = 0;
    static constexpr uint DATA_OUT = 1;
    static constexpr uint DATA = 0;
    static constexpr uint ADD_IN = 0;
    static constexpr uint ADD_OUT = 1;
    static constexpr uint VALUE = 0;
    static constexpr uint COUNTS = 0;
    static constexpr uint SUMS = 1;
    static constexpr uint NUM_DATA_ELEMENTS = 1;
    static constexpr uint ELEMENTS_PER_WG = 1 << 14;
    static constexpr uint MAX_WORKGROUPS = 64;
    static constexpr uint NUM_THREADS_PER_BLOCK = 1024;
    static constexpr uint NUM_GROUPS_PER_WORKGROUP = NUM_THREADS_PER_BLOCK / WORD_SIZE;

    enum Query  { COUNT, PREFIX_SUM, REORDER, NUM_QUERIES };

public:
    class Profiler{
    public:
        Profiler() = default;

        explicit  Profiler(VulkanDevice* device, bool active = false)
        : device(device)
        , active(active)
        {
            if(active) {
                queryPool = TimestampQueryPool{*device, QueryCount};
            }
        }


        template<typename Body>
        void profile(VkCommandBuffer commandBuffer, int pass, Query query, Body&& body){
            if(active) {
                uint32_t startId = (pass * NUM_QUERIES + query) * 2;
                uint32_t endId = startId + 1;
                vkCmdResetQueryPool(commandBuffer, queryPool, startId, 2);
                vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, queryPool, startId);
                body();
                vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, queryPool, endId);
            }else{
                body();
            }
        }

        void commit(){
            if(active) {
                VkQueryResultFlags flags = VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT;
                vkGetQueryPoolResults(*device, queryPool, 0, QueryCount, sizeof(StopWatch) * PASSES,
                                      timesPerPass.data(), sizeof(uint64_t), flags);

                Runtime rt{};
                auto timestampPeriod = device->timestampPeriod();
                for (auto &stopWatch : timesPerPass) {
                    rt.count += (stopWatch.countStop - stopWatch.countStart) * timestampPeriod;
                    rt.prefixSum += (stopWatch.prefixSumStop - stopWatch.prefixSumStart) * timestampPeriod;
                    rt.reorder += (stopWatch.reorderStop - stopWatch.reorderStart) * timestampPeriod;
                }
                rt.count /= PASSES;
                rt.prefixSum /= PASSES;
                rt.prefixSum /= PASSES;
                runtimes.push_back(rt);
            }
        }

        struct Runtime{
            uint64_t count{0};
            uint64_t prefixSum{0};
            uint64_t reorder{0};
        };
        std::vector<Runtime> runtimes{};

    private:
        static constexpr uint32_t QueryCount = 24;
        struct StopWatch {
            uint64_t countStart;
            uint64_t countStop;
            uint64_t prefixSumStart;
            uint64_t prefixSumStop;
            uint64_t reorderStart;
            uint64_t reorderStop;
        };

        std::array<StopWatch, PASSES> timesPerPass{};
        TimestampQueryPool queryPool;
        VulkanDevice* device = nullptr;
        bool active = false;
    };

public:
    explicit RadixSort(VulkanDevice* device = nullptr, bool debug = false);

    void init() override;

    void createDescriptorPool();

    void createDescriptorSetLayouts();

    void createDescriptorSets();

    void createProfiler();

    std::vector<PipelineMetaData> pipelineMetaData() override;

    void operator()(VkCommandBuffer commandBuffer, VulkanBuffer &buffer) override;

    VulkanBuffer sortWithIndices(VkCommandBuffer commandBuffer, VulkanBuffer &buffer);

    void updateConstants(VulkanBuffer& buffer);

    static uint numWorkGroups(VulkanBuffer& buffer);

    void count(VkCommandBuffer commandBuffer, VkDescriptorSet dataDescriptorSet);

    void prefixSum(VkCommandBuffer commandBuffer);

    void reorder(VkCommandBuffer commandBuffer, std::array<VkDescriptorSet, 2>& dataDescriptorSets);

    void updateDataDescriptorSets(VulkanBuffer& inBuffer);

    void commitProfiler();

    Profiler profiler;

protected:
    VulkanDescriptorPool descriptorPool;
    VulkanDescriptorSetLayout dataSetLayout;
    VulkanDescriptorSetLayout countsSetLayout;
    std::array<VkDescriptorSet, 2> dataDescriptorSets;
    VkDescriptorSet countsDescriptorSet;
    VulkanBuffer countsBuffer;
    VulkanBuffer sumBuffer;
    std::array<VulkanBuffer*, 2> dataBuffers;
    std::array<VulkanBuffer, 2> indexBuffers;
    VulkanBuffer dataScratchBuffer;
    uint workGroupCount = 0;
    bool debug = false;

    struct {
        uint block;
        uint R = WORD_SIZE;
        uint Radix = RADIX;
        uint Num_Groups_per_WorkGroup;
        uint Num_Elements_per_WorkGroup;
        uint Num_Elements_Per_Group;
        uint Num_Elements;
        uint Num_Radices_Per_WorkGroup;
        uint Num_Groups;
        uint reorderIndices = false;
    } constants;

};