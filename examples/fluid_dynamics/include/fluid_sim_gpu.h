#pragma once

#include "VulkanDevice.h"
#include <array>
#include "ComputePipelins.hpp"
#include "filemanager.hpp"
#include "Texture.h"

#define HORIZONTAL_COMPONENT_BOUNDARY 1
#define VERTICAL_COMPONENT_BOUNDARY 2
#define SCALAR_FIELD_BOUNDARY 0

#define VELOCITY_FIELD_U "velocity_field_u"
#define VELOCITY_FIELD_V "velocity_field_v"
#define DENSITY "density"

using Quantity = std::string;

class FluidSim : public ComputePipelines{
public:
    FluidSim() = default;

    FluidSim(VulkanDevice* device, FileManager fileMgr,  VulkanBuffer u0, VulkanBuffer v0,
             VulkanBuffer u, VulkanBuffer v, int N, float dt, float dissipation);


    void init();

    void set(Texture& color);

    void initTextureSampler();

    void createDescriptorPool();

    void setVectorFieldQuantity();

    std::vector<PipelineMetaData> pipelineMetaData() override;

    void createDescriptorSets();

    void updateDescriptorSets();

    void run(float speed);

    void velocityStep(VkCommandBuffer commandBuffer, float dt = 0.0f, float viscosity = 1.0f);

    void quantityStep(VkCommandBuffer commandBuffer, const Quantity& quantity, float dt = 0.0f, float dissipation = 0.99f);

    void advectVectorField(VkCommandBuffer commandBuffer);

    void advect(VkCommandBuffer commandBuffer, const Quantity& quantity, int boundary);

    void calculateDivergence(VkCommandBuffer commandBuffer);

    void project(VkCommandBuffer commandBuffer, int iterations = 20);

    void clearSupportBuffers(VkCommandBuffer commandBuffer);

    void swapVectorFieldBuffers();

    void diffuseVectorField(VkCommandBuffer commandBuffer, int iterations);

    void diffuse(VkCommandBuffer commandBuffer, const Quantity& quantity, int boundary, int iterations = 20);

    void computePressureGradient(VkCommandBuffer commandBuffer);

    void solvePressureEquation(VkCommandBuffer commandBuffer, int iterations = 20);

    void computeDivergenceFreeField(VkCommandBuffer commandBuffer);

    void setBoundary(VkCommandBuffer commandBuffer, VkDescriptorSet descriptorSet, VulkanBuffer vulkanBuffer, int boundary);

    void setBoundary(VkCommandBuffer commandBuffer, const Quantity& quantity, int boundary);

    void dissipation(float value);

    static void barrier(VkCommandBuffer commandBuffer, const VulkanBuffer& buffer);

    static void barrier(VkCommandBuffer commandBuffer, const std::vector<VulkanBuffer>& buffers);

    void setQuantity(const Quantity& quantity, VulkanBuffer q0, VulkanBuffer q);

    const VulkanBuffer& inputBuffer(const Quantity& quantity);

    const VulkanBuffer& outputBuffer(const Quantity& quantity);

    std::array<VkDescriptorSet, 2>& descriptorSet(const Quantity& quantity);

    VkDescriptorSet inputDescriptorSet(const Quantity& quantity);

    VkDescriptorSet outputDescriptorSet(const Quantity& quantity);

    void resize(VulkanBuffer u0, VulkanBuffer v0,VulkanBuffer u, VulkanBuffer v, int N);

    void createSupportBuffers();

    std::string resource(const std::string& path);

    VulkanBuffer _divBuffer, _pressure, _gu, _gv;

public:
    VulkanDescriptorPool _descriptorPool;
    VulkanDescriptorSetLayout _velocitySetLayout;
    VulkanDescriptorSetLayout _quantitySetLayout;
    VulkanDescriptorSetLayout _scalaFieldSetLayout;
    VkDescriptorSet _divergenceDescriptorSet{};
    VkDescriptorSet _pressureDescriptorSet{};
    VkDescriptorSet _pressureGradientDescriptorSet{};
    std::array<VkDescriptorSet, 2> _velocityDescriptorSet{};
    std::map<Quantity, std::array<VkDescriptorSet, 2>> _quantityDescriptorSets;
    std::map<Quantity, int> _quantityIn;
    std::map<Quantity, int> _quantityOut;
    std::map<Quantity, std::array<VulkanBuffer, 2>> _quantityBuffer;
    std::array<VulkanBuffer, 2> _u, _v;
    VulkanSampler textureSampler;

    int q_in = 0;
    int q_out = 1;
    int v_in = 0;
    int v_out = 1;
    int color_in = 0;
    int color_out = 1;

    struct {
        int N;
        float dt{1};
        float dissipation{1};
    } _constants{};

    struct {
        int N;
        int boundary{0};
    } _boundaryConstants;

    struct {
        int N;
        float alpha{1};
        float rBeta{1};
    } jacobiConstants;

    std::array<int, 2> sc{};
    FileManager _fileManager;
    std::array<Texture, 2> scratchColors;
    Texture* _outputColor{nullptr};
};