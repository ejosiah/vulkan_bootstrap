#pragma once

#include <utility>

#include "common.h"
#include "VulkanDrawable.hpp"
#include "VulkanDevice.h"
#include "VulkanModel.h"
#include "VulkanExtensions.h"
#include "implicit_shapes.hpp"

namespace rt{

    using BlasId = uint32_t;
    using Normal = glm::vec3;

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

    using InstanceBuilder = std::function<void(std::vector<Instance>&)>;

    struct ImplicitObject{
        uint32_t numObjects{0};
        uint32_t hitGroupId{0};
        uint32_t mask{0xFF};
        VulkanBuffer aabbBuffer;
    };

    struct TriangleMesh{
        struct MetaData {
            uint32_t hitGroupId{0};
            uint32_t mask{0xFF};
        };

        TriangleMesh() = default;

        TriangleMesh(VulkanDrawable* drawable, std::vector<MetaData> meta = {})
            : drawable{ drawable }
            , metaData{std::move(meta)}
        {
            if(metaData.empty()){
               metaData.resize(drawable->meshes.size());
            }
        }

        VulkanDrawable* drawable{ nullptr };
        std::vector<MetaData> metaData;
    };


    struct MeshObjectInstance{
        TriangleMesh object;
        uint32_t hitGroupId:24;
        uint32_t mask{0xFF};
        glm::mat4 xform{ 1 };
        glm::mat4 xformIT{ 1 };
    };


    struct ImplicitObjectInstance{
        uint64_t handle;
        glm::mat4 xform{ 1 };
        glm::mat4 xformIT{ 1 };
        imp::ImplicitType type = imp::ImplicitType::NONE;

    };

    struct ObjectInstance {
        glm::mat4 xform{1};
        glm::mat4 xformIT{1};
        int objId = 0;
        int objectType = 0;
        int padding1;
        int padding2;
    };

    struct InstanceGroup{

        InstanceGroup() = default;

        InstanceGroup(const MeshObjectInstance& _instance, uint32_t id)
            :sceneInstances{_instance}, objectId{id}
        {

        }

        InstanceGroup(const ImplicitObjectInstance& _instance, uint32_t id)
            :sceneInstances{_instance}, objectId{id}
        {
        }

        void add(Instance* instance){
            instances.push_back(instance);
            ObjectInstance objInst{};
            objInst.objId = objectId;
            std::visit(overloaded{
                [&](MeshObjectInstance inst){
                    objInst.xform = inst.xform;
                    objInst.xformIT = inst.xformIT;
                },
                [&](ImplicitObjectInstance inst){
                    objInst.xform = inst.xform;
                    objInst.xformIT = inst.xform;
                }
            }, sceneInstances);
            objectInstances.push_back(objInst);
        }

        uint32_t getInstanceId(const std::string& name)  {
            assert(instanceIds.find(name) != end(instanceIds));
            return instanceIds[name];
        }

        std::variant<MeshObjectInstance, ImplicitObjectInstance> sceneInstances{};
        std::vector<Instance*> instances;
        std::map<std::string, uint32_t> instanceIds;
        std::vector<ObjectInstance> objectInstances;
        uint32_t objectId = 0;
        uint32_t mask{0xFF};

    };

    struct ScratchBuffer{
        VkDeviceAddress deviceAddress = 0;
        VulkanBuffer buffer;
    };

    struct BlasInput{
        std::vector<VkAccelerationStructureGeometryKHR> asGeomentry;
        std::vector<VkAccelerationStructureBuildRangeInfoKHR> asBuildOffsetInfo;

        BlasInput(const VulkanDevice& device, const VulkanDrawable& drawable, int meshIndex = std::numeric_limits<int>::max()){
            VkDeviceOrHostAddressConstKHR vertexAddress{};
            VkDeviceOrHostAddressConstKHR indicesAddress{};
            vertexAddress.deviceAddress = device.getAddress(drawable.vertexBuffer);
            indicesAddress.deviceAddress = device.getAddress(drawable.indexBuffer);

            int size = meshIndex + 1;
            if(meshIndex == std::numeric_limits<int>::max()){
                meshIndex = 0;
                size = drawable.meshes.size();
            }

            for(auto i = meshIndex; i < size; i++) {
                auto& mesh = drawable.meshes[i];
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

        BlasInput(const VulkanDevice& device, const ImplicitObject& object){
            VkDeviceOrHostAddressConstKHR  aabbAddress{};
            aabbAddress.deviceAddress = device.getAddress(object.aabbBuffer);

            VkAccelerationStructureGeometryDataKHR gData{};
            gData.aabbs.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_AABBS_DATA_KHR;
            gData.aabbs.data = aabbAddress;
            gData.aabbs.stride = sizeof(imp::Box);

            VkAccelerationStructureGeometryKHR asGeom{};
            asGeom.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
            asGeom.geometryType = VK_GEOMETRY_TYPE_AABBS_KHR;
            asGeom.geometry = gData;
            asGeomentry.push_back(asGeom);

            VkAccelerationStructureBuildRangeInfoKHR buildInfo{};
            buildInfo.primitiveCount = object.numObjects;
            buildInfo.primitiveOffset = 0;
            buildInfo.firstVertex = 0;
            buildInfo.transformOffset = 0;
            asBuildOffsetInfo.push_back(buildInfo);
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
        DISABLE_COPY(AccelerationStructureBuilder)

        AccelerationStructureBuilder() = default;

        explicit AccelerationStructureBuilder(const VulkanDevice* device);

        AccelerationStructureBuilder(AccelerationStructureBuilder&& source) noexcept;

        ~AccelerationStructureBuilder();

        AccelerationStructureBuilder& operator=(AccelerationStructureBuilder&& source) noexcept;

        std::vector<InstanceGroup> add(const std::vector<MeshObjectInstance>& drawableInstances, VkBuildAccelerationStructureFlagsKHR flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR);

        ImplicitObject add(const std::vector<imp::Sphere>& spheres,  uint32_t customInstanceId = static_cast<uint32_t>(imp::ImplicitType::SPHERE), uint32_t hitGroup = 0, uint32_t cullMask = 0xFF, VkBuildAccelerationStructureFlagsKHR flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR);

        ImplicitObject add(const std::vector<imp::Cylinder>& cyliners, uint32_t customInstanceId = static_cast<uint32_t>(imp::ImplicitType::CYLINDER), uint32_t hitGroup = 0,  uint32_t cullMask = 0xFF, VkBuildAccelerationStructureFlagsKHR flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR);

        ImplicitObject add(const std::vector<imp::Plane>& planes, uint32_t customInstanceId = static_cast<uint32_t>(imp::ImplicitType::PLANE), float length = 1000, uint32_t hitGroup = 0, uint32_t cullMask = 0xFF, VkBuildAccelerationStructureFlagsKHR flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR);

        ImplicitObject add(const std::vector<imp::Box>& aabbs, uint32_t customInstanceId = static_cast<uint32_t>(imp::ImplicitType::BOX), uint32_t hitGroup = 0, uint32_t cullMask = 0xFF, VkBuildAccelerationStructureFlagsKHR flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR);

        std::tuple<std::vector<uint32_t>, std::vector<rt::BlasId>> buildBlas(const std::vector<VulkanDrawable*>& drawables, VkBuildAccelerationStructureFlagsKHR flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR);

        std::vector<rt::BlasId> buildBlas(const std::vector<ImplicitObject*>& implicits, VkBuildAccelerationStructureFlagsKHR flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR);

        std::vector<BlasId> buildBlas(const std::vector<BlasInput>& input, VkBuildAccelerationStructureFlagsKHR flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR);

        std::vector<Instance> updateTlas(const std::vector<Instance>& instances);

        std::vector<Instance> buildTlas(VkBuildAccelerationStructureFlagsKHR flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR, const std::vector<Instance>& instances = {});

        void addOrUpdateInstances(InstanceBuilder&& builder);

        VkAccelerationStructureInstanceKHR toVkAccStruct(const Instance& instance);

        void add(Instance instance);

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

        std::vector<Instance> m_instances;
        std::vector<BlasEntry> m_blas;

        Tlas m_tlas;
        VulkanBuffer m_instanceBuffer;
        const VulkanDevice* m_device = nullptr;
    };

}