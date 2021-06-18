#include "VulkanBaseApp.h"

struct Particle{
    glm::vec4 position{0};
    glm::vec4 color{0};
    glm::vec3 velocity{0};
    float invMass{0};
};

class SPHFluidSimulation : public VulkanBaseApp{
public:
    explicit SPHFluidSimulation(const Settings& settings = {});

protected:
    void initApp() override;

    void initGrid();

    void createParticles();

    void initGridBuilder();

    void createDescriptorPool();

    void createDescriptorSetLayouts();

    void createDescriptorSets();

    void createCommandPool();

    void createPipelineCache();

    void createRenderPipeline();

    void createComputePipeline();

    void runPhysics();

    void onSwapChainDispose() override;

    void onSwapChainRecreation() override;

    VkCommandBuffer *buildCommandBuffers(uint32_t imageIndex, uint32_t &numCommandBuffers) override;

    void update(float time) override;

    void checkAppInputs() override;

    void cleanup() override;

    void onPause() override;

    void createGridBuilderDescriptorSetLayout();

    void createGridBuilderDescriptorSet(VkDeviceSize offset, VkDeviceSize range);

    void createGridBuilderPipeline();

    void buildPointHashGrid();

    void GeneratePointHashGrid(int pass = 0);

protected:
    struct {
        VulkanPipelineLayout layout;
        VulkanPipeline pipeline;
    } render;

    struct {
        VulkanPipelineLayout layout;
        VulkanPipeline pipeline;
    } compute;

    struct {
        VulkanBuffer vertexBuffer;
        struct {
            glm::mat4 model = glm::mat4(1);
            glm::mat4 view  = glm::mat4(1);
            glm::mat4 projection = glm::mat4(1);
        } xforms;
        uint32_t numCells = 100;
        float size = 2.0f;
    } grid;

    struct {
        VulkanPipelineLayout layout;
        VulkanPipeline pipeline;
        std::array<VulkanBuffer, 2> buffers;
        std::array<VkDescriptorSet, 2> descriptorSets;
        VulkanDescriptorSetLayout descriptorSetLayout;

        struct {
            glm::vec3 gravity{0, -9.8, 0};
            uint32_t numParticles{ 1024 };
            float drag = 1e-4;
            float time;
        } constants;

    } particles;

    struct {
        VulkanPipelineLayout layout;
        VulkanPipeline pipeline;
        VulkanDescriptorSetLayout descriptorSetLayout;
        VkDescriptorSet descriptorSet;
        VulkanBuffer buffer;

        struct {
            glm::ivec3 resolution{1};
            float gridSpacing{1};
            uint32_t pass{0};
            uint32_t numParticles{0};
        } constants;
    } gridBuilder;

    VulkanDescriptorPool descriptorPool;
    VulkanCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;
    VulkanPipelineCache pipelineCache;
};