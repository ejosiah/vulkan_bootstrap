#pragma once

#include "fluid_solver_common.h"
#include "VulkanCommandBuffer.h"
#include "VulkanFramebuffer.h"
#include "Texture.h"
#include "glm/glm.hpp"
#include "filemanager.hpp"

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
using PostAdvect = std::function<bool(VkCommandBuffer, Field&)>;


struct Quantity{
    std::string name{};
    Field field;
    Field source;
    float diffuseRate{0};
    UpdateSource update = [](VkCommandBuffer, Field&){};
    PostAdvect postAdvect = [](VkCommandBuffer, Field&) { return false; };
};

class FluidSolver{
public:
    FluidSolver() = default;
    FluidSolver(VulkanDevice *device, VulkanDescriptorPool *descriptorPool,
                VulkanRenderPass *displayRenderPass, FileManager *fileManager, glm::uvec3 gridSize);

    void withRenderPass(VkCommandBuffer commandBuffer, const VulkanFramebuffer& framebuffer
            , GpuProcess&& process, glm::vec4 clearColor = glm::vec4(0)) const;


    void updateDescriptorSet(Field &field);

    void createDescriptorSets(Quantity& quantity);

protected:

    void initBuffers();

    virtual std::tuple<VkDeviceSize, void*> getGlobalConstants() = 0;

    void createSamplers();

    void createRenderPass();

    virtual void createPipelines();

    void createDescriptorSetLayouts();

    void initSimData();

    void initViewVectors();

    void initFullScreenQuad();

    std::string resource(const std::string& name);

    virtual void updateDescriptorSets();

    void updateUboDescriptorSets();

    void updateDiffuseDescriptorSet();

    void updateAdvectDescriptorSet();

protected:
    VulkanDescriptorSetLayout samplerSet;
    VulkanDescriptorSetLayout advectTextureSet;
    VkDescriptorSet samplerDescriptorSet{};
    VulkanDevice* device{};
    VulkanRenderPass* displayRenderPass{};
    VulkanDescriptorPool* descriptorPool{};
    FileManager* fileManager{};

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
            float vorticityConfinementScale{0};
        } constants;
    } vorticityForce;

    struct {
        VulkanPipeline pipeline;
        VulkanPipelineLayout layout;
        struct{
            float alpha{1};
            float rBeta{1};
            int isVectorField{0};
        } constants;
    } jacobi;

    struct {
        VulkanBuffer vertices;
        VulkanPipelineLayout layout;
        VulkanPipeline pipeline;
    } screenQuad;

    VulkanDescriptorSetLayout globalConstantsSet;
    VkDescriptorSet globalConstantsDescriptorSet{};

    uint32_t width{};
    uint32_t height{};
    uint32_t depth{1};

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

    VulkanSampler valueSampler;
    VulkanSampler linearSampler;

    static const int in{0};
    static const int out{1};

    float timeStep{1/120.f};
    float _elapsedTime{};

    VulkanBuffer globalConstantsBuffer;
    VulkanBuffer debugBuffer;

public:
    VulkanRenderPass renderPass;
    VulkanDescriptorSetLayout textureSetLayout;
};