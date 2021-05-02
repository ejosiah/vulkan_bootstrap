#include "VulkanRayTraceModel.hpp"

rt::AccelerationStructureBuilder::AccelerationStructureBuilder(const VulkanDevice *device)
    :m_device{device}
{

}

void rt::AccelerationStructureBuilder::buildAs(const std::vector<VulkanDrawableInstance> &drawableInstances,
                                               VkBuildAccelerationStructureFlagsKHR flags) {
    std::set<VulkanDrawable*> set;
    for(auto& dInstance : drawableInstances){
        set.insert(dInstance.drawable);
    }
    std::vector<VulkanDrawable*> drawables(begin(set), end(set));
    buildBlas(drawables, flags);

    auto blasId = [&](VulkanDrawable* drawable) -> std::optional<uint32_t> {
        for(int i = 0; i < drawables.size(); i++){
            if(drawable == drawables[i]) return i;
        }
        return {};
    };

    std::vector<Instance> instances;
    instances.reserve(drawableInstances.size());
    for(auto i = 0; i < drawableInstances.size(); i++){
        auto dInstance = drawableInstances[i];
        auto id = blasId(dInstance.drawable);
        assert(id.has_value());

        Instance instance;
        instance.blasId = *id;
        instance.instanceCustomId = i;
        instance.hitGroupId = 0;
        instance.mask = 0xFF;
        instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
        instance.xform = dInstance.xform;
        instances.push_back(instance);
    }
    buildTlas(instances);
}

void rt::AccelerationStructureBuilder::buildBlas(const std::vector<VulkanDrawable*>& drawables, VkBuildAccelerationStructureFlagsKHR flags){
    std::vector<BlasInput> inputs;
    inputs.reserve(drawables.size());
    for(auto drawable : drawables){
        inputs.emplace_back(*m_device, *drawable);
    }
    buildBlas(inputs, flags);
}

void rt::AccelerationStructureBuilder::buildBlas(const std::vector<BlasInput> &inputs,
                                                 VkBuildAccelerationStructureFlagsKHR flags) {
    std::vector<rt::ScratchBuffer> scratchBuffers;
    scratchBuffers.reserve(inputs.size());

    std::vector<VkAccelerationStructureBuildGeometryInfoKHR> asBuildGeomInfos;
    asBuildGeomInfos.reserve(inputs.size());

    std::vector<const VkAccelerationStructureBuildRangeInfoKHR*> buildRangeInfos;
    for(auto& input : inputs){
        VkAccelerationStructureBuildGeometryInfoKHR buildInfo{};
        buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        buildInfo.flags = flags;
      //  buildInfo.geometryCount = COUNT(input.asGeomentry);
        buildInfo.geometryCount = COUNT(input.asGeomentry);
        buildInfo.pGeometries = input.asGeomentry.data();

        std::vector<uint32_t> numTriangles = input.maxPrimitiveCounts();

        VkAccelerationStructureBuildSizesInfoKHR sizeInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
        vkGetAccelerationStructureBuildSizesKHR(*m_device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &buildInfo, numTriangles.data(), &sizeInfo);

        BlasEntry entry{input};
        entry.flags = flags;
        entry.as.buffer = m_device->createBuffer(VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
                , VMA_MEMORY_USAGE_GPU_ONLY, sizeInfo.accelerationStructureSize);

        VkAccelerationStructureCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
        createInfo.buffer = entry.as.buffer;
        createInfo.size = sizeInfo.accelerationStructureSize;
        createInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        vkCreateAccelerationStructureKHR(*m_device, &createInfo, nullptr, &entry.as.handle);

        auto scratchBuffer = createScratchBuffer(sizeInfo.buildScratchSize);
        buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        buildInfo.dstAccelerationStructure = entry.as.handle;
        buildInfo.scratchData.deviceAddress = scratchBuffer.deviceAddress;

        scratchBuffers.push_back(std::move(scratchBuffer));
        m_blas.push_back(std::move(entry));
        buildRangeInfos.push_back(input.asBuildOffsetInfo.data());
        asBuildGeomInfos.push_back(buildInfo);
    }

    m_device->commandPoolFor(*m_device->queueFamilyIndex.graphics).oneTimeCommand([&](auto commandBuffer){
        vkCmdBuildAccelerationStructuresKHR(commandBuffer, COUNT(asBuildGeomInfos), asBuildGeomInfos.data(), buildRangeInfos.data());
    });

    for(auto& entry : m_blas){
        VkAccelerationStructureDeviceAddressInfoKHR asDeviceAddressInfo{};
        asDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
        asDeviceAddressInfo.accelerationStructure = entry.as.handle;
        entry.as.deviceAddress = vkGetAccelerationStructureDeviceAddressKHR(*m_device, &asDeviceAddressInfo);
    }

}

void rt::AccelerationStructureBuilder::buildTlas(const std::vector<Instance> &instances,
                                                 VkBuildAccelerationStructureFlagsKHR flags, bool update) {
    uint32_t numInstances = instances.size();
    std::vector<VkAccelerationStructureInstanceKHR> asInstances(numInstances);
    std::transform(begin(instances), end(instances), begin(asInstances), [&](auto& instance){
        return convert(instance);
    });

    m_instanceBuffer = m_device->createDeviceLocalBuffer(asInstances.data(), sizeof(VkAccelerationStructureInstanceKHR) * numInstances
                                                         , VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR);

    VkDeviceOrHostAddressConstKHR instanceDataDeviceAddress{};
    instanceDataDeviceAddress.deviceAddress = m_device->getAddress(m_instanceBuffer);

    VkAccelerationStructureGeometryKHR geometry{};
    geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    geometry.geometryType =  VK_GEOMETRY_TYPE_INSTANCES_KHR;
    geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
    geometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
    geometry.geometry.instances.arrayOfPointers = VK_FALSE;
    geometry.geometry.instances.data = instanceDataDeviceAddress;

    VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo{};
    accelerationStructureBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    accelerationStructureBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    accelerationStructureBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_BUILD_BIT_KHR;
    accelerationStructureBuildGeometryInfo.geometryCount = numInstances;
    accelerationStructureBuildGeometryInfo.pGeometries = &geometry;

    VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo{};
    accelerationStructureBuildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
    vkGetAccelerationStructureBuildSizesKHR(
            *m_device,
            VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
            &accelerationStructureBuildGeometryInfo,
            &numInstances,
            &accelerationStructureBuildSizesInfo);

    m_tlas.as.buffer = m_device->createBuffer(
            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
            VMA_MEMORY_USAGE_GPU_ONLY,
            accelerationStructureBuildSizesInfo.accelerationStructureSize
    );

    VkAccelerationStructureCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
    createInfo.buffer = m_tlas.as.buffer;
    createInfo.size = accelerationStructureBuildSizesInfo.accelerationStructureSize;
    createInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    vkCreateAccelerationStructureKHR(*m_device, &createInfo, nullptr, &m_tlas.as.handle);

    auto scratchBuffer = createScratchBuffer(accelerationStructureBuildSizesInfo.buildScratchSize);

    accelerationStructureBuildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    accelerationStructureBuildGeometryInfo.dstAccelerationStructure = m_tlas.as.handle;
    accelerationStructureBuildGeometryInfo.scratchData.deviceAddress = scratchBuffer.deviceAddress;

    VkAccelerationStructureBuildRangeInfoKHR  accelerationStructureBuildRangeInfo{};
    accelerationStructureBuildRangeInfo.primitiveCount = numInstances;
    accelerationStructureBuildRangeInfo.primitiveOffset = 0;
    accelerationStructureBuildRangeInfo.firstVertex = 0;
    accelerationStructureBuildRangeInfo.transformOffset = 0;
    std::vector<VkAccelerationStructureBuildRangeInfoKHR*> accelerationBuildStructureRangeInfos = { &accelerationStructureBuildRangeInfo };

    m_device->commandPoolFor(*m_device->queueFamilyIndex.graphics).oneTimeCommand( [&](auto commandBuffer){
        vkCmdBuildAccelerationStructuresKHR(commandBuffer, 1, &accelerationStructureBuildGeometryInfo, accelerationBuildStructureRangeInfos.data());
    });

    VkAccelerationStructureDeviceAddressInfoKHR accelerationStructureDeviceAddressInfo{};
    accelerationStructureDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
    accelerationStructureDeviceAddressInfo.accelerationStructure = m_tlas.as.handle;
    m_tlas.as.deviceAddress = vkGetAccelerationStructureDeviceAddressKHR(*m_device, &accelerationStructureDeviceAddressInfo);
}

rt::ScratchBuffer rt::AccelerationStructureBuilder::createScratchBuffer(VkDeviceSize size) const {
    rt::ScratchBuffer scratchBuffer;
    scratchBuffer.buffer = m_device->createBuffer(
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
            VMA_MEMORY_USAGE_GPU_ONLY,
            size, "");

    VkBufferDeviceAddressInfo bufferDeviceAddressInfo{};
    bufferDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    bufferDeviceAddressInfo.buffer = scratchBuffer.buffer;
    scratchBuffer.deviceAddress = vkGetBufferDeviceAddress(*m_device, &bufferDeviceAddressInfo);

    return scratchBuffer;
}
