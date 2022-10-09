#include "fluid_solver_2d.h"
#include "Vertex.h"
#include "GraphicsPipelineBuilder.hpp"
#include "DescriptorSetBuilder.hpp"
#include <spdlog/spdlog.h>

FluidSolver2D::FluidSolver2D(VulkanDevice* device, VulkanDescriptorPool* descriptorPool, VulkanRenderPass* displayRenderPass, FileManager *fileManager, glm::vec2 gridSize)
: FluidSolver(device, descriptorPool, displayRenderPass, fileManager, {gridSize.x, gridSize.y, 1})
, gridSize(gridSize)
, delta(1.0f/gridSize)
{
    globalConstants.dx.x = delta.x;
    globalConstants.dy.y = delta.y;
    fileManager->addSearchPath("../../data/shaders/fluid_2d");
}

void FluidSolver2D::init() {
    initBuffers();
    createSamplers();
    initViewVectors();
    createRenderPass();
    initSimData();
    initFullScreenQuad();
    createDescriptorSetLayouts();
    updateDescriptorSets();
    createPipelines();
}

void FluidSolver2D::set(VulkanBuffer vectorFieldBuffer) {

    device->graphicsCommandPool().oneTimeCommand([&](auto commandBuffer){
        vectorField.texture[in].image.transitionLayout(commandBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, DEFAULT_SUB_RANGE
                , VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_TRANSFER_WRITE_BIT
                , VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;

        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = {0, 0, 0};
        region.imageExtent = {width, height, 1u};

        vkCmdCopyBufferToImage(commandBuffer, vectorFieldBuffer, vectorField.texture[in].image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                               &region);

        vectorField.texture[in].image.transitionLayout(commandBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, DEFAULT_SUB_RANGE
                , VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT
                , VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    });
}

void FluidSolver2D::add(ExternalForce &&force) {
    externalForces.push_back(force);
}

void FluidSolver2D::runSimulation(VkCommandBuffer commandBuffer) {
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, screenQuad.vertices, &offset);
    velocityStep(commandBuffer);
    quantityStep(commandBuffer);
    _elapsedTime += timeStep;
}

void FluidSolver2D::velocityStep(VkCommandBuffer commandBuffer) {
    if(!options.advectVField) return;

    clearForces(commandBuffer);
    applyForces(commandBuffer);
    if(options.viscosity > MIN_FLOAT) {
        jacobi.constants.isVectorField = 1;
        diffuse(commandBuffer, vectorField, options.viscosity);
        project(commandBuffer);
    }
    advectVectorField(commandBuffer);
    project(commandBuffer);
}

void FluidSolver2D::quantityStep(VkCommandBuffer commandBuffer) {
    for(auto& quantity : quantities){
        quantityStep(commandBuffer, quantity);
    }
}

void FluidSolver2D::quantityStep(VkCommandBuffer commandBuffer, Quantity &quantity) {
    clearSources(commandBuffer, quantity);
    updateSources(commandBuffer, quantity);
    addSource(commandBuffer, quantity);
    diffuseQuantity(commandBuffer, quantity);
    advectQuantity(commandBuffer, quantity);
    postAdvection(commandBuffer, quantity);
}

void FluidSolver2D::clearSources(VkCommandBuffer commandBuffer, Quantity &quantity) {
    clear(commandBuffer, quantity.source.texture[in]);
    clear(commandBuffer, quantity.source.texture[out]);
}

void FluidSolver2D::updateSources(VkCommandBuffer commandBuffer, Quantity &quantity) {
    quantity.update(commandBuffer, quantity.source);
}

void FluidSolver2D::addSource(VkCommandBuffer commandBuffer, Quantity &quantity) {
    addSources(commandBuffer, quantity.source, quantity.field);
}

void FluidSolver2D::diffuseQuantity(VkCommandBuffer commandBuffer, Quantity &quantity) {
    jacobi.constants.isVectorField = 0;
    diffuse(commandBuffer, quantity.field, quantity.diffuseRate);
}

void FluidSolver2D::advectQuantity(VkCommandBuffer commandBuffer, Quantity &quantity) {
    static std::array<VkDescriptorSet, 2> sets;
    sets[0] = vectorField.descriptorSet[in];
    sets[1] = quantity.field.advectDescriptorSet[in];

    advect(commandBuffer, sets,quantity.field.framebuffer[out]);
    quantity.field.swap();
}

void FluidSolver2D::postAdvection(VkCommandBuffer commandBuffer, Quantity &quantity) {
    withRenderPass(commandBuffer, quantity.field.framebuffer[out], [&](VkCommandBuffer commandBuffer){
       auto postAction = quantity.postAdvect(commandBuffer, quantity.field);
       if(postAction){
           quantity.field.swap();
       }
    });
}

void FluidSolver2D::applyForces(VkCommandBuffer commandBuffer) {
    applyExternalForces(commandBuffer);
    computeVorticityConfinement(commandBuffer);
    addSources(commandBuffer, forceField, vectorField);
}

void FluidSolver2D::applyExternalForces(VkCommandBuffer commandBuffer) {
    for(const auto& externalForce : externalForces){
        withRenderPass(commandBuffer, forceField.framebuffer[out], [&](auto commandBuffer){
            externalForce(commandBuffer, forceField.descriptorSet[in]);
        });
        forceField.swap();
    }
}

void FluidSolver2D::computeVorticityConfinement(VkCommandBuffer commandBuffer) {
    if(!options.vorticity) return;
    withRenderPass(commandBuffer, vorticityField.framebuffer[0], [&](auto commandBuffer){
        static std::array<VkDescriptorSet, 2> sets;
        sets[0] = globalConstantsDescriptorSet;
        sets[1] = vectorField.descriptorSet[in];
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vorticity.pipeline);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vorticity.layout, 0, COUNT(sets), sets.data(), 0, VK_NULL_HANDLE);
        vkCmdDraw(commandBuffer, 4, 1, 0, 0);
    });
    withRenderPass(commandBuffer, forceField.framebuffer[out], [&](auto commandBuffer){
        static std::array<VkDescriptorSet, 3> sets;
        sets[0] = globalConstantsDescriptorSet;
        sets[1] = vorticityField.descriptorSet[in];
        sets[2] = forceField.descriptorSet[in];
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vorticityForce.pipeline);
        vkCmdPushConstants(commandBuffer, vorticityForce.layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(vorticityForce.constants), &vorticityForce.constants);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vorticityForce.layout, 0, COUNT(sets), sets.data(), 0, VK_NULL_HANDLE);
        vkCmdDraw(commandBuffer, 4, 1, 0, 0);
    });
    forceField.swap();
}

void FluidSolver2D::clearForces(VkCommandBuffer commandBuffer) {
    clear(commandBuffer, forceField.texture[in]);
    clear(commandBuffer, forceField.texture[out]);
}

void FluidSolver2D::clear(VkCommandBuffer commandBuffer, Texture &texture) {
    texture.image.transitionLayout(commandBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, DEFAULT_SUB_RANGE
            , VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_TRANSFER_WRITE_BIT
            , VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

    VkClearColorValue color{ {0.f, 0.f, 0.f, 0.f}};
    VkImageSubresourceRange range{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    vkCmdClearColorImage(commandBuffer, texture.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &color, 1, &range);

    texture.image.transitionLayout(commandBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, DEFAULT_SUB_RANGE
            , VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT
            , VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
}

void FluidSolver2D::addSources(VkCommandBuffer commandBuffer, Field &sourceField, Field &destinationField) {
    addSourcePipeline.constants.dt = globalConstants.dt;
    withRenderPass(commandBuffer, destinationField.framebuffer[out], [&](auto commandBuffer){
        static std::array<VkDescriptorSet, 2> sets;
        sets[0] = sourceField.descriptorSet[in];
        sets[1] = destinationField.descriptorSet[in];
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, addSourcePipeline.pipeline);
        vkCmdPushConstants(commandBuffer, addSourcePipeline.layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(addSourcePipeline.constants), &addSourcePipeline.constants);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, addSourcePipeline.layout
                , 0, COUNT(sets), sets.data(), 0, VK_NULL_HANDLE);
        vkCmdDraw(commandBuffer, 4, 1, 0, 0);
    });
    destinationField.swap();
}

void FluidSolver2D::advectVectorField(VkCommandBuffer commandBuffer) {
    static std::array<VkDescriptorSet, 2> sets;
    sets[0] = vectorField.descriptorSet[in];
    sets[1] = vectorField.advectDescriptorSet[in];

    advect(commandBuffer, sets,vectorField.framebuffer[out]);
    vectorField.swap();
}

void FluidSolver2D::advect(VkCommandBuffer commandBuffer, const std::array<VkDescriptorSet, 2> &inSets,
                           VulkanFramebuffer &framebuffer) {


    static std::array<VkDescriptorSet, 4> sets;
    sets[0] = globalConstantsDescriptorSet;
    sets[1] = inSets[0];
    sets[2] = inSets[1];
    sets[3] = samplerDescriptorSet;
    withRenderPass(commandBuffer, framebuffer, [&](auto commandBuffer){
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, advectPipeline.pipeline);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, advectPipeline.layout
                , 0, COUNT(sets), sets.data(), 0, VK_NULL_HANDLE);

        vkCmdDraw(commandBuffer, 4, 1, 0, 0);
    });
}

void FluidSolver2D::project(VkCommandBuffer commandBuffer) {
    if(!options.project) return;
    computeDivergence(commandBuffer);
    solvePressure(commandBuffer);
    computeDivergenceFreeField(commandBuffer);
    vectorField.swap();
}

void FluidSolver2D::computeDivergence(VkCommandBuffer commandBuffer) {
    static std::array<VkDescriptorSet, 2> sets;
    sets[0] = globalConstantsDescriptorSet;
    sets[1] = vectorField.descriptorSet[in];
    withRenderPass(commandBuffer, divergenceField.framebuffer[0], [&](auto commandBuffer){
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, divergence.pipeline);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, divergence.layout
                , 0, COUNT(sets), sets.data(), 0, VK_NULL_HANDLE);

        vkCmdDraw(commandBuffer, 4, 1, 0, 0);
    });
}

void FluidSolver2D::solvePressure(VkCommandBuffer commandBuffer) {
    auto alpha = -delta.x * delta.x * delta.y * delta.y;
    auto rBeta = (1.0f/(2.0f * glm::dot(delta, delta)));
    for(int i = 0; i < options.poissonIterations; i++){
        withRenderPass(commandBuffer, pressureField.framebuffer[out], [&](auto commandBuffer){
            jacobiIteration(commandBuffer, pressureField.descriptorSet[in], divergenceField.descriptorSet[in], alpha, rBeta);
            pressureField.swap();
        });
    }
}

void FluidSolver2D::jacobiIteration(VkCommandBuffer commandBuffer, VkDescriptorSet unknownDescriptor,
                                    VkDescriptorSet solutionDescriptor, float alpha, float rBeta) {
    jacobi.constants.alpha = alpha;
    jacobi.constants.rBeta = rBeta;
    static std::array<VkDescriptorSet, 3> sets;
    sets[0] = globalConstantsDescriptorSet;
    sets[1] = solutionDescriptor;
    sets[2] = unknownDescriptor;
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, jacobi.pipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, jacobi.layout
            , 0, COUNT(sets), sets.data(), 0, VK_NULL_HANDLE);

    vkCmdPushConstants(commandBuffer, jacobi.layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(jacobi.constants), &jacobi.constants);
    vkCmdDraw(commandBuffer, 4, 1, 0, 0);
}

void FluidSolver2D::computeDivergenceFreeField(VkCommandBuffer commandBuffer) {
    withRenderPass(commandBuffer, vectorField.framebuffer[out], [&](auto commandBuffer){
        static std::array<VkDescriptorSet, 3> sets;
        sets[0] = globalConstantsDescriptorSet;
        sets[1] = vectorField.descriptorSet[in];
        sets[2] = pressureField.descriptorSet[in];
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, divergenceFree.pipeline);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, divergenceFree.layout
                , 0, COUNT(sets), sets.data(), 0, VK_NULL_HANDLE);

        vkCmdDraw(commandBuffer, 4, 1, 0, 0);
    });
}

void FluidSolver2D::diffuse(VkCommandBuffer commandBuffer, Field &field, float rate) {
    if(rate <= MIN_FLOAT) return;
    diffuseHelper.texture.image.transitionLayout(commandBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, DEFAULT_SUB_RANGE
            , VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_TRANSFER_WRITE_BIT
            , VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

    field.texture[in].image.transitionLayout(commandBuffer, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, DEFAULT_SUB_RANGE
            , VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT
            , VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

    VkImageSubresourceLayers subresourceLayers{VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    VkExtent3D extent3D{ static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1};
    VkImageCopy pRegion{subresourceLayers, {0, 0, 0}, subresourceLayers, {0, 0, 0}, extent3D};

    vkCmdCopyImage(commandBuffer, field.texture[in].image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
            , diffuseHelper.texture.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &pRegion);

    field.texture[in].image.transitionLayout(commandBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, DEFAULT_SUB_RANGE
            , VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_SHADER_READ_BIT
            , VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

    diffuseHelper.texture.image.transitionLayout(commandBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, DEFAULT_SUB_RANGE
            , VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT
            , VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);


    float alpha = (delta.x * delta.x * delta.x * delta.y)/(timeStep * rate);
    float rBeta = 1.0f/((2.0f * glm::dot(delta, delta)) + alpha);
    for(int i = 0; i < options.poissonIterations; i++){
        withRenderPass(commandBuffer, field.framebuffer[out], [&](auto commandBuffer){
            jacobiIteration(commandBuffer, field.descriptorSet[in], diffuseHelper.solutionDescriptorSet, alpha, rBeta);
            field.swap();
        });
    }
}


void FluidSolver2D::renderVectorField(VkCommandBuffer commandBuffer) {
    if(!options.showArrows) return;
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, arrows.vertexBuffer, &offset);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, arrows.pipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, arrows.layout
            , 0, 1, &vectorField.descriptorSet[in], 0
            , VK_NULL_HANDLE);

    vkCmdDraw(commandBuffer, arrows.numArrows, 1, 0, 0);
}

void FluidSolver2D::add(Quantity& quantity) {
    createFrameBuffer(quantity);
    createDescriptorSets(quantity);
    quantities.emplace_back(quantity);
}

void FluidSolver2D::createFrameBuffer(Quantity &quantity) {
    for(auto i = 0; i < 2; i++){
        quantity.field.framebuffer[i] = device->createFramebuffer(renderPass, {quantity.field.texture[i].imageView }, width, height);
        device->setName<VK_OBJECT_TYPE_FRAMEBUFFER>(fmt::format("{}_{}", quantity.name, i), quantity.field.framebuffer[i].frameBuffer);

        quantity.source.framebuffer[i] = device->createFramebuffer(renderPass, {quantity.source.texture[i].imageView }, width, height);
        device->setName<VK_OBJECT_TYPE_FRAMEBUFFER>(fmt::format("{}_source_{}", quantity.name, i), quantity.field.framebuffer[i].frameBuffer);
    }
}

void FluidSolver2D::dt(float value) {
    timeStep = value;
    globalConstants.dt = timeStep;
    updateUBOs();
}

float FluidSolver2D::dt() const {
    return timeStep;
}

void FluidSolver2D::advectVelocity(bool flag) {
    options.advectVField = flag;
}

void FluidSolver2D::project(bool flag) {
    options.project = flag;
}

void FluidSolver2D::showVectors(bool flag) {
    options.showArrows = flag;
}

void FluidSolver2D::applyVorticity(bool flag) {
    options.vorticity = flag;
}

void FluidSolver2D::poissonIterations(int value) {
    options.poissonIterations = value;
}

void FluidSolver2D::viscosity(float value) {
    options.viscosity = value;
}

void FluidSolver2D::updateUBOs() {
    globalConstantsBuffer.copy(&globalConstants, sizeof(globalConstants));
}

void FluidSolver2D::ensureBoundaryCondition(bool flag) {
    globalConstants.ensureBoundaryCondition = static_cast<int>(flag);
    updateUBOs();
}

float FluidSolver2D::elapsedTime() {
    return _elapsedTime;
}

std::tuple<VkDeviceSize, void *> FluidSolver2D::getGlobalConstants() {
    return std::make_tuple(sizeof(globalConstants), &globalConstants);
}
