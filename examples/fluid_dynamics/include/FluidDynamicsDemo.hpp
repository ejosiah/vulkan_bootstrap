#include "VulkanBaseApp.h"
#include "vulkan_context.hpp"
#include "fluid_sim_gpu.h"

struct PatchVertex{
    glm::vec2 point{0};
    float orientation{0};
};

class FluidDynamicsDemo : public VulkanBaseApp{
public:
    explicit FluidDynamicsDemo(const Settings& settings = {});

protected:
    void initApp() override;

    void initFluidSim();

    void initSimData();

    void initGrid();

    void initCamera();

    void initVectorRender();

    void initDensityRender();

    void initRenderBuffer();

    void initParticles();

    void createDescriptorPool();

    void createDescriptorSetLayouts();

    void updateDescriptorSets();

    void createCommandPool();

    void createPipelineCache();

    void createRenderPipeline();

    void createComputePipeline();

    void onSwapChainDispose() override;

    void onSwapChainRecreation() override;

    VkCommandBuffer *buildCommandBuffers(uint32_t imageIndex, uint32_t &numCommandBuffers) override;

    void renderGrid(VkCommandBuffer commandBuffer);

    void renderVectorField(VkCommandBuffer commandBuffer);

    void renderDensityField(VkCommandBuffer commandBuffer);

    void renderParticles(VkCommandBuffer commandBuffer);

    void renderUI(VkCommandBuffer commandBuffer);

    void renderBrush(VkCommandBuffer commandBuffer);

    void renderImage(VkCommandBuffer commandBuffer);

    void update(float time) override;

    void gpuSimulation();

    void checkAppInputs() override;

    void cleanup() override;

    void onPause() override;

    void updateVelocityField();

    void updateDensityField();

    void initBrush();

    void createUVTexture();

protected:
    struct {
        VulkanPipelineLayout layout;
        VulkanPipeline pipeline;
        VulkanBuffer vertexBuffer;
    } render;

    struct {
        VulkanPipelineLayout layout;
        VulkanPipeline pipeline;
    } compute;

    VulkanDescriptorPool descriptorPool;
    VulkanCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;
    VulkanPipelineCache pipelineCache;

    struct {
        std::array<VulkanBuffer, 2> densityBuffer;
        std::array<VulkanBuffer, 2> uBuffer;
        std::array<VulkanBuffer, 2> vBuffer;
        std::array<float*, 2> density{};
        std::array<float*, 2> u{};
        std::array<float*, 2> v{};
        VkDeviceSize size;
        int N{32};
        float diff{1};
        float visc{0};
        VulkanDescriptorSetLayout setLayout;
        std::array<VkDescriptorSet, 2> descriptorSets;
    } simData;

    struct {
        VulkanPipeline pipeline;
        VulkanPipelineLayout layout;
        VulkanBuffer vertexBuffer;
    } grid;

    struct {
        struct {
            VulkanPipeline pipeline;
            VulkanPipelineLayout layout;
            VulkanBuffer vertexBuffer;
        } thick;

        struct {
            VulkanPipeline pipeline;
            VulkanPipelineLayout layout;
            VulkanBuffer vertexBuffer;
        } thin;
    } vectorView;

    struct {
        VulkanPipeline pipeline;
        VulkanPipelineLayout layout;
        VulkanBuffer vertexBuffer;
    } density;


    struct {
        VulkanPipeline pipeline;
        VulkanPipelineLayout layout;
        VulkanBuffer vertexBuffer;
        VulkanBuffer particleBuffer;
        VulkanDescriptorSetLayout setLayout;
        VkDescriptorSet descriptorSet;
        glm::vec2* particlePtr;
    } particles;

    Camera camera;
    VulkanDescriptorSetLayout camSetLayout;
    VkDescriptorSet cameraSet;
    VulkanBuffer camBuffer;

    struct {
        std::deque<std::tuple<glm::vec2, float>> trail;
        std::map<int, glm::vec2> gridData;
        VulkanBuffer patch;
        VulkanPipeline pipeline;
        VulkanPipelineLayout layout;
        bool active{false};
        bool erase{true};
        struct {
            glm::vec2 position{0.5};
            float radius{0.0};
        } constants;
    } brush;

    float speed{0.05};
    bool showGrid{true};
    bool showVectorField{true};
    bool showDensityField{false};
    bool showParticles{false};
    bool debug{true};
    bool simStarted{true};

    static constexpr int PAINT_VECTOR_FIELD = 0;
    static constexpr int PAINT_DENSITY_FIELD = 1;
    int paintState{PAINT_DENSITY_FIELD};

    struct {
        int N{0};
        float maxMagnitude{1};
        float dt{0};
    } constants;

    Texture colorTexture;
    VulkanDescriptorSetLayout textureDescriptorSetLayout;
    VkDescriptorSet textureDescriptorSet;
    FluidSim fluidSim;
    VulkanCommandPool simCommandPool;
};