#pragma once
#include "fluid_solver_common.h"
#include "VulkanDevice.h"
#include "VulkanRenderPass.h"
#include "VulkanFramebuffer.h"
#include "VulkanRAII.h"
#include "filemanager.hpp"
#include "glm/glm.hpp"

class FluidSolver2D : public FluidSolver{
public:
    FluidSolver2D() = default;

    FluidSolver2D(VulkanDevice* device, VulkanDescriptorPool* descriptorPool, VulkanRenderPass* displayRenderPass, FileManager* fileManager, glm::vec2 gridSize);

    void init();

protected:
    std::tuple<VkDeviceSize, void *> getGlobalConstants() override;

public:

    void set(VulkanBuffer vectorFieldBuffer);

    void add(ExternalForce&& force);

    void runSimulation(VkCommandBuffer commandBuffer);

    void updateUBOs();

    void velocityStep(VkCommandBuffer commandBuffer);

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

    void ensureBoundaryCondition(bool flag);

    float elapsedTime();

protected:
    void quantityStep(VkCommandBuffer commandBuffer);

    void quantityStep(VkCommandBuffer commandBuffer, Quantity& quantity);

    void clearSources(VkCommandBuffer commandBuffer, Quantity &quantity);

    void updateSources(VkCommandBuffer commandBuffer, Quantity &quantity);

    void addSource(VkCommandBuffer commandBuffer, Quantity &quantity);

    void diffuseQuantity(VkCommandBuffer commandBuffer, Quantity &quantity);

    void advectQuantity(VkCommandBuffer commandBuffer, Quantity &quantity);

    void postAdvection(VkCommandBuffer commandBuffer, Quantity &quantity);

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
    glm::vec2 gridSize{};
    glm::vec2 delta{};
    struct {
        glm::vec2 dx{0};
        glm::vec2 dy{0};
        float dt{1.0f / 120.f};
        int ensureBoundaryCondition{1};
    } globalConstants;

    struct {
        bool advectVField = true;
        bool project = true;
        bool showArrows = false;
        bool vorticity = true;
        int poissonIterations = 30;
        float viscosity = MIN_FLOAT;
    } options;
};