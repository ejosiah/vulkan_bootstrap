#include "VulkanBaseApp.h"
#include "fluid_solver_2d.h"

using ColorField = Field;

class FluidSimulation : public VulkanBaseApp{
public:
    explicit FluidSimulation(const Settings& settings = {});

protected:
    void initApp() override;

    void initFluidSolver();

    void initColorField();

    void initFullScreenQuad();

    void createDescriptorPool();

    void createDescriptorSetLayouts();

    void createCommandPool();

    void createPipelineCache();

    void createRenderPipeline();

    void onSwapChainDispose() override;

    void onSwapChainRecreation() override;

    VkCommandBuffer *buildCommandBuffers(uint32_t imageIndex, uint32_t &numCommandBuffers) override;

    void renderColorField(VkCommandBuffer commandBuffer);

    void update(float time) override;

    void runSimulation();

    ExternalForce userInputForce();

    void addDyeSource(VkCommandBuffer commandBuffer, Field& field, glm::vec3 color, glm::vec2 source);

    void checkAppInputs() override;

    void cleanup() override;

    void onPause() override;

    void createSamplers();

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
        struct {
            glm::vec2 force{0};
            glm::vec2 center{0};
            float radius{1};
            float dt;
        } constants;
    } forceGen;

    struct {
        VulkanPipeline pipeline;
        VulkanPipelineLayout layout;
        struct{
            glm::vec4 color;
            glm::vec2 source;
            float radius{0.01};
            float dt{1.0f/120.f};
        } constants;
    } dyeSource;

    static const int in{0};
    static const int out{1};

    Quantity color;

    struct {
        float dt{1.0f / 120.f};
        float epsilon{1};
        float rho{1};
    } constants;

    VulkanBuffer debugBuffer;
    VulkanSampler valueSampler;
    VulkanSampler linearSampler;
    FluidSolver2D fluidSolver;
};