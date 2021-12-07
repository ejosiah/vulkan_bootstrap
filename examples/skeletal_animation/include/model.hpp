#pragma once

#include "common.h"
#include "Mesh.h"
#include "VulkanModel.h"
#include <assimp/postprocess.h>

namespace model {
    static constexpr int NUN_BONES_PER_VERTEX = 4;
    constexpr uint32_t DEFAULT_PROCESS_FLAGS = aiProcess_GenSmoothNormals | aiProcess_Triangulate | aiProcess_CalcTangentSpace | aiProcess_JoinIdenticalVertices | aiProcess_ValidateDataStructure;


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
        uint32_t id;
        std::string name;
        glm::mat4 offsetMatrix{1};
        glm::mat4 finalTransform{1};
    };

    struct Model {
        std::vector<vkn::Primitive> primitives;
        std::vector<Bone> bones;
        std::unordered_map<std::string, uint32_t> bonesMapping;
        glm::mat4 globalInverseTransform{1};

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
        uint32_t numTriangles() const;

        void render(VkCommandBuffer commandBuffer) const;

    };

    std::shared_ptr<Model> load(const std::string &path, uint32_t flags = mesh::DEFAULT_PROCESS_FLAGS);
}