#pragma once

#include "VulkanDrawable.hpp"
#include "VulkanDevice.h"
#include "VulkanModel.h"
#include "VulkanExtensions.h"


namespace rt{

    struct AccelerationStructure{
        VkAccelerationStructureKHR handle = VK_NULL_HANDLE;
        VkDeviceAddress deviceAddress = 0;
        VulkanBuffer buffer;
    };

    struct Instance{
        uint32_t blasId{0};
        uint32_t instanceCustomId{0};
        uint32_t hitGroupId{0};
        uint32_t mask{0xFF};
        VkGeometryInstanceFlagsKHR flags{0};
        glm::mat4 xform{1};
    };

    struct ScratchBuffer{
        VkDeviceAddress deviceAddress = 0;
        VulkanBuffer buffer;
    };

    struct BlasInput{
        std::vector<VkAccelerationStructureGeometryKHR> asGeomentry;
        std::vector<VkAccelerationStructureBuildRangeInfoKHR> asBuildOffsetInfo;

        BlasInput(const VulkanDevice& device, const VulkanDrawable& drawable){
            VkDeviceOrHostAddressConstKHR vertexAddress{};
            VkDeviceOrHostAddressConstKHR indicesAddress{};
            vertexAddress.deviceAddress = device.getAddress(drawable.vertexBuffer);
            indicesAddress.deviceAddress = device.getAddress(drawable.indexBuffer);

            for(auto& mesh : drawable.meshes) {
                VkAccelerationStructureGeometryDataKHR gData{};
                gData.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
                gData.triangles.vertexFormat = VK_FORMAT_R32G32B32A32_SFLOAT;
                gData.triangles.vertexData = vertexAddress;
                gData.triangles.vertexStride = sizeof(Vertex);
                gData.triangles.maxVertex = mesh.maxVertex();
                gData.triangles.indexType = VK_INDEX_TYPE_UINT32;
                gData.triangles.indexData = indicesAddress;
                gData.triangles.transformData.deviceAddress = 0;
                gData.triangles.transformData.hostAddress = nullptr;

                VkAccelerationStructureGeometryKHR asGeom{};
                asGeom.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
                asGeom.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
                asGeom.flags = VK_GEOMETRY_OPAQUE_BIT_KHR; // get from drawable
                asGeom.geometry = gData;
                asGeomentry.push_back(asGeom);

                VkAccelerationStructureBuildRangeInfoKHR buildInfo{};
                buildInfo.primitiveCount = mesh.numTriangles();
                buildInfo.primitiveOffset = mesh.firstIndex * sizeof(uint32_t);
                buildInfo.firstVertex = mesh.vertexOffset;
                buildInfo.transformOffset = 0;
                asBuildOffsetInfo.push_back(buildInfo);
            }
        }

        [[nodiscard]]
        std::vector<uint32_t> maxPrimitiveCounts() const {
            std::vector<uint32_t> counts;
            for(auto& buildRangeInfo : asBuildOffsetInfo){
                counts.push_back(buildRangeInfo.primitiveCount);
            }
            return counts;
        }
    };

    struct BlasEntry{
        BlasInput input;
        AccelerationStructure as;
        VkBuildAccelerationStructureFlagsKHR flags = 0;

        BlasEntry() = default;
        BlasEntry(BlasInput input_)
            : input(std::move(input_))
        {

        }
    };

    class AccelerationStructureBuilder{
    public:
        AccelerationStructureBuilder() = default;

        explicit AccelerationStructureBuilder(const VulkanDevice* device);

        void buildAs(const std::vector<VulkanDrawableInstance>& drawableInstances,
                     VkBuildAccelerationStructureFlagsKHR flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR);

        void buildBlas(const std::vector<VulkanDrawable*>& drawables, VkBuildAccelerationStructureFlagsKHR flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR);

        void buildBlas(const std::vector<BlasInput>& input, VkBuildAccelerationStructureFlagsKHR flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR);

        void buildTlas(const std::vector<Instance>&         instances,
                       VkBuildAccelerationStructureFlagsKHR flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
                       bool                                 update = false);

        VkAccelerationStructureInstanceKHR convert(const Instance& instance){
            assert(!m_blas.empty() && instance.blasId < m_blas.size());
            VkAccelerationStructureInstanceKHR asInstance{};
            asInstance.instanceCustomIndex = instance.instanceCustomId;
            asInstance.mask = instance.mask;
            asInstance.instanceShaderBindingTableRecordOffset = 0;
            asInstance.flags = instance.flags;
            asInstance.accelerationStructureReference = m_blas[instance.blasId].as.deviceAddress;

            auto xform = glm::transpose(instance.xform);
            std::memcpy(&asInstance.transform, glm::value_ptr(xform), sizeof(VkTransformMatrixKHR));

             return asInstance;
        }

        [[nodiscard]]
        ScratchBuffer createScratchBuffer(VkDeviceSize size) const;

        [[nodiscard]]
        VkAccelerationStructureKHR* accelerationStructure() {
            return &m_tlas.as.handle;
        }

    private:
        struct Tlas{
            AccelerationStructure as;
            VkBuildAccelerationStructureFlagsKHR flags = 0;
        };

        std::vector<BlasEntry> m_blas;

        Tlas m_tlas;
        VulkanBuffer m_instanceBuffer;
        const VulkanDevice* m_device = nullptr;
    };

}