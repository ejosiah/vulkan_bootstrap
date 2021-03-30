#pragma once

#include <vector>
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

struct Vertex{
    glm::vec4 position;
    glm::vec3 normal;
    glm::vec4 color;
    glm::vec2 uv; // TODO std::array<std::vector<glm::vec2>, MAX_UVS>>;

    static std::vector<VkVertexInputBindingDescription> bindingDisc(){
        return {
                {0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX}
        };
    }

    static std::vector<VkVertexInputAttributeDescription> attributeDisc(){
        return {
                {0, 0, VK_FORMAT_R32G32B32A32_SFLOAT, (uint32_t)offsetof(Vertex, position)},
                {1, 0, VK_FORMAT_R32G32B32_SFLOAT, (uint32_t)offsetof(Vertex, normal)},
                {2, 0, VK_FORMAT_R32G32B32A32_SFLOAT, (uint32_t)offsetof(Vertex, color)},
                {3, 0, VK_FORMAT_R32G32_SFLOAT, (uint32_t)offsetof(Vertex, uv)}
        };
    }
};


struct Vertices{
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
};