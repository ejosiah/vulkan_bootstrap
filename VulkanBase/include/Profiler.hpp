#pragma once
#include "VulkanDevice.h"
#include <optional>

class Profiler{
public:
    struct Query{
        std::string name;
        uint64_t startId{0};
        uint64_t endId{0};
        std::vector<uint64_t> runtimes{};
    };

    struct QueryGroup{
        std::string name;
        std::vector<std::string> queries;
        std::vector<uint64_t> runtimes{};
    };

    Profiler() = default;

    explicit  Profiler(VulkanDevice* device, uint32_t queryCount = 20)
    : device(device)
    , queryCount(std::max(queryCount, DEFAULT_QUERY_COUNT))
    {
        queryPool = TimestampQueryPool{*device, queryCount};
    }

    void addQuery(const std::string& name){
        if(!queryPool && !device) return;
        if(queries.size() >= queryCount){
            queryCount += queryCount * 0.25;
            queryPool = TimestampQueryPool{*device, queryCount};
        }
        Query query{name};
        query.startId = queries.size() * 2;
        query.endId = queries.size() * 2 + 1;
        queries.insert(std::make_pair(name, query));
    }

    template<typename Queries>
    void group(const std::string& groupName, Queries queries){
        if(!queryPool && !device) return;
        QueryGroup queryGroup = queryGroups.find(groupName) == end(queryGroups) ? QueryGroup{groupName} : queryGroups[groupName];
        for(auto _query : queries){
            queryGroup.queries.push_back(_query);
        }
        queryGroups[groupName] = queryGroup;
    }

    void addGroup(const std::string& name, int queries){
        if(!queryPool && !device) return;
        assert(queries > 0);
        std::vector<std::string> queryNames;
        for(auto i = 0; i < queries; i++){
            auto queryName = fmt::format("{}_{}", name, i);
            addQuery(queryName);
            queryNames.push_back(queryName);
        }
        group(name, queryNames);
    }

    template<typename Body>
    void profile(const std::string& name, VkCommandBuffer commandBuffer, Body&& body){
        if(!queryPool && !device){
            body();
            return;
        }
        assert(queries.find(name) != end(queries));
        auto query = queries[name];
        vkCmdResetQueryPool(commandBuffer, queryPool, query.startId, 2);
        vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, queryPool, query.startId);
        body();
        vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, queryPool, query.endId);
    }

    void commit(){
        if(!queryPool && !device) return;
        std::vector<uint64_t> counters(queries.size() * 2);

        VkQueryResultFlags flags = VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT;
        vkGetQueryPoolResults(*device, queryPool, 0, COUNT(counters), BYTE_SIZE(counters), counters.data(), sizeof(uint64_t), flags);

        auto timestampPeriod = device->timestampPeriod();
        for(auto& [_, query] : queries){
            auto start = counters[query.startId];
            auto end = counters[query.endId];
            auto runtime = (end - start) * timestampPeriod;
            query.runtimes.push_back(runtime);
        }

        for(auto& [_, group] : queryGroups){
            uint64_t runtime = 0;
            for(auto& queryName : group.queries){
                auto& query = queries[queryName];
                runtime += query.runtimes.back();
            }
            runtime /= group.queries.size();
            group.runtimes.push_back(runtime);
        }
    }

    std::optional<QueryGroup> getGroup(const std::string& name){
        if(queryGroups.find(name) != end(queryGroups)){
            return queryGroups[name];
        }
        return {};
    }

private:
    static constexpr uint32_t DEFAULT_QUERY_COUNT = 20;

    TimestampQueryPool queryPool;
    VulkanDevice* device = nullptr;
    uint32_t queryCount = DEFAULT_QUERY_COUNT;
    std::map<std::string, Query> queries;
    std::map<std::string, QueryGroup> queryGroups;
};