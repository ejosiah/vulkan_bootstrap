#pragma once

#include <vector>
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <array>
#include <vector>

struct ClipSpace{

    struct Quad{
        static constexpr  std::array<glm::vec2, 8> positions{
                glm::vec2{-1, 1},glm::vec2{0, 1},
                glm::vec2{1, 1}, glm::vec2{1, 1},
                glm::vec2{1, -1}, glm::vec2{1, 0},
                glm::vec2{-1, -1}, glm::vec2{0, 0}
        };
        static constexpr VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;

        static constexpr VkFrontFace  frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    };

    struct Triangle{
        static constexpr  std::array<glm::vec2, 6> positions{
                glm::vec2{-1, 1},glm::vec2{0, 1},
                glm::vec2{1, 1}, glm::vec2{1, 1},
                glm::vec2{0, -1}, glm::vec2{0.5, 0.5},
        };
        static constexpr VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        static constexpr VkFrontFace  frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    };


    static std::vector<VkVertexInputBindingDescription> bindingDescription(){
        return {
                {0, 2 * sizeof(glm::vec2), VK_VERTEX_INPUT_RATE_VERTEX}
        };
    }

    static std::vector<VkVertexInputAttributeDescription> attributeDescriptions(){
        return {
                {0, 0, VK_FORMAT_R32G32_SFLOAT, 0},
                {1, 0, VK_FORMAT_R32G32_SFLOAT, sizeof(glm::vec2)}
        };
    }
};

struct Vertex{
    glm::vec4 position;
    glm::vec3 normal;
    glm::vec4 color;
    glm::vec2 uv; // TODO std::array<std::vector<glm::vec2>, MAX_UVS>>;
    glm::vec3 tangent;
    glm::vec3 bitangent;

    static std::vector<VkVertexInputBindingDescription> bindingDisc(){
        return {
                {0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX}
        };
    }

    static std::vector<VkVertexInputAttributeDescription> attributeDisc(){
        return {
                {0, 0, VK_FORMAT_R32G32B32A32_SFLOAT, (uint32_t)offsetof(Vertex, position)},
                {1, 0, VK_FORMAT_R32G32B32_SFLOAT, (uint32_t)offsetof(Vertex, normal)},
                {2, 0, VK_FORMAT_R32G32B32_SFLOAT, (uint32_t)offsetof(Vertex, tangent)},
                {3, 0, VK_FORMAT_R32G32B32_SFLOAT, (uint32_t)offsetof(Vertex, bitangent)},
                {4, 0, VK_FORMAT_R32G32B32A32_SFLOAT, (uint32_t)offsetof(Vertex, color)},
                {5, 0, VK_FORMAT_R32G32_SFLOAT, (uint32_t)offsetof(Vertex, uv)}
        };
    }
};


struct Vertices{
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    VkPrimitiveTopology topology;
};