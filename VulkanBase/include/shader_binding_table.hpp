#pragma once

#include "common.h"
#include <string>
#include <map>
#include "VulkanDevice.h"
#include "VulkanBuffer.h"

struct ShaderRecord{
    std::string name;
    uint32_t handleCount{0};
    bool isRayGen{false};
    std::vector<unsigned char> record{};
    uint32_t handleSize{};
    uint32_t handleSizeAligned{};

    template<typename T>
    void addData(const T& data){
        const auto src = reinterpret_cast<const unsigned char*>(&data);
        for(int i = 0; i < sizeof(T); i++){
            const auto byte = src[i];
            record.push_back(byte);
        }
    }

    [[nodiscard]]
    uint32_t sizeAligned() const {
        return handleSizeAligned * handleCount + record.size();
    }

    [[nodiscard]]
    uint32_t size() const {
        return handleSize * handleCount + record.size();
    }

    [[nodiscard]]
    uint32_t stride() const {
        return alignedSize(sizeAligned(), handleSizeAligned);
    }
};

struct ShaderGroups {

    enum class Type { RAY_GEN, MISS, CLOSEST_HIT, CALLABLE, NONE };

    Type type{NONE};
    std::map<std::string, ShaderRecord> group;
    uint32_t _shaderGroupBaseAlignment{0};
    uint32_t _stride{0};

    [[nodiscard]]
    uint32_t handleCount() const {
        uint32_t count = 0;
        for(auto [_, record] : group){
            count += record.handleCount;
        }
        return count;
    }

    void addGroup(const std::string& name, uint32_t handleCount = 0){
        group[name] = ShaderRecord{ name, handleCount };
    }

    void set(uint32_t handleSize, uint32_t handleSizeAligned, uint32_t shaderGroupBaseAlignment){
        for(auto& [_, record] : group){
            record.handleSize = handleSize;
            record.handleSizeAligned = handleSizeAligned;
        }
        _shaderGroupBaseAlignment = shaderGroupBaseAlignment;
        _stride = type == Type::RAY_GEN ? shaderGroupBaseAlignment : handleSizeAligned;
    }

    template<typename T>
    void addRecord(const std::string& name, const T& record){
        group[name].addData(record);
    }

    [[nodiscard]]
    uint32_t alignedSize() const {
        return stride() * group.size();
    }

    [[nodiscard]]
    uint32_t size() const {
        auto size = 0u;
        for(auto& [_, record] : group){
            size += record.size();
        }
        return size;
    }

    void clear(){
        group.clear();
    }

    [[nodiscard]]
    uint32_t stride() const {
        auto stride = 0u;
        for(auto& [_, record] : group){
            stride = glm::max(record.stride(), stride);
        }
        return stride;
    }

    void transferRecords(unsigned char* dest){
        auto offset = 0u;
        for(auto& [_, record] : group){
            offset += record.handleSizeAligned;
            const auto src = record.record.data();
            const auto size = record.record.size();
            std::memcpy(dest + offset, src, size);
            offset += size;
            offset += record.stride() - record.size();
        }
    }

    [[nodiscard]]
    VkStridedDeviceAddressRegionKHR getStridedDeviceAddressRegionKHR() const{
        VkStridedDeviceAddressRegionKHR stridedDeviceAddressRegionKHR{};
        stridedDeviceAddressRegionKHR.stride = _stride;
        stridedDeviceAddressRegionKHR.size = ::alignedSize(alignedSize(), _shaderGroupBaseAlignment);

        return stridedDeviceAddressRegionKHR;
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
    ShaderGroups rayGen{ShaderGroups::Type::RAY_GEN};
    ShaderGroups missGroups{ShaderGroups::Type::MISS};
    ShaderGroups hitGroups{ShaderGroups::Type::CLOSEST_HIT};
    ShaderGroups callableGroups{ShaderGroups::Type::CALLABLE};

    VkRayTracingShaderGroupCreateInfoKHR rayGenGroup(){
        rayGen.addGroup("ray_generation", 1);
        VkRayTracingShaderGroupCreateInfoKHR shaderGroupInfo{};
        shaderGroupInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
        shaderGroupInfo.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
        shaderGroupInfo.generalShader = 0;
        shaderGroupInfo.closestHitShader = VK_SHADER_UNUSED_KHR;
        shaderGroupInfo.anyHitShader = VK_SHADER_UNUSED_KHR;
        shaderGroupInfo.intersectionShader = VK_SHADER_UNUSED_KHR;

        return shaderGroupInfo;
    }

    VkRayTracingShaderGroupCreateInfoKHR addMissGroup(const std::string& name){
        missGroups.addGroup(name, 1);
        VkRayTracingShaderGroupCreateInfoKHR shaderGroupInfo{};
        shaderGroupInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
        shaderGroupInfo.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
        shaderGroupInfo.generalShader = numGroups++;
        shaderGroupInfo.closestHitShader = VK_SHADER_UNUSED_KHR;
        shaderGroupInfo.anyHitShader = VK_SHADER_UNUSED_KHR;
        shaderGroupInfo.intersectionShader = VK_SHADER_UNUSED_KHR;

        return shaderGroupInfo;
    }

    VkRayTracingShaderGroupCreateInfoKHR addHitGroup(const std::string& name,
                                                     VkRayTracingShaderGroupTypeKHR type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR,
                                                     bool closestHitShader = true, bool anyHitShader = false, bool intersectionShader = false){
        auto prevNumGroups = numGroups;
        VkRayTracingShaderGroupCreateInfoKHR shaderGroupInfo{};
        shaderGroupInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
        shaderGroupInfo.type = type;
        shaderGroupInfo.generalShader = VK_SHADER_UNUSED_KHR;
        shaderGroupInfo.closestHitShader = closestHitShader ? numGroups++ : VK_SHADER_UNUSED_KHR;
        shaderGroupInfo.anyHitShader = anyHitShader ? numGroups++ : VK_SHADER_UNUSED_KHR;
        shaderGroupInfo.intersectionShader = intersectionShader ? numGroups++ : VK_SHADER_UNUSED_KHR;

        auto handleCount = numGroups - prevNumGroups;
        hitGroups.addGroup(name, handleCount);
        return shaderGroupInfo;
    }

    VkRayTracingShaderGroupCreateInfoKHR addCallableGroup(const std::string& name){
        callableGroups.addGroup(name);
        VkRayTracingShaderGroupCreateInfoKHR shaderGroupInfo{};
        shaderGroupInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
        shaderGroupInfo.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
        shaderGroupInfo.generalShader = numGroups++;
        shaderGroupInfo.closestHitShader = VK_SHADER_UNUSED_KHR;
        shaderGroupInfo.anyHitShader = VK_SHADER_UNUSED_KHR;
        shaderGroupInfo.intersectionShader = VK_SHADER_UNUSED_KHR;

        return shaderGroupInfo;
    }

    ShaderBindingTables compile(const VulkanDevice& device, const VulkanPipeline& pipeline){
        initHandleSizeInfo(device);
        uint32_t sbtSize = getSbtSize();
        spdlog::info("handleSize: {}, handleSizeAligned {}, sbtSize: {}", handleSize, handleSizeAligned, sbtSize);

        std::vector<uint8_t> shaderHandleStorage(sbtSize);

        vkGetRayTracingShaderGroupHandlesKHR(device, pipeline, 0, numGroups, sbtSize, shaderHandleStorage.data());

        const VkBufferUsageFlags usageFlags = VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;


        auto rayGenPtr = shaderHandleStorage.data();
        auto missPtr = rayGenPtr + rayGen.alignedSize();
        auto hitPtr = missPtr + missGroups.alignedSize();
        auto callablePtr = hitPtr + hitGroups.alignedSize();

        rayGen.transferRecords(rayGenPtr);
        missGroups.transferRecords(missPtr);
        hitGroups.transferRecords(hitPtr);
        callableGroups.transferRecords(callablePtr);

        ShaderBindingTables sbt;
        createShaderBindingTable(device, sbt.rayGen, rayGenPtr, usageFlags, VMA_MEMORY_USAGE_GPU_ONLY, rayGen);
        createShaderBindingTable(device, sbt.miss, missPtr, usageFlags, VMA_MEMORY_USAGE_GPU_ONLY, missGroups);
        createShaderBindingTable(device, sbt.closestHit, hitPtr, usageFlags, VMA_MEMORY_USAGE_GPU_ONLY, hitGroups);
        createShaderBindingTable(device, sbt.callable, hitPtr, usageFlags, VMA_MEMORY_USAGE_GPU_ONLY, callableGroups);

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

    [[nodiscard]]
    uint32_t getSbtSize() const {
        return
            rayGen.getStridedDeviceAddressRegionKHR().size
            + missGroups.getStridedDeviceAddressRegionKHR().size
            + hitGroups.getStridedDeviceAddressRegionKHR().size
            + callableGroups.getStridedDeviceAddressRegionKHR().size;
    }


    void createShaderBindingTable(const VulkanDevice& device, ShaderBindingTable& shaderBindingTable,  void* shaderHandleStoragePtr, VkBufferUsageFlags usageFlags, VmaMemoryUsage memoryUsage, const ShaderGroups& groups) const {
        if(groups.handleCount() == 0) return;
        VkDeviceSize size = groups.size();
        auto stagingBuffer = device.createStagingBuffer(size);
        stagingBuffer.copy(shaderHandleStoragePtr, size);

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
        numGroups = 1;
        rayGen.clear();
        missGroups.clear();
        hitGroups.clear();
    }

private:
    uint32_t handleSize{};
    uint32_t handleSizeAligned{};
    uint32_t shaderGroupHandleAlignment{};
    uint32_t shaderGroupBaseAlignment{};
    uint32_t numGroups = 1;
};