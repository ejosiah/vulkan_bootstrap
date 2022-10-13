#pragma once

#include "common.h"
#include <string>
#include <map>
#include "VulkanDevice.h"
#include "VulkanBuffer.h"
#include "glm_format.h"
#include "spdlog/spdlog.h"
#include <algorithm>

struct ShaderRecord{
    std::vector<unsigned char> record{};

    template<typename T>
    void addData(const T& data){
        const auto src = reinterpret_cast<const unsigned char*>(&data);
        for(int i = 0; i < sizeof(T); i++){
            const auto byte = src[i];
            record.push_back(byte);
        }
    }

    [[nodiscard]]
    inline uint32_t size() const {
        return record.size();
    }

    [[nodiscard]]
    inline auto data() const {
        return record.data();
    }

    inline void clear() {
        record.clear();
    }

};

struct ShaderGroup {

    uint32_t next{};
    ShaderRecord record;
    uint32_t handleSizeAligned{0};
    uint32_t shaderGroupBaseAlignment{0};

    void set(uint32_t pHandleSizeAligned, uint32_t pShaderGroupBaseAlignment){
        handleSizeAligned = pHandleSizeAligned;
        shaderGroupBaseAlignment = pShaderGroupBaseAlignment;
    }

    template<typename T>
    inline void addRecord(const T& data){
        record.addData(data);
    }

    [[nodiscard]]
    inline uint32_t size() const {
        return handleSizeAligned + record.size();
    }

    inline void clear(){
        record.clear();
    }

    [[nodiscard]]
    inline uint32_t stride() const {
        return ::alignedSize(size(), shaderGroupBaseAlignment);
    }

    inline void transferRecords(uint8_t* dest) const {
        std::memcpy(dest + handleSizeAligned, record.data(), record.size());
    }
};

enum class GroupType { RAY_GEN, MISS, CLOSEST_HIT, CALLABLE, NONE };

inline std::string groupToString(GroupType type){
    switch (type) {
        case GroupType::RAY_GEN: return "ray generation group";
        case GroupType::MISS: return "miss group";
        case GroupType::CLOSEST_HIT: return "hit group";
        case GroupType::CALLABLE: return "callable group";
        default: throw std::runtime_error{"unsupported type"};
    }
}

struct ShaderGroups{
    
    GroupType type{NONE};
    std::vector<ShaderGroup> groups{};
    
    void add(){
        groups.emplace_back();
    }

    void set(uint32_t handleSizeAligned, uint32_t shaderGroupBaseAlignment){
        for(auto& group : groups){
            group.set(handleSizeAligned, shaderGroupBaseAlignment);
        }
    }

    void transferRecords(uint8_t* dest, uint32_t stride){
        for(auto& group : groups){
            group.transferRecords(dest);
            dest += stride;
        }
    }
    
    [[nodiscard]]
    uint32_t stride() const {
        auto stride = 0u;
        for(const auto& group : groups){
            stride = glm::max(stride, group.stride());
        }
        return stride;
    }
    
    ShaderGroup& get(const uint32_t index){
        assert(count() > index);
        return groups[index];
    }

    [[nodiscard]]
    const ShaderGroup& get(const uint32_t index) const {
        assert(count() > index);
        return groups[index];
    }

    ShaderGroup& operator[](const int index){
        return groups[index];
    }

    const ShaderGroup& operator[](const int index) const {
        return groups[index];
    }

    [[nodiscard]]
    VkStridedDeviceAddressRegionKHR getStridedDeviceAddressRegionKHR(const VkDeviceAddress& deviceAddress) const{
        VkStridedDeviceAddressRegionKHR region{};
        region.deviceAddress = deviceAddress;
        region.stride = stride();
        region.size = stride() * count();

        return region;
    }

    [[nodiscard]]
    inline uint32_t count() const {
        return groups.size();
    }
    
    inline auto begin(){
        return groups.begin();
    }
    
    inline auto end() {
        return groups.end();
    }

    inline void clear(){
        groups.clear();
    }
};

struct ShaderBindingTable{
    VulkanBuffer buffer;
    VkStridedDeviceAddressRegionKHR stridedDeviceAddressRegion{};

    operator VkStridedDeviceAddressRegionKHR*()  {
        return &stridedDeviceAddressRegion;
    }
};

struct ShaderBindingTables {
    ShaderBindingTable rayGen;
    ShaderBindingTable miss;
    ShaderBindingTable closestHit;
    ShaderBindingTable callable;
};

struct ShaderTablesDescription{
    ShaderGroups rayGen{GroupType::RAY_GEN};
    ShaderGroups missGroups{ GroupType::MISS};
    ShaderGroups hitGroups{ GroupType::CLOSEST_HIT};
    ShaderGroups callableGroups{ GroupType::CALLABLE };

    VkRayTracingShaderGroupCreateInfoKHR rayGenGroup(uint32_t shaderIndex = 0){
        numGroups++;
        rayGen.add();

        VkRayTracingShaderGroupCreateInfoKHR shaderGroupInfo{};
        shaderGroupInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
        shaderGroupInfo.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
        shaderGroupInfo.generalShader = shaderIndex;
        shaderGroupInfo.closestHitShader = VK_SHADER_UNUSED_KHR;
        shaderGroupInfo.anyHitShader = VK_SHADER_UNUSED_KHR;
        shaderGroupInfo.intersectionShader = VK_SHADER_UNUSED_KHR;

        return shaderGroupInfo;
    }

    VkRayTracingShaderGroupCreateInfoKHR addMissGroup(uint32_t shaderIndex){
        numGroups++;
        missGroups.add();

        VkRayTracingShaderGroupCreateInfoKHR shaderGroupInfo{};
        shaderGroupInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
        shaderGroupInfo.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
        shaderGroupInfo.generalShader = shaderIndex;
        shaderGroupInfo.closestHitShader = VK_SHADER_UNUSED_KHR;
        shaderGroupInfo.anyHitShader = VK_SHADER_UNUSED_KHR;
        shaderGroupInfo.intersectionShader = VK_SHADER_UNUSED_KHR;

        return shaderGroupInfo;
    }

    VkRayTracingShaderGroupCreateInfoKHR addHitGroup( uint32_t closestHitShader, uint32_t intersectionShader = VK_SHADER_UNUSED_KHR,
                                                      uint32_t anyHitShader = VK_SHADER_UNUSED_KHR,
                                                      VkRayTracingShaderGroupTypeKHR type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR){
        numGroups++;
        VkRayTracingShaderGroupCreateInfoKHR shaderGroupInfo{};
        shaderGroupInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
        shaderGroupInfo.type = type;
        shaderGroupInfo.generalShader = VK_SHADER_UNUSED_KHR;
        shaderGroupInfo.closestHitShader = closestHitShader;
        shaderGroupInfo.anyHitShader = anyHitShader;
        shaderGroupInfo.intersectionShader = intersectionShader;

        hitGroups.add();
        return shaderGroupInfo;
    }

    VkRayTracingShaderGroupCreateInfoKHR addCallableGroup(uint32_t shaderIndex){
        numGroups++;
        callableGroups.add();
        VkRayTracingShaderGroupCreateInfoKHR shaderGroupInfo{};
        shaderGroupInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
        shaderGroupInfo.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
        shaderGroupInfo.generalShader = shaderIndex;
        shaderGroupInfo.closestHitShader = VK_SHADER_UNUSED_KHR;
        shaderGroupInfo.anyHitShader = VK_SHADER_UNUSED_KHR;
        shaderGroupInfo.intersectionShader = VK_SHADER_UNUSED_KHR;

        return shaderGroupInfo;
    }

    void addGroups(const VkRayTracingPipelineCreateInfoKHR& createInfo){
        assert(createInfo.groupCount > 0);

        for(int i = 1; i < createInfo.groupCount; i++){
            const auto group = createInfo.pGroups[i];
            if(group.type == VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR){
                if(createInfo.pStages[group.generalShader].stage == VK_SHADER_STAGE_RAYGEN_BIT_KHR){
                    rayGenGroup(group.generalShader);
                }else if(createInfo.pStages[group.generalShader].stage == VK_SHADER_STAGE_MISS_BIT_KHR){
                    addMissGroup(group.generalShader);
                }else if(createInfo.pStages[group.generalShader].stage == VK_SHADER_STAGE_CALLABLE_BIT_KHR){
                    addCallableGroup(group.generalShader);
                }
            }else{
                addHitGroup(group.closestHitShader, group.intersectionShader, group.anyHitShader, group.type);
            }
        }
    }

    ShaderBindingTables compile(const VulkanDevice& device, const VulkanPipeline& pipeline){
        initHandleSizeInfo(device);
        uint32_t sbtSize = numGroups * handleSizeAligned;
        spdlog::info("handleSize: {}, handleSizeAligned {}, sbtSize: {}", handleSize, handleSizeAligned, sbtSize);

        std::vector<uint8_t> shaderHandleStorage(sbtSize);

        auto groupCount = numGroups;
        vkGetRayTracingShaderGroupHandlesKHR(device, pipeline, 0, groupCount, sbtSize, shaderHandleStorage.data());

        const VkBufferUsageFlags usageFlags = VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;


        auto rayGenPtr = shaderHandleStorage.data();
        auto missPtr = rayGenPtr + rayGen.count() * handleSizeAligned;
        auto hitPtr = missPtr + missGroups.count() * handleSizeAligned;
        auto callablePtr = hitPtr + hitGroups.count() * handleSizeAligned;


        ShaderBindingTables sbt;
        createShaderBindingTable(device, sbt.rayGen, rayGenPtr, usageFlags, VMA_MEMORY_USAGE_GPU_ONLY, rayGen);
        createShaderBindingTable(device, sbt.miss, missPtr, usageFlags, VMA_MEMORY_USAGE_GPU_ONLY, missGroups);
        createShaderBindingTable(device, sbt.closestHit, hitPtr, usageFlags, VMA_MEMORY_USAGE_GPU_ONLY, hitGroups);
        createShaderBindingTable(device, sbt.callable, callablePtr, usageFlags, VMA_MEMORY_USAGE_GPU_ONLY, callableGroups);

        clear();

        return sbt;
    }

    void initHandleSizeInfo(const VulkanDevice& device){
        VkPhysicalDeviceRayTracingPipelinePropertiesKHR rayTraceProps{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR};
        VkPhysicalDeviceProperties2 props{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
        props.pNext = &rayTraceProps;
        vkGetPhysicalDeviceProperties2(device, &props);
        handleSize = rayTraceProps.shaderGroupHandleSize;
        shaderGroupHandleAlignment = rayTraceProps.shaderGroupHandleAlignment;
        shaderGroupBaseAlignment = rayTraceProps.shaderGroupBaseAlignment;
        handleSizeAligned = alignedSize(handleSize, shaderGroupHandleAlignment);

        rayGen.set(handleSizeAligned, shaderGroupBaseAlignment);
        missGroups.set(handleSizeAligned, shaderGroupBaseAlignment);
        hitGroups.set(handleSizeAligned, shaderGroupBaseAlignment);
        callableGroups.set(handleSizeAligned, shaderGroupBaseAlignment);
    }

    void createShaderBindingTable(const VulkanDevice& device, ShaderBindingTable& shaderBindingTable, uint8_t* shaderHandleStoragePtr, VkBufferUsageFlags usageFlags, VmaMemoryUsage memoryUsage, ShaderGroups& groups) const {
        if(groups.count() == 0) return;

        const auto stride = groups.stride();
        const auto groupCount = groups.count();
        VkDeviceSize size =  stride * groupCount;
        auto stagingBuffer = device.createStagingBuffer(size);

        auto buffer = reinterpret_cast<uint8_t*>(stagingBuffer.map());
        auto dest = buffer;
        auto src = shaderHandleStoragePtr;

        for(auto& group : groups){
            std::memcpy(dest, src, handleSizeAligned);
            src += handleSizeAligned;
            dest += stride;
        }

        groups.transferRecords(buffer, stride);
        stagingBuffer.unmap();
        spdlog::info("{} size: {}", groupToString(groups.type), stagingBuffer.size);

        shaderBindingTable.buffer = device.createBuffer(usageFlags | VK_BUFFER_USAGE_TRANSFER_DST_BIT, memoryUsage, size);
        device.copy(stagingBuffer, shaderBindingTable.buffer, size, 0, 0);

        shaderBindingTable.stridedDeviceAddressRegion =
                groups.getStridedDeviceAddressRegionKHR(device.getAddress(shaderBindingTable.buffer));
    }


    void clear(){
        numGroups = 0;
        rayGen.clear();
        missGroups.clear();
        hitGroups.clear();
    }

private:
    uint32_t handleSize{};
    uint32_t handleSizeAligned{};
    uint32_t shaderGroupHandleAlignment{};
    uint32_t shaderGroupBaseAlignment{};
    uint32_t numGroups = 0;
};