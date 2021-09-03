#pragma once
#include "VulkanDevice.h"

class Profiler{
public:
    Profiler() = default;

    explicit  Profiler(VulkanDevice* device) : device(device)
    {
        queryPool = TimestampQueryPool{*device, QueryCount};
    }

    void addQuery(const std::string& name){
        if(queries.size() >= queryCount){
            queryCount += queryCount * 0.25;
            queryPool = TimestampQueryPool{*device, QueryCount};
        }
        Query query{name, queries.size()};
        query.startId = queries.size() * 2;
        query.endId = queries.size() * 2 + 1;
        queries.insert(std::make_pair(name, query));
    }

    void group(const string::string& groupName, std::initializer_list<std::string> queryNames){
        QueryGroup queryGroup = queryGroups.find(groupName) == end(queryGroup) ? QueryGroup{groupName} : queryGroups[groupName];
        for(auto query : queryNames){
            queryGroup.queries.push_back(query);
        }
        queryGroup[groupName] = queryGroup;
    }

    template<typename Body>
    void profile(const std::string& name, VkCommandBuffer commandBuffer, Body&& body){
        assert(queries.find(name) != end(queries));
        auto query = queries[name];
        vkCmdResetQueryPool(commandBuffer, queryPool, query.startId, 2);
        vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, queryPool, query.startId);
        body();
        vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, queryPool, query.endId);
    }

    void commit(){
        std::vector<uint64_t> counters(queries.size() * 2);

        VkQueryResultFlags flags = VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT;
        vkGetQueryPoolResults(*device, queryPool, 0, COUNT(counters), BYTE_SIZE(counters), counters.data(), sizeof(uint64_t), flags);

        for(auto& [_, query] : queries){
            auto start = counters[query.startId];
            auto end = counters[query.endId];
            auto runtime = end - start;
            query.runtimes.push_back(runtime);
        }

        for(auto& [_, group] : queryGroups){
            for(auto& queryName : group.queries){
                auto& query = queries[queryName];
                auto runtime = query.runtimes.back();
                group.runtimes.push_back(runtime);
            }
            group.runtimes[group.runtimes.size() - 1] /= group.queries.size();
        }
    }

private:
    struct Query{
        std::string name;
        uint32_t queryId{0};
        uint64_t startId{0};
        uint64_t endId{0};
        std::vector<uint64_t> runtimes{};
    };

    struct QueryGroup{
        std::string name;
        std::vector<std::string> queries;
        std::vector<uint64_t> runtimes{};
    };

    static constexpr uint32_t DEFAULT_QUERY_COUNT = 20;

    TimestampQueryPool queryPool;
    VulkanDevice* device = nullptr;
    uint32_t queryCount = DEFAULT_QUERY_COUNT;
    std::map<std::string, Query> queries;
    std::map<std::string, QueryGroup> queryGroups;
};