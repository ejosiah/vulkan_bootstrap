#pragma once

#include "common.h"
#include <string>
#include <map>
#include "VulkanDevice.h"
#include "VulkanBuffer.h"

struct ShaderRecord{
    std::string name;
    uint32_t handleCount{1};
    std::vector<unsigned char> record;

    template<typename T>
    void addData(const T& data){
        const auto src = reinterpret_cast<void*>(&data);
        const auto dst = record.data() + record.size();
        std::memcpy(dst, src, sizeof(T));
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
    ShaderRecord rayGen;
    std::map<std::string, ShaderRecord> missGroups;
    std::map<std::string, ShaderRecord> hitGroups;
    std::map<std::string, ShaderRecord> callableGroups;

    VkRayTracingShaderGroupCreateInfoKHR rayGenGroup(){
        rayGen.name = "ray_generation";
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
        missGroups[name] = ShaderRecord{ name };
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
        hitGroups[name] = ShaderRecord{ name, handleCount };
        return shaderGroupInfo;
    }

    VkRayTracingShaderGroupCreateInfoKHR addCallableGroup(const std::string& name){
        callableGroups[name] = ShaderRecord{ name };
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
        uint32_t sbtSize = numGroups * handleSizeAligned;
        spdlog::info("handleSize: {}, handleSizeAligned {}, sbtSize: {}", handleSize, handleSizeAligned, sbtSize);

        std::vector<uint8_t> shaderHandleStorage(sbtSize);

        vkGetRayTracingShaderGroupHandlesKHR(device, pipeline, 0, numGroups, sbtSize, shaderHandleStorage.data());

        const VkBufferUsageFlags usageFlags = VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

        auto rayGenHandles = rayGen.handleCount;
        auto missGroupHandles = handleCount(missGroups);
        auto hitGroupHandles = handleCount(hitGroups);

        void* rayGenPtr = shaderHandleStorage.data();

        auto offset = handleSizeAligned * rayGenHandles;
        void* missPtr = shaderHandleStorage.data() + offset;

        offset = handleSizeAligned * (rayGenHandles + missGroupHandles);
        void* hitPtr = shaderHandleStorage.data() + offset;
        ShaderBindingTables sbt;

        createShaderBindingTable(device, sbt.rayGen, rayGenPtr, usageFlags, VMA_MEMORY_USAGE_GPU_ONLY, rayGenHandles, true);
        createShaderBindingTable(device, sbt.miss, missPtr, usageFlags, VMA_MEMORY_USAGE_GPU_ONLY, missGroupHandles);
        createShaderBindingTable(device, sbt.closestHit, hitPtr, usageFlags, VMA_MEMORY_USAGE_GPU_ONLY, hitGroupHandles);


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
    }

    static uint32_t handleCount(const std::map<std::string, ShaderRecord> group){
        uint32_t count = 0;
        for(auto [_, record] : group){
            count += record.handleCount;
        }
        return count;
    }

    void createShaderBindingTable(const VulkanDevice& device, ShaderBindingTable& shaderBindingTable,  void* shaderHandleStoragePtr, VkBufferUsageFlags usageFlags, VmaMemoryUsage memoryUsage, uint32_t handleCount, bool isRayGenHandle = false) const {
        if(handleCount == 0) return;
        VkDeviceSize size = handleSize * handleCount;
        auto stagingBuffer = device.createStagingBuffer(size);
        stagingBuffer.copy(shaderHandleStoragePtr, size);

        shaderBindingTable.buffer = device.createBuffer(usageFlags | VK_BUFFER_USAGE_TRANSFER_DST_BIT, memoryUsage, size);
        device.copy(stagingBuffer, shaderBindingTable.buffer, size, 0, 0);

        shaderBindingTable.stridedDeviceAddressRegion = getSbtEntryStrideDeviceAddressRegion(device, shaderBindingTable.buffer, handleCount, isRayGenHandle);
    }

     [[nodiscard]]
     VkStridedDeviceAddressRegionKHR getSbtEntryStrideDeviceAddressRegion(const VulkanDevice& device, const VulkanBuffer& buffer, uint32_t handleCount, bool isRayGenHandle) const{
        VkStridedDeviceAddressRegionKHR strideDeviceAddressRegion{};
        strideDeviceAddressRegion.deviceAddress = device.getAddress(buffer);
        strideDeviceAddressRegion.stride = isRayGenHandle ? alignedSize(handleSizeAligned, shaderGroupBaseAlignment) : handleSizeAligned;
        strideDeviceAddressRegion.size = alignedSize(handleSizeAligned * handleCount, shaderGroupBaseAlignment);

        return strideDeviceAddressRegion;
    }


    void clear(){
        numGroups = 1;
        rayGen.record.clear();
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