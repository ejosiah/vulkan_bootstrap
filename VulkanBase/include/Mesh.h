#include "common.h"
#include "Vertex.h"
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

namespace mesh {

    constexpr uint32_t DEFAULT_PROCESS_FLAGS = aiProcess_GenSmoothNormals | aiProcess_Triangulate | aiProcess_CalcTangentSpace | aiProcess_JoinIdenticalVertices;

    struct Material{

    };

    struct Mesh {
        std::string name{};
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        Material material{};
        VkPrimitiveTopology primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    };

    void load(std::vector<Mesh>& meshes, std::string_view path, uint32_t flags = DEFAULT_PROCESS_FLAGS);

    void normalize(std::vector<Mesh>& meshes, float scale = 1.0f);
}