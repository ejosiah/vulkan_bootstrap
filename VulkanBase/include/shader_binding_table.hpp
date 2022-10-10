#pragma once

#include "common.h"
#include <string>
#include <map>
#include "VulkanDevice.h"
#include "VulkanBuffer.h"
#include "glm_format.h"
#include "spdlog/spdlog.h"

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
    uint32_t size() const {
        return record.size();
    }

};

struct ShaderGroup {

    uint32_t groupOffset{};
    uint32_t next{};
    std::map<uint32_t, ShaderRecord> entry{};
    uint32_t handleSize{0};
    uint32_t handleSizeAligned{0};
    uint32_t shaderGroupBaseAlignment{0};

    explicit ShaderGroup(uint32_t offset):groupOffset{offset}
    {}

    [[nodiscard]]
    uint32_t handleCount() const {
        return entry.size();
    }

    void addEntry(){
        const auto index = groupOffset + next++;
        entry.insert(std::make_pair(index, ShaderRecord{}));
    }

    void set(uint32_t pHandleSize, uint32_t pHandleSizeAligned, uint32_t pShaderGroupBaseAlignment){
        handleSize = pHandleSize;
        handleSizeAligned = pHandleSizeAligned;
        shaderGroupBaseAlignment = pShaderGroupBaseAlignment;
    }

    template<typename T>
    void addRecord(uint32_t index, const T& record){
        const auto key = groupOffset + index;
        assert(entry.find(key) != entry.end());
        entry[key].addData(record);
    }

    [[nodiscard]]
    uint32_t alignedSize() const {
        auto size = 0u;
        for(auto& [_, record] : entry){
            size += record.size();
        }
        return ::alignedSize(handleSize * handleCount() + size, handleSizeAligned);
    }

    [[nodiscard]]
    uint32_t size() const {
        auto size = 0u;
        for(auto& [_, record] : entry){
            size += record.size();
        }
        return handleSize * handleCount() + size;
    }

    [[nodiscard]]
    uint32_t totalHandleSize() const {
        return handleSizeAligned * handleCount();
    }

    void clear(){
        entry.clear();
    }

    [[nodiscard]]
    uint32_t stride() const {
        auto stride = ::alignedSize(handleSizeAligned, shaderGroupBaseAlignment);
        for(const auto& [_, record] : entry){
            auto rStride = ::alignedSize(handleSizeAligned + record.size(), shaderGroupBaseAlignment);
            stride = glm::max(stride, rStride);
        }
        return stride;
    }

    void transferRecords(uint8_t* dest, uint32_t stride){
        auto offset = 0u;
        for(auto& [_, record] : entry){
            offset += handleSizeAligned;
            const auto src = record.record.data();
            const auto size = record.record.size();
            std::memcpy(dest + offset, src, size);
            offset +=  stride - (offset + record.size());
        }
    }
};

enum class GroupType { RAY_GEN, MISS, CLOSEST_HIT, CALLABLE, NONE };

struct ShaderGroups{
    
    GroupType type{NONE};
    std::vector<ShaderGroup> groups{};
    
    void add(uint32_t groupOffset, uint32_t handleCount = 1){
        ShaderGroup group = ShaderGroup{ groupOffset };
        for(auto i = 0; i < handleCount; i++) group.addEntry();
        groups.push_back(group);
    }

    void set(uint32_t handleSize, uint32_t handleSizeAligned, uint32_t shaderGroupBaseAlignment){
        for(auto& group : groups){
            group.set(handleSize, handleSizeAligned, shaderGroupBaseAlignment);
        }
    }

    void transferRecords(uint8_t* dest, uint32_t stride){
        for(auto& group : groups){
            group.transferRecords(dest, stride);
            dest += stride * group.handleCount();
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
        assert(groups.size() > index);
        return groups[index];
    }
    
    [[nodiscard]]
    uint32_t sizeAligned() const {
        auto size = 0u;
        for(auto& group : groups){
            size += group.alignedSize();
        }
        return size;
    }

    [[nodiscard]]
    uint32_t totalHandleSize() const {
        auto size = 0u;
        for(auto& group : groups){
            size += group.totalHandleSize();
        }
        return size;
    }

    [[nodiscard]]
    uint32_t handleCount() const {
        auto count = 0u;
        for(auto& group : groups){
            count += group.handleCount();
        }
        return count;
    }

    [[nodiscard]]
    uint32_t groupOffset() const{
        assert(!groups.empty());
        return groups[0].groupOffset;
    }

    [[nodiscard]]
    VkStridedDeviceAddressRegionKHR getStridedDeviceAddressRegionKHR() const{
        VkStridedDeviceAddressRegionKHR stridedDeviceAddressRegionKHR{};
        stridedDeviceAddressRegionKHR.stride = stride();
        stridedDeviceAddressRegionKHR.size = stride() * handleCount();

        return stridedDeviceAddressRegionKHR;
    }

    [[nodiscard]]
    uint32_t groupCount() const {
        return groups.size();
    }

    void clear(){
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

    VkRayTracingShaderGroupCreateInfoKHR rayGenGroup(){
        rayGen.add(totalHandles);

        VkRayTracingShaderGroupCreateInfoKHR shaderGroupInfo{};
        shaderGroupInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
        shaderGroupInfo.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
        shaderGroupInfo.generalShader = totalHandles++;
        shaderGroupInfo.closestHitShader = VK_SHADER_UNUSED_KHR;
        shaderGroupInfo.anyHitShader = VK_SHADER_UNUSED_KHR;
        shaderGroupInfo.intersectionShader = VK_SHADER_UNUSED_KHR;

        return shaderGroupInfo;
    }

    VkRayTracingShaderGroupCreateInfoKHR addMissGroup(){
        missGroups.add(totalHandles);

        VkRayTracingShaderGroupCreateInfoKHR shaderGroupInfo{};
        shaderGroupInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
        shaderGroupInfo.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
        shaderGroupInfo.generalShader = totalHandles++;
        shaderGroupInfo.closestHitShader = VK_SHADER_UNUSED_KHR;
        shaderGroupInfo.anyHitShader = VK_SHADER_UNUSED_KHR;
        shaderGroupInfo.intersectionShader = VK_SHADER_UNUSED_KHR;

        return shaderGroupInfo;
    }

    VkRayTracingShaderGroupCreateInfoKHR addHitGroup(VkRayTracingShaderGroupTypeKHR type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR,
                                                     bool closestHitShader = true, bool anyHitShader = false, bool intersectionShader = false){
        auto prevNumGroups = totalHandles;
        VkRayTracingShaderGroupCreateInfoKHR shaderGroupInfo{};
        shaderGroupInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
        shaderGroupInfo.type = type;
        shaderGroupInfo.generalShader = VK_SHADER_UNUSED_KHR;
        shaderGroupInfo.closestHitShader = closestHitShader ? totalHandles++ : VK_SHADER_UNUSED_KHR;
        shaderGroupInfo.anyHitShader = anyHitShader ? totalHandles++ : VK_SHADER_UNUSED_KHR;
        shaderGroupInfo.intersectionShader = intersectionShader ? totalHandles++ : VK_SHADER_UNUSED_KHR;

        auto handleCount = totalHandles - prevNumGroups;
        hitGroups.add(prevNumGroups, handleCount);
        return shaderGroupInfo;
    }

    VkRayTracingShaderGroupCreateInfoKHR addCallableGroup(){
        callableGroups.add(totalHandles);
        VkRayTracingShaderGroupCreateInfoKHR shaderGroupInfo{};
        shaderGroupInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
        shaderGroupInfo.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
        shaderGroupInfo.generalShader = totalHandles++;
        shaderGroupInfo.closestHitShader = VK_SHADER_UNUSED_KHR;
        shaderGroupInfo.anyHitShader = VK_SHADER_UNUSED_KHR;
        shaderGroupInfo.intersectionShader = VK_SHADER_UNUSED_KHR;

        return shaderGroupInfo;
    }

    ShaderBindingTables compile(const VulkanDevice& device, const VulkanPipeline& pipeline){
        initHandleSizeInfo(device);
        uint32_t sbtSize = totalHandles * handleSizeAligned;
        spdlog::info("handleSize: {}, handleSizeAligned {}, sbtSize: {}", handleSize, handleSizeAligned, sbtSize);

        std::vector<uint8_t> shaderHandleStorage(sbtSize);

        auto groupCount = rayGen.groupCount() + missGroups.groupCount() + hitGroups.groupCount() + callableGroups.groupCount();
        vkGetRayTracingShaderGroupHandlesKHR(device, pipeline, 0, groupCount, sbtSize, shaderHandleStorage.data());

        const VkBufferUsageFlags usageFlags = VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;


        auto rayGenPtr = shaderHandleStorage.data();
        auto missPtr = rayGenPtr + rayGen.totalHandleSize();
        auto hitPtr = missPtr + missGroups.totalHandleSize();
        auto callablePtr = hitPtr + hitGroups.totalHandleSize();


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

        rayGen.set(handleSize, handleSizeAligned, shaderGroupBaseAlignment);
        missGroups.set(handleSize, handleSizeAligned, shaderGroupBaseAlignment);
        hitGroups.set(handleSize, handleSizeAligned, shaderGroupBaseAlignment);
        callableGroups.set(handleSize, handleSizeAligned, shaderGroupBaseAlignment);
    }

    void createShaderBindingTable(const VulkanDevice& device, ShaderBindingTable& shaderBindingTable, void* shaderHandleStoragePtr, VkBufferUsageFlags usageFlags, VmaMemoryUsage memoryUsage, ShaderGroups& groups) const {
        if(groups.handleCount() == 0) return;

        const auto stride = groups.stride();
        const auto numHandles = groups.handleCount();
        VkDeviceSize size =  stride * numHandles;
        auto stagingBuffer = device.createStagingBuffer(size);

        auto buffer = reinterpret_cast<uint8_t*>(stagingBuffer.map());
        for(int i = 0; i < numHandles; i++){
            std::memcpy(buffer + i * stride, shaderHandleStoragePtr, handleSizeAligned);
        }
        groups.transferRecords(buffer, stride);
        stagingBuffer.unmap();

        shaderBindingTable.buffer = device.createBuffer(usageFlags | VK_BUFFER_USAGE_TRANSFER_DST_BIT, memoryUsage, size);
        device.copy(stagingBuffer, shaderBindingTable.buffer, size, 0, 0);

        shaderBindingTable.stridedDeviceAddressRegion = getSbtEntryStrideDeviceAddressRegion(device, shaderBindingTable.buffer, groups);
    }

     [[nodiscard]]
     VkStridedDeviceAddressRegionKHR getSbtEntryStrideDeviceAddressRegion(const VulkanDevice& device, const VulkanBuffer& buffer, const ShaderGroups& groups) const{
        VkStridedDeviceAddressRegionKHR strideDeviceAddressRegion = groups.getStridedDeviceAddressRegionKHR();
        strideDeviceAddressRegion.deviceAddress = device.getAddress(buffer);

        return strideDeviceAddressRegion;
    }


    void clear(){
        totalHandles = 0;
        rayGen.clear();
        missGroups.clear();
        hitGroups.clear();
    }

private:
    uint32_t handleSize{};
    uint32_t handleSizeAligned{};
    uint32_t shaderGroupHandleAlignment{};
    uint32_t shaderGroupBaseAlignment{};
    uint32_t totalHandles = 0;
};