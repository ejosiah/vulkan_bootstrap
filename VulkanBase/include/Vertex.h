#pragma once

#include <vector>
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <array>
#include <vector>

struct ClipSpace{

    struct Quad{
        static constexpr  std::array<glm::vec2, 8> positions{
                glm::vec2{-1, -1},glm::vec2{0, 0},
                glm::vec2{-1, 1}, glm::vec2{0, 1},
                glm::vec2{1, -1}, glm::vec2{1, 0},
                glm::vec2{1, 1}, glm::vec2{1, 1}
        };
        static constexpr VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;

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

struct Ndc{
    static constexpr std::array<glm::vec4, 8> points{
            glm::vec4{-1, 1, 0, 1}, glm::vec4{1, 1, 0, 1},
            glm::vec4{-1, -1, 0, 1}, glm::vec4{1, -1, 0, 1},
            glm::vec4{-1, 1, 1, 1}, glm::vec4{1, 1, 1, 1},
            glm::vec4{-1, -1, 1, 1}, glm::vec4{1, -1, 1, 1}
    };

    static constexpr std::array<glm::uint32_t, 24> indices{
            0, 1, 2, 3, 0, 2, 1, 3,
            4, 5, 6, 7, 4, 6, 5, 7,
            0, 4, 1, 5, 2, 6, 3, 7
    };
};

struct Vertex{
    glm::vec4 position;
    glm::vec4 color;
    alignas(16) glm::vec3 normal;
    alignas(16) glm::vec3 tangent;
    alignas(16) glm::vec3 bitangent;
    glm::vec2 uv;

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