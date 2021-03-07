#pragma once

#include <vector>
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

struct Vertex{
    glm::vec4 position;
    glm::vec3 normal;
    glm::vec3 color;
    glm::vec2 uv;

    static VkVertexInputBindingDescription bindingDisc(){
        return {0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX};
    }

    static std::vector<VkVertexInputAttributeDescription> attributeDisc(){
        return {
                {0, 0, VK_FORMAT_R32G32B32A32_SFLOAT, (uint32_t)offsetof(Vertex, position)},
                {1, 0, VK_FORMAT_R32G32B32_SFLOAT, (uint32_t)offsetof(Vertex, normal)},
                {2, 0, VK_FORMAT_R32G32B32_SFLOAT, (uint32_t)offsetof(Vertex, color)},
                {3, 0, VK_FORMAT_R32G32_SFLOAT, (uint32_t)offsetof(Vertex, uv)},
        };
    }
};


using Vertices = std::vector<Vertex>;
using Indices = std::vector<uint32_t>;

struct Mesh{
    Vertices vertices;
    Indices indices;
};

namespace primitives{

    inline Mesh cube(const glm::vec3& color = {1, 0, 0}){
        Mesh mesh;
        mesh.vertices = {
                {{-0.5f, -0.5f, 0.5f, 1.0f},{0.0f, 0.0f, 1.0f}, color, {0.0f, 0.0f}},
                {{0.5f, -0.5f, 0.5f, 1.0f}, {0.0f, 0.0f, 1.0f}, color, {1.0f, 0.0f}},
                {{0.5f,  0.5f, 0.5f, 1.0f}, {0.0f, 0.0f, 1.0f}, color, {1.0f, 1.0f}},
                {{-0.5f,  0.5f, 0.5f, 1.0f}, {0.0f, 0.0f, 1.0f}, color, {0.0f, 1.0f}},

                {{ 0.5f, -0.5f, 0.5f, 1.0f}, {1.0f, 0.0f, 0.0f}, color, {0.0f, 0.0f}},
                {{0.5f, -0.5f, -0.5f, 1.0f}, {1.0f, 0.0f, 0.0f}, color, {1.0f, 0.0f}},
                {{0.5f,  0.5f, -0.5f, 1.0f}, {1.0f, 0.0f, 0.0f}, color, {1.0f, 1.0f}},
                {{0.5f,  0.5f, 0.5f, 1.0f}, {1.0f, 0.0f, 0.0f}, color, {0.0f, 1.0f}},

                {{-0.5f, -0.5f, -0.5f, 1.0f}, {0.0f, 0.0f, -1.0f}, color, {0.0f, 0.0f}},
                {{-0.5f,  0.5f, -0.5f, 1.0f}, {0.0f, 0.0f, -1.0f}, color, {1.0f, 0.0f}},
                {{0.5f,  0.5f, -0.5f, 1.0f}, {0.0f, 0.0f, -1.0f}, color, {1.0f, 1.0f}},
                {{0.5f, -0.5f, -0.5f, 1.0f}, {0.0f, 0.0f, -1.0f}, color, {0.0f, 1.0f}},

                {{-0.5, -0.5, 0.5, 1.0f}, {-1.0f, 0.0f, 0.0f}, color, {0.0f, 0.0f}},
                {{-0.5f,  0.5f, 0.5f, 1.0f}, {-1.0f, 0.0f, 0.0f}, color, {1.0f, 0.0f}},
                {{-0.5f,  0.5f, -0.5f, 1.0f}, {-1.0f, 0.0f, 0.0f}, color, {1.0f, 1.0f}},
                {{-0.5f, -0.5f, -0.5f, 1.0f}, {-1.0f, 0.0f, 0.0f}, color, {0.0f, 1.0f}},

                {{-0.5f, -0.5f, 0.5f, 1.0f}, {0.0f, -1.0f, 0.0f}, color, {0.0f, 0.0f}},
                {{-0.5f, -0.5f, -0.5f, 1.0f}, {0.0f, -1.0f, 0.0f}, color, {1.0f, 0.0f}},
                {{0.5f, -0.5f, -0.5f, 1.0f}, {0.0f, -1.0f, 0.0f}, color, {1.0f, 1.0f}},
                {{0.5f, -0.5f, 0.5f, 1.0f}, {0.0f, -1.0f, 0.0f}, color, {0.0f, 1.0f}},

                {{-0.5f, 0.5f, 0.5f, 1.0f}, {0.0f, 1.0f, 0.0f}, color, {0.0f, 0.0f}},
                {{0.5f, 0.5f, 0.5f, 1.0f}, {0.0f, 1.0f, 0.0f}, color, {1.0f, 0.0f}},
                {{0.5f, 0.5f, -0.5f, 1.0f}, {0.0f, 1.0f, 0.0f}, color, {1.0f, 1.0f}},
                {{-0.5f, 0.5f, -0.5f, 1.0f}, {0.0f, 1.0f, 0.0f}, color, {0.0f, 1.0f}},
        };

        mesh.indices = {
                0,1,2,0,2,3,
                4,5,6,4,6,7,
                8,9,10,8,10,11,
                12,13,14,12,14,15,
                16,17,18,16,18,19,
                20,21,22,20,22,23
        };
        return mesh;
    }
}
