#include "VulkanDevice.h"
#include "VulkanRenderPass.h"
#include "VulkanFramebuffer.h"
#include "VulkanRAII.h"
#include "Texture.h"
#include "filemanager.hpp"

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

class FluidSolver2D{
public:
    struct {
        bool advectVField = true;
        bool project = true;
        bool showArrows = false;
        bool vorticity = true;
        int poissonIterations = 30;
        float viscosity = MIN_FLOAT;
        float diffuseRate = MIN_FLOAT;
        float dt{1.0f / 120.f};
    } options;

    explicit FluidSolver2D(VulkanDevice* device, FileManager* fileManager);

    void init();

    void initFullScreenQuad();

    void createDescriptorPool();

    void createDescriptorSetLayouts();

    void updateDescriptorSets();

    void createPipelines();

    void set(VulkanBuffer vectorField);

    void add(GpuProcess&& force);

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

private:
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

    static const int in{0};
    static const int out{1};


    VectorField vectorField;
    ColorField colorField;
    DivergenceField divergenceField;
    PressureField pressureField;
    ForceField forceField;
    VorticityField vorticityField;


    struct {
        Texture texture;
        VkDescriptorSet solutionDescriptorSet;
    } diffuseHelper;

    VulkanRenderPass simRenderPass;
    VulkanDescriptorPool descriptorPool;
    VulkanBuffer debugBuffer;
    VulkanSampler valueSampler;
    VulkanSampler linearSampler;
    std::vector<GpuProcess> forces;
    FileManager* fileManager{nullptr};
    VulkanDevice* device;

};