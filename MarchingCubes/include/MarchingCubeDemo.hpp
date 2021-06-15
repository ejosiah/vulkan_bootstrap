#pragma once

#include "VulkanBaseApp.h"



struct mVertex{
    glm::vec4 position;
    glm::vec4 normal;
};

inline bool similar(const mVertex& a, const mVertex& b, float epsilon = 1E-7){
    return closeEnough(a.position.x, b.position.x, epsilon)
           && closeEnough(a.position.y, b.position.y, epsilon)
           && closeEnough(a.position.z, b.position.z, epsilon);
};

template<>
struct std::less<mVertex>{

    bool operator()(const mVertex& a, const mVertex& b) const {
        return a.position.x < b.position.x
                && a.position.y < b.position.y
                && a.position.z < b.position.z;
    }
};

template<>
struct std::hash<mVertex>{

    size_t operator()(const mVertex& a) const noexcept {
        size_t h = std::hash<float>{}(a.position.x);
               h ^= (std::hash<float>{}(a.position.y) << 1);
               h ^= (std::hash<float>{}(a.position.z) << 1);
        return h;
    }
};

template<>
struct std::equal_to<mVertex>{

    bool operator()(const mVertex& a, const mVertex& b) const {
        return  similar(a, b);
    }
};

class MarchingCubeDemo : public VulkanBaseApp{
public:
    MarchingCubeDemo(Settings settings);

protected:
    void initApp() override;

    void initCamera();

    void initSdf();

    void createSdf();

    int march(int pass);

    void updateMarchingCubeVertexDescriptorSet();

    void initVertexBuffer();

    void createCellBuffer();

    void createDescriptorPool();

    void createDescriptorSetLayout();

    void initMarchingCubeBuffers();

    void createMarchingCubeDescriptorSetLayout();

    void createMarchingCubeDescriptorSet();

    void createMarchingCubePipeline();

    void createDescriptorSet();

    void createPipeline();

    void onSwapChainDispose() override;

    void onSwapChainRecreation() override;

    VkCommandBuffer *buildCommandBuffers(uint32_t imageIndex, uint32_t &numCommandBuffers) override;

    void update(float time) override;

    void checkAppInputs() override;

    void cleanup() override;

    void generateTriangles();

    void renderText(VkCommandBuffer commandBuffer);

    void generateIndex(VulkanBuffer& source, VulkanBuffer& vertices, VulkanBuffer& indices);

private:
    VulkanCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;
    std::vector<glm::vec3> vertices;
    std::vector<uint32_t> indices;
    VulkanBuffer vertexBuffer;
    VulkanBuffer drawCommandBuffer;
    VulkanBuffer cellBuffer;
    VulkanBuffer cellIndices;
    Action* nextConfig = nullptr;
    uint32_t config = 0;

    struct{
        VulkanPipeline points;
        VulkanPipeline lines;
        VulkanPipeline triangles;
        VulkanPipeline sdf;
    } pipelines;

    struct {
        VulkanPipelineLayout lines;
        VulkanPipelineLayout points;
        VulkanPipelineLayout triangles;
        VulkanPipelineLayout sdf;
    } pipelineLayout;

    struct {
        VulkanDescriptorSetLayout descriptorSetLayout;
        VkDescriptorSet descriptorSet;
        VulkanPipelineLayout layout;
        VulkanPipeline pipeline;
        VulkanBuffer atomicCounterBuffers;
        VulkanBuffer edgeTableBuffer;
        VulkanBuffer triTableBuffer;
        VulkanBuffer vertexBuffer;
        VulkanBuffer indexBuffer;
        int numVertices;

        struct {
            float isoLevel = 1E-8;
            int pass = 0;
//            int config = 0;
        } constants;
    } marchingCube;

    Texture sdf;
    uint32_t sdfSize = 256;
    uint32_t numGrids = 4;
    Texture normalMap;

    VulkanDescriptorSetLayout sdfDescriptorSetLayout;
    VulkanDescriptorSetLayout computeDescriptorSetLayout;
    VkDescriptorSet sdfDescriptorSet;
    VkDescriptorSet computeDescriptorSet;
    VulkanDescriptorPool descriptorPool;

    std::unique_ptr<OrbitingCameraController> camera;
};