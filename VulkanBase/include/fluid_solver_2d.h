#include "VulkanDevice.h"
#include "VulkanRenderPass.h"
#include "VulkanFramebuffer.h"
#include "VulkanRAII.h"
#include "Texture.h"
#include "filemanager.hpp"
#include "glm/glm.hpp"

using GpuProcess = std::function<void(VkCommandBuffer)>;
using ExternalForce = std::function<void(VkCommandBuffer, VkDescriptorSet)>;

struct Vector{
    glm::vec2 vertex;
    glm::vec2 position;
};

struct Field{
    std::array<Texture, 2> texture;
    std::array<VkDescriptorSet, 2> descriptorSet{nullptr, nullptr};
    std::array<VkDescriptorSet, 2> advectDescriptorSet{nullptr, nullptr};
    std::array<VulkanFramebuffer, 2> framebuffer;

    void swap(){
        std::swap(descriptorSet[0], descriptorSet[1]);
        std::swap(advectDescriptorSet[0], advectDescriptorSet[1]);
        std::swap(framebuffer[0], framebuffer[1]);
    }
};

using VectorField = Field;
using PressureField = Field;
using DivergenceField = Field;
using ForceField = Field;
using VorticityField = Field;

using UpdateSource = std::function<void(VkCommandBuffer, Field&)>;


struct Quantity{
    std::string name{};
    Field field;
    Field source;
    float diffuseRate{0};
    UpdateSource update = [](VkCommandBuffer, Field&){};
};

class FluidSolver2D{
public:
    FluidSolver2D() = default;

    FluidSolver2D(VulkanDevice* device, VulkanDescriptorPool* descriptorPool, VulkanRenderPass* displayRenderPass, FileManager* fileManager, glm::vec2 gridSize);

    void init();

    void createSamplers();

    void createRenderPass();

    void initViewVectors();

    void initSimData();

    void initFullScreenQuad();

    void createDescriptorSetLayouts();

    void createDescriptorSets(Quantity& quantity);

    void updateDescriptorSets();

    void updateDescriptorSet(Field &field);

    void updateDiffuseDescriptorSet();

    void updateAdvectDescriptorSet();

    void createPipelines();

    void set(VulkanBuffer vectorFieldBuffer);

    void add(ExternalForce&& force);

    void runSimulation(VkCommandBuffer commandBuffer);

    void velocityStep(VkCommandBuffer commandBuffer);

    void withRenderPass(VkCommandBuffer commandBuffer, const VulkanFramebuffer& framebuffer
            , GpuProcess&& process, glm::vec4 clearColor = glm::vec4(0)) const;

    std::string resource(const std::string& name);

    void renderVectorField(VkCommandBuffer commandBuffer);

    void add(Quantity& quantity);

    void createFrameBuffer(Quantity& quantity);

    void dt(float value);

    float dt() const;

    void advectVelocity(bool flag);

    void project(bool flag);

    void showVectors(bool flag);

    void applyVorticity(bool flag);

    void poissonIterations(int value);

    void viscosity(float value);

protected:
    void quantityStep(VkCommandBuffer commandBuffer);

    void quantityStep(VkCommandBuffer commandBuffer, Quantity& quantity);

    void clearSources(VkCommandBuffer commandBuffer, Quantity &quantity);

    void updateSources(VkCommandBuffer commandBuffer, Quantity &quantity);

    void addSource(VkCommandBuffer commandBuffer, Quantity &quantity);

    void diffuseQuantity(VkCommandBuffer commandBuffer, Quantity &quantity);

    void advectQuantity(VkCommandBuffer commandBuffer, Quantity &quantity);

    void computeVorticityConfinement(VkCommandBuffer commandBuffer);

    void applyForces(VkCommandBuffer commandBuffer);

    void applyExternalForces(VkCommandBuffer commandBuffer);

    void clearForces(VkCommandBuffer commandBuffer);

    void addSources(VkCommandBuffer commandBuffer, Field& sourceField, Field& destinationField);

    void advectVectorField(VkCommandBuffer commandBuffer);

    static void clear(VkCommandBuffer commandBuffer, Texture& texture);

    void advect(VkCommandBuffer commandBuffer, const std::array<VkDescriptorSet, 2>& sets, VulkanFramebuffer& framebuffer);

    void project(VkCommandBuffer commandBuffer);

    void computeDivergence(VkCommandBuffer commandBuffer);

    void solvePressure(VkCommandBuffer commandBuffer);

    void diffuse(VkCommandBuffer commandBuffer, Field& field, float rate = 1);

    void jacobiIteration(VkCommandBuffer commandBuffer, VkDescriptorSet unknownDescriptor, VkDescriptorSet solutionDescriptor, float alpha, float rBeta);

    void computeDivergenceFreeField(VkCommandBuffer commandBuffer);

private:
    VulkanDescriptorSetLayout textureSetLayout;
    VulkanDescriptorSetLayout samplerSet;
    VulkanDescriptorSetLayout advectTextureSet;
    VkDescriptorSet samplerDescriptorSet;
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
    } addSourcePipeline;


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

    static const int in{0};
    static const int out{1};


    VectorField vectorField;
    DivergenceField divergenceField;
    PressureField pressureField;
    ForceField forceField;
    VorticityField vorticityField;
    std::vector<std::reference_wrapper<Quantity>> quantities;

    std::vector<ExternalForce> externalForces;

    struct {
        Texture texture;
        VkDescriptorSet solutionDescriptorSet;
    } diffuseHelper;

    VulkanRenderPass* displayRenderPass{};
    VulkanDescriptorPool* descriptorPool{};
    VulkanSampler valueSampler;
    VulkanSampler linearSampler;
    FileManager* fileManager{};
    bool vectorFieldSet{};
    VulkanDevice* device{};
    glm::vec2 gridSize{};
    glm::vec2 delta{};
    struct {
        float dt{1.0f / 120.f};
        float epsilon{1};
        float rho{1};
    } constants;

    uint32_t width{};
    uint32_t height{};

    struct {
        bool advectVField = true;
        bool project = true;
        bool showArrows = false;
        bool vorticity = false;
        int poissonIterations = 30;
        float viscosity = MIN_FLOAT;
        float dt{1.0f / 120.f};
    } options;

public:
    VulkanRenderPass renderPass;
};