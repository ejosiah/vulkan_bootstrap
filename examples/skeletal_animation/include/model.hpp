#pragma once

#include "common.h"
#include "Mesh.h"
#include "VulkanModel.h"
#include "VulkanDevice.h"
#include <assimp/postprocess.h>

namespace mdl {
    static constexpr int NUN_BONES_PER_VERTEX = 5;
    constexpr uint32_t DEFAULT_PROCESS_FLAGS = aiProcess_GenSmoothNormals | aiProcess_Triangulate | aiProcess_CalcTangentSpace | aiProcess_JoinIdenticalVertices | aiProcess_ValidateDataStructure;
    constexpr int NULL_BONE = -1;

    struct VertexBoneInfo {
        std::array<uint32_t, NUN_BONES_PER_VERTEX> boneIds;
        std::array<float, NUN_BONES_PER_VERTEX> weights;
    };


    struct Mesh {
        std::string name{};
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        std::vector<VertexBoneInfo> bones;

        VkPrimitiveTopology primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        struct {
            glm::vec3 min = glm::vec3(MAX_FLOAT);
            glm::vec3 max = glm::vec3(MIN_FLOAT);
        } bounds;
    };

    struct Bone{
        int id{NULL_BONE};
        std::string name;
        glm::mat4 offsetMatrix{1};
        int parent{NULL_BONE};
        std::vector<int> children;
        glm::mat4 transform{1};
    };

    struct Model {
        std::vector<vkn::Primitive> primitives;
        std::vector<Bone> bones;
        int rootBone;
        std::unordered_map<std::string, int> bonesMapping;
        glm::mat4 globalInverseTransform{1};
        std::vector<std::tuple<glm::vec3, glm::vec3>> boneBounds;

        struct {
            VulkanBuffer vertices;
            VulkanBuffer indices;
            VulkanBuffer vertexBones;
            VulkanBuffer boneTransforms;
        } buffers;

        struct {
            glm::vec3 min;
            glm::vec3 max;
        } bounds;

        [[nodiscard]]
        float height() const;

        [[nodiscard]]
        glm::vec3 diagonal() const;

        [[nodiscard]]
        uint32_t numTriangles() const;

        void render(VkCommandBuffer commandBuffer) const;

        void createBuffers(const VulkanDevice& device, const std::vector<Mesh>& meshes);

    };

    std::shared_ptr<Model> load(const VulkanDevice& device, const std::string &path, uint32_t flags = mesh::DEFAULT_PROCESS_FLAGS);
}