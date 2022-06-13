#include "VulkanBaseApp.h"

struct PatchVertex{
    glm::vec2 point{0};
    float orientation{0};
};

class FluidDynamicsDemo : public VulkanBaseApp{
public:
    explicit FluidDynamicsDemo(const Settings& settings = {});

protected:
    void initApp() override;

    void initSimData();

    void initGrid();

    void initCamera();

    void initVectorView();

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

    void update(float time) override;

    void checkAppInputs() override;

    void cleanup() override;

    void onPause() override;

protected:
    struct {
        VulkanPipelineLayout layout;
        VulkanPipeline pipeline;
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
        std::array<VulkanBuffer, 2> velocityBuffer;
        std::array<float*, 2> density{};
        std::array<float*, 2> u{};
        std::array<float*, 2> v{};
        int N{30};
        float diff{1};
        float visc{1};
        VulkanDescriptorSetLayout setLayout;
        std::array<VkDescriptorSet, 2> descriptorSets;
    } simData;

    struct {
        VulkanPipeline pipeline;
        VulkanPipelineLayout layout;
        VulkanBuffer vertexBuffer;
    } grid;

    struct {
        VulkanPipeline pipeline;
        VulkanPipelineLayout layout;
        VulkanBuffer vertexBuffer;
    } vectorView;

    Camera camera;
    VulkanDescriptorSetLayout camSetLayout;
    VkDescriptorSet cameraSet;
    VulkanBuffer camBuffer;

    struct {
        int N{0};
        float maxMagnitude{1};
    } constants;
};