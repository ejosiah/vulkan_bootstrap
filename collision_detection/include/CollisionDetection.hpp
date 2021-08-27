#include "VulkanBaseApp.h"

struct Object{
    glm::vec3 center;
    float radius;
    VulkanBuffer vertexBuffer;
    VulkanBuffer crossHairBuffer;
    VulkanBuffer indexBuffers;
};

struct Objects{

    void init(const VulkanDevice& device){
        Object object{glm::vec3(-0.30), 0.33f};
        auto vertices = primitives::sphere(50, 50, object.radius, glm::translate(glm::mat4{1}, object.center), glm::vec4{0, 1, 0, 0.2});
        object.vertexBuffer = device.createCpuVisibleBuffer(vertices.vertices.data(), sizeof(Vertex) * vertices.vertices.size(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
        object.indexBuffers = device.createCpuVisibleBuffer(vertices.indices.data(), sizeof(uint32_t) * vertices.indices.size(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
        createCrossHair(device, object, 0.05);
        objects.push_back(std::move(object));

        object = {{-0.70, -0.20, 0.05}, 0.2};
        vertices = primitives::sphere(50, 50, object.radius, glm::translate(glm::mat4{1}, object.center), glm::vec4{0, 1, 0, 0.2});
        object.vertexBuffer = device.createCpuVisibleBuffer(vertices.vertices.data(), sizeof(Vertex) * vertices.vertices.size(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
        object.indexBuffers = device.createCpuVisibleBuffer(vertices.indices.data(), sizeof(uint32_t) * vertices.indices.size(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
        createCrossHair(device, object, 0.05);
        objects.push_back(std::move(object));


    }

    void createCrossHair(const VulkanDevice& device, Object& object, float length){
        std::vector<Vertex> vertices;
        float h = length * 0.5f;
        Vertex vertex{};
        vertex.color = {1, 1, 0, 1};

        vertex.position = glm::vec4{object.center + glm::vec3{h, 0, 0}, 1};
        vertices.push_back(vertex);

        vertex.position = glm::vec4{object.center + glm::vec3{-h, 0, 0}, 1};
        vertices.push_back(vertex);

        vertex.position = glm::vec4{object.center + glm::vec3{0, h, 0}, 1};
        vertices.push_back(vertex);

        vertex.position = glm::vec4{object.center + glm::vec3{0, -h, 0}, 1};
        vertices.push_back(vertex);

        vertex.position = glm::vec4{object.center + glm::vec3{0, 0, h}, 1};
        vertices.push_back(vertex);

        vertex.position = glm::vec4{object.center + glm::vec3{0, 0, -h}, 1};
        vertices.push_back(vertex);

        object.crossHairBuffer = device.createCpuVisibleBuffer(vertices.data(), BYTE_SIZE(vertices), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);

    }

    std::vector<Object> objects;

    struct {
        VulkanPipeline perspectivePipeline;
        VulkanPipeline topPipeline;
        VulkanPipeline rightPipeline;
        VulkanPipeline leftPipeline;
    } render;

    struct {
        VulkanPipeline perspectivePipeline;
        VulkanPipeline topPipeline;
        VulkanPipeline rightPipeline;
        VulkanPipeline leftPipeline;
    } crossHair;

    VulkanPipelineLayout layout;
};

enum class ViewPort{
    PERSPECTIVE, TOP, RIGHT, LEFT
};

class CollisionDetection : public VulkanBaseApp{
public:
    explicit CollisionDetection(const Settings& settings = {});

protected:
    void initApp() override;

    void createDescriptorPool();

    void createGridBuffers();

    void createGridDescriptorSetLayout();

    void createGridDescriptorSet();

    void createCommandPool();

    void createPipelineCache();

    void createRenderPipeline();

    void createGridPipeline();

    void createComputePipeline();

    void onSwapChainDispose() override;

    void onSwapChainRecreation() override;

    VkCommandBuffer *buildCommandBuffers(uint32_t imageIndex, uint32_t &numCommandBuffers) override;

    void drawGrid(VkCommandBuffer commandBuffer, VkPipeline pipeline, VkPipelineLayout layout, const Camera& camera);

    void drawObject(VkCommandBuffer commandBuffer, VkPipeline objectPipeline, VkPipeline crossHairPipeline,
                    VkPipelineLayout layout, const Camera& camera);

    void update(float time) override;

    void checkAppInputs() override;

    void cleanup() override;

    void onPause() override;

    void initCamera();

protected:
    struct {
        VulkanPipelineLayout layout;
        VulkanPipeline perspectivePipeline;
        VulkanPipeline topPipeline;
        VulkanPipeline rightPipeline;
        VulkanPipeline leftPipeline;
        VulkanDescriptorSetLayout setLayout;
        VkDescriptorSet descriptorSet;
        VulkanBuffer mvp;
        VulkanBuffer vertexBuffer;
        uint32_t numCells = 9;
        float size = 2.0;
        uint32_t vertexCount;
    } grid;

    struct {
        VulkanPipelineLayout layout;
        VulkanPipeline pipeline;
    } compute;

    VulkanDescriptorPool descriptorPool;
    VulkanCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;
    VulkanPipelineCache pipelineCache;
    std::unique_ptr<OrbitingCameraController> camera;
    Objects objects;

    ViewPort activeViewPort = ViewPort::PERSPECTIVE;

    struct Cameras{
        Camera top;
        Camera right;
        Camera front;
    } cameras;
};