#include "VulkanBaseApp.h"

struct Vector{
    glm::vec2 vertex;
    glm::vec2 position;
};

struct Field{
    std::array<Texture, 2> texture;
    std::array<VkDescriptorSet, 2> descriptorSet;
    std::array<VulkanFramebuffer, 2> framebuffer;

    void swap(){
        std::swap(descriptorSet[0], descriptorSet[1]);
        std::swap(framebuffer[0], framebuffer[1]);
    }
};

using ColorField = Field;
using VectorField = Field;
using PressureField = Field;
using DivergenceField = Field;

class FluidSimulation : public VulkanBaseApp{
public:
    explicit FluidSimulation(const Settings& settings = {});

protected:
    void initApp() override;

    void initVectorField();

    void initColorField();

    void initAdvectStage();

    void initFullScreenQuad();

    void createDescriptorPool();

    void createDescriptorSetLayouts();

    void updateDescriptorSets();

    void createCommandPool();

    void createPipelineCache();

    void createRenderPipeline();

    void onSwapChainDispose() override;

    void onSwapChainRecreation() override;

    VkCommandBuffer *buildCommandBuffers(uint32_t imageIndex, uint32_t &numCommandBuffers) override;

    void renderVectorField(VkCommandBuffer commandBuffer);

    void renderColorField(VkCommandBuffer commandBuffer);

    void update(float time) override;

    void runSimulation();

    void advectColor(VkCommandBuffer commandBuffer);

    void advectVectorField(VkCommandBuffer commandBuffer);

    void advect(VkCommandBuffer commandBuffer, const std::array<VkDescriptorSet, 2>& sets, VulkanFramebuffer& framebuffer);

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

    VulkanDescriptorSetLayout textureSetLayout;
    struct {
        VulkanBuffer vertices;
        VulkanPipelineLayout layout;
        VulkanPipeline pipeline;
    } screenQuad;


    struct {
        VulkanPipeline pipeline;
        VulkanPipelineLayout layout;
        VulkanRenderPass renderPass;
        struct {
            float dt{1.0f/240.f};
        } constants;
    } advectPipeline;


    struct {
        VulkanPipeline pipeline;
        VulkanPipelineLayout layout;
        VulkanBuffer vertexBuffer;
        uint32_t numArrows{0};
    } arrows;

    struct {
        VulkanPipeline pipeline;
        VulkanPipelineLayout layout;
        VulkanRenderPass renderPass;
    } divergence;

    static const int in{0};
    static const int out{1};

    struct {
        bool advectVField = false;
        bool project = false;
        bool showArrows = true;
        int poissonIterations = 80;
    } options;

    VectorField vectorField;
    ColorField colorField;
    DivergenceField divergenceField;
    PressureField pressureField;

    float dt{1.0f/240.f};
    float epsilon{1};
    float rho{1};
};