#include "common.h"
#include "Vertex.h"
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

namespace mesh {

    constexpr uint32_t DEFAULT_PROCESS_FLAGS = aiProcess_GenSmoothNormals | aiProcess_Triangulate | aiProcess_CalcTangentSpace | aiProcess_JoinIdenticalVertices;

    struct Material{
        std::string name;
        glm::vec3 diffuse = glm::vec3(0.6f);
        glm::vec3 ambient = glm::vec3(0.6f);
        glm::vec3 specular = glm::vec3(1);
        glm::vec3 emission = glm::vec3(0);
        std::string diffuseMap;
        std::string ambientMap;
        std::string specularMap;
        std::string normalMap;
        std::string ambientOcclusionMap;
        float indexOfRefraction = 0;
        float shininess = 0;
        float opacity = 1;
    };

    struct Mesh {
        std::string name{};
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        Material material{};
        VkPrimitiveTopology primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        struct {
            glm::vec3 min = glm::vec3(std::numeric_limits<float>::max());
            glm::vec3 max = glm::vec3(std::numeric_limits<float>::min());
        } bounds;
    };

    int load(std::vector<Mesh>& meshes, std::string_view path, uint32_t flags = DEFAULT_PROCESS_FLAGS);

    void normalize(std::vector<Mesh>& meshes, float scale = 1.0f);
}