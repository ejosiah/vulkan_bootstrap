#include "VulkanBaseApp.h"

struct Vector{
    glm::vec2 vertex;
    glm::vec2 position;
};

struct Field{
    std::array<Texture, 2> texture;
    std::array<VkDescriptorSet, 2> descriptorSet{nullptr, nullptr};
    std::array<VulkanFramebuffer, 2> framebuffer;

    void swap(){
        std::swap(descriptorSet[0], descriptorSet[1]);
        std::swap(framebuffer[0], framebuffer[1]);
    }
};

using GpuProcess = std::function<void(VkCommandBuffer)>;

using ColorField = Field;
using VectorField = Field;
using PressureField = Field;
using DivergenceField = Field;
using ForceField = Field;
using VorticityField = Field;

class FluidSimulation : public VulkanBaseApp{
public:
    explicit FluidSimulation(const Settings& settings = {});

protected:
    void initApp() override;

    void initVectorField();

    void initColorField();

    void initSimulationStages();

    void initProjectionStage();

    void initStage(VulkanRenderPass& renderPass, const std::string& name);

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

    void computeVorticityConfinement(VkCommandBuffer commandBuffer);

    void applyForces(VkCommandBuffer commandBuffer);

    void userInputForce(VkCommandBuffer commandBuffer);

    void clearForces(VkCommandBuffer commandBuffer);

    void addSources(VkCommandBuffer commandBuffer, Field& sourceField, Field& destinationField);

    void advectColor(VkCommandBuffer commandBuffer);

    void advectVectorField(VkCommandBuffer commandBuffer);

    static void clear(VkCommandBuffer commandBuffer, Texture& texture);

    void advect(VkCommandBuffer commandBuffer, const std::array<VkDescriptorSet, 2>& sets, VulkanFramebuffer& framebuffer);

    void project(VkCommandBuffer commandBuffer);

    void computeDivergence(VkCommandBuffer commandBuffer);

    void solvePressure(VkCommandBuffer commandBuffer);

    void diffuse(VkCommandBuffer commandBuffer, Field& field, float rate = 1);

    void jacobiIteration(VkCommandBuffer commandBuffer, VkDescriptorSet unknownDescriptor, VkDescriptorSet solutionDescriptor, float alpha, float rBeta);

    void computeDivergenceFreeField(VkCommandBuffer commandBuffer);

    void withRenderPass(VkCommandBuffer commandBuffer, const VulkanRenderPass& renderPass, const VulkanFramebuffer& framebuffer
                        , GpuProcess&& process, glm::vec4 clearColor = glm::vec4(0));

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
    } divergence;

    struct {
        VulkanPipeline pipeline;
        VulkanPipelineLayout layout;
    } pressure;


    struct {
        VulkanPipeline pipeline;
        VulkanPipelineLayout layout;
    } divergenceFree;

    struct {
        VulkanPipeline pipeline;
        VulkanPipelineLayout layout;
        struct {
            float dt;
        } constants;
    } addSource;

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
    } vorticity;

    struct {
        VulkanPipeline pipeline;
        VulkanPipelineLayout layout;
        struct {
            float vorticityConfinementScale{20};
        } constants;
    } vorticityForce;

    struct {
        VulkanPipeline pipeline;
        VulkanPipelineLayout layout;
        struct{
            float alpha;
            float rBeta;
        } constants;
    } jacobi;

    static const int in{0};
    static const int out{1};

    struct {
        bool advectVField = true;
        bool project = true;
        bool showArrows = false;
        int poissonIterations = 30;
        float viscosity = MIN_FLOAT;
        float diffuseRate = MIN_FLOAT;
    } options;

    VectorField vectorField;
    ColorField colorField;
    DivergenceField divergenceField;
    PressureField pressureField;
    ForceField forceField;
    VorticityField vorticityField;

    struct {
        float dt{1.0f / 120.f};
        float epsilon{1};
        float rho{1};
    } constants;

    struct {
        Texture texture;
        VkDescriptorSet solutionDescriptorSet;
    } diffuseHelper;

    VulkanRenderPass simRenderPass;
    VulkanBuffer debugBuffer;
    VulkanSampler valueSampler;
    VulkanSampler linearSampler;
};