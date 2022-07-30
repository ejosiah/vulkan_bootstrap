#include "VulkanBaseApp.h"
#include "fluid_solver_2d.h"

using ColorField = Field;

class FluidSimulation : public VulkanBaseApp{
public:
    explicit FluidSimulation(const Settings& settings = {});

protected:
    void initApp() override;

    void initVectorField();

    void initFluidSolver();

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

    void clearSources(VkCommandBuffer commandBuffer);

    void addColors(VkCommandBuffer commandBuffer);

    void addDyeSource(VkCommandBuffer commandBuffer, glm::vec3 color, glm::vec2 source);

    void addDyeSource(VkCommandBuffer commandBuffer, VulkanRenderPass& renderPass, Field& field, glm::vec3 color, glm::vec2 source);

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
            float vorticityConfinementScale{10};
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

    struct {
        bool advectVField = true;
        bool project = true;
        bool showArrows = false;
        bool vorticity = true;
        int poissonIterations = 30;
        float viscosity = MIN_FLOAT;
        float diffuseRate = MIN_FLOAT;
    } options;

    VectorField vectorField;
    Quantity colorQuantity;
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
    FluidSolver2D fluidSolver;
};