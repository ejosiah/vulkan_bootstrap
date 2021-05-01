#pragma once

#include "common.h"
#include "Vertex.h"
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include "primitives.h"

namespace mesh {

    constexpr uint32_t DEFAULT_PROCESS_FLAGS = aiProcess_GenSmoothNormals | aiProcess_Triangulate | aiProcess_CalcTangentSpace | aiProcess_JoinIdenticalVertices;

    struct Material{
        std::string name;
        alignas(16) glm::vec3 diffuse = glm::vec3(0.6f);
        alignas(16) glm::vec3 ambient = glm::vec3(0.6f);
        alignas(16) glm::vec3 specular = glm::vec3(1);
        alignas(16) glm::vec3 emission = glm::vec3(0);
        alignas(16) glm::vec3 transmittance = glm::vec3(0);
        float shininess = 0;
        float ior = 0;
        float opacity = 1;
        float illum = 1;
    };

    struct TextureMaterial{
        std::string diffuseMap;
        std::string ambientMap;
        std::string specularMap;
        std::string normalMap;
        std::string ambientOcclusionMap;
    };

    struct Mesh {
        std::string name{};
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        Material material{};
        TextureMaterial textureMaterial{};
        VkPrimitiveTopology primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        struct {
            glm::vec3 min = glm::vec3(MAX_FLOAT);
            glm::vec3 max = glm::vec3(MIN_FLOAT);
        } bounds;
    };

    int load(std::vector<Mesh>& meshes, const std::string& path, uint32_t flags = DEFAULT_PROCESS_FLAGS);

    inline void normalize(std::vector<Mesh>& meshes, float scale = 1.0f){
        primitives::normalize(meshes, scale);
    }
}