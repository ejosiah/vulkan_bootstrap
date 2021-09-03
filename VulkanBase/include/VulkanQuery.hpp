#pragma once

#include "common.h"
#include "VulkanDevice.h"

template<VkQueryType queryType>
struct VulkanQueryPool{
    DISABLE_COPY(VulkanQueryPool);

    VulkanQueryPool() = default;

    explicit VulkanQueryPool(VkDevice device, uint32_t queryCount, VkQueryPipelineStatisticFlags statsFlags = 0)
        : device{device}
        , queryCount(queryCount)
    {
        VkQueryPoolCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
        info.queryType = queryType;
        info.queryCount = queryCount;
        if constexpr (queryType == VK_QUERY_TYPE_PIPELINE_STATISTICS){
            info.pipelineStatistics = statsFlags;
        }

        ASSERT(vkCreateQueryPool(device, &info, nullptr, &queryPool));
    }

    template<VkQueryType srcQueryType>
    VulkanQueryPool(VulkanQueryPool<srcQueryType>&& source) noexcept {
        operator=(static_cast<VulkanQueryPool<srcQueryType>&&>(source));
    }

    template<VkQueryType srcQueryType>
    VulkanQueryPool& operator=(VulkanQueryPool<srcQueryType>&& source) noexcept {
        this->device = source.device;
        this->queryPool = source.queryPool;
        this->queryNames = std::move(source.queryNames);
        this->queryCount = source.queryCount;

        source.queryPool = VK_NULL_HANDLE;
        return *this;
    }

    ~VulkanQueryPool(){
        if(queryPool){
            vkDestroyQueryPool(device, queryPool, nullptr);
        }
    }

    void addQuery(const std::string& name){
        assert(queryNames.size() < queryCount);
        queryNames.insert(std::make_pair(name, queryNames.size()));
    }

    void reset(uint32_t firstQuery, uint32_t count) const {
        vkResetQueryPool(device, queryPool, firstQuery, count);
    }

    void reset(VkCommandBuffer commandBuffer, const std::string& name) const {
        auto query = getQuery(name);
        vkCmdResetQueryPool(commandBuffer, queryPool, query, 1);
    }

    [[nodiscard]]
    uint32_t getQuery(const std::string& name) const {
        auto itr = queryNames.find(name);
        assert(itr != end(queryNames));
        return itr->second;
    }

    void reset() const {
        vkResetQueryPool(device, queryPool, 0, queryCount);
    }

    template<typename Func>
    void query(VkCommandBuffer commandBuffer, const std::string& queryName, Func&& func, VkQueryControlFlags controlFlags = 0) {
        assert(!queryNames.empty());
        auto itr = queryNames.find(queryName);
        assert(itr != end(queryNames));
        auto query = itr->second;
        vkCmdBeginQuery(commandBuffer, queryPool, query, controlFlags);
        func();
        vkCmdEndQuery(commandBuffer, queryPool, query);
    }

//    template<typename Func, std::enable_if<queryType == VK_QUERY_TYPE_TIMESTAMP>>
//    void time(VkCommandBuffer commandBuffer, const std::string& queryName, Func&& func, VkQueryControlFlags controlFlags = 0) {
//        query(commandBuffer, queryName, func, controlFlags);
//    }

    operator VkQueryPool() const {
        return queryPool;
    }

    operator VkQueryPool*()  {
        return &queryPool;
    }


    template<typename Result>
    Result queryResult(VkQueryResultFlags flags = 0) const {
        uint32_t dataSize = sizeof(Result);
        VkDeviceSize stride = (flags & VK_QUERY_RESULT_64_BIT) ? sizeof(uint64_t) : sizeof(uint32_t);
        uint32_t queryCount = dataSize/stride;
        Result result;
        vkGetQueryPoolResults(device, queryPool, 0, queryCount, dataSize, &result, stride, flags);

        return result;
    }


    VkDevice device = VK_NULL_HANDLE;
    VkQueryPool queryPool = VK_NULL_HANDLE;
    std::map<std::string, uint32_t> queryNames;
    uint32_t queryCount = 0;
};

using TimestampQueryPool = VulkanQueryPool<VK_QUERY_TYPE_TIMESTAMP>;