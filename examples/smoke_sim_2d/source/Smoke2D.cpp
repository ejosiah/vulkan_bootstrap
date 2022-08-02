#include "Smoke2D.hpp"
#include "GraphicsPipelineBuilder.hpp"
#include "DescriptorSetBuilder.hpp"

Smoke2D::Smoke2D(const Settings& settings) : VulkanBaseApp("2D Smoke Simulation", settings) {
    fileManager.addSearchPath(".");
    fileManager.addSearchPath("../../examples/smoke_sim_2d");
    fileManager.addSearchPath("../../examples/smoke_sim_2d/spv");
    fileManager.addSearchPath("../../examples/smoke_sim_2d/models");
    fileManager.addSearchPath("../../examples/smoke_sim_2d/textures");
    fileManager.addSearchPath("../../data/shaders");
    fileManager.addSearchPath("../../data/models");
    fileManager.addSearchPath("../../data/textures");
    fileManager.addSearchPath("../../data");
    fileManager.addSearchPath("../../data/shaders/fluid_2d");
}

void Smoke2D::initApp() {
    initFullScreenQuad();
    createDescriptorPool();
    initSolver();
    createCommandPool();
    createPipelineCache();
    createRenderPipeline();
    createComputePipeline();
}

void Smoke2D::initFullScreenQuad() {
    auto quad = ClipSpace::Quad::positions;
    screenQuad = device.createDeviceLocalBuffer(quad.data(), BYTE_SIZE(quad), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
}

void Smoke2D::createDescriptorPool() {
    constexpr uint32_t maxSets = 100;
    std::array<VkDescriptorPoolSize, 16> poolSizes{
            {
                    {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 100 * maxSets},
                    {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 100 * maxSets},
                    {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 100 * maxSets},
                    { VK_DESCRIPTOR_TYPE_SAMPLER, 100 * maxSets },
                    { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 100 * maxSets },
                    { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 100 * maxSets },
                    { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 100 * maxSets },
                    { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 100 * maxSets },
                    { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 100 * maxSets },
                    { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 100 * maxSets },
                    { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 100 * maxSets },
                    { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 100 * maxSets },
                    { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 100 * maxSets },
                    { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 100 * maxSets },
                    { VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK_EXT, 100 * maxSets },
                    { VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 100 * maxSets }
            }
    };
    descriptorPool = device.createDescriptorPool(maxSets, poolSizes, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT);

}

void Smoke2D::createCommandPool() {
    commandPool = device.createCommandPool(*device.queueFamilyIndex.graphics, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    commandBuffers = commandPool.allocateCommandBuffers(swapChainImageCount);
}

void Smoke2D::createPipelineCache() {
    pipelineCache = device.createPipelineCache();
}


void Smoke2D::createRenderPipeline() {
    //    @formatter:off
    auto builder = device.graphicsPipelineBuilder();
    temperatureRender.pipeline =
        builder
            .shaderStage()
                .vertexShader(resource("quad.vert.spv"))
                .fragmentShader(resource("temperature.frag.spv"))
            .vertexInputState()
                .addVertexBindingDescriptions(ClipSpace::bindingDescription())
                .addVertexAttributeDescriptions(ClipSpace::attributeDescriptions())
            .inputAssemblyState()
                .triangleStrip()
            .viewportState()
                .viewport()
                    .origin(0, 0)
                    .dimension(swapChain.extent)
                    .minDepth(0)
                    .maxDepth(1)
                .scissor()
                    .offset(0, 0)
                    .extent(swapChain.extent)
                .add()
                .rasterizationState()
                    .cullBackFace()
                    .frontFaceCounterClockwise()
                    .polygonModeFill()
                .multisampleState()
                    .rasterizationSamples(settings.msaaSamples)
                .depthStencilState()
                    .enableDepthWrite()
                    .enableDepthTest()
                    .compareOpLess()
                    .minDepthBounds(0)
                    .maxDepthBounds(1)
                .colorBlendState()
                    .attachment()
                    .add()
                .layout()
                    .addDescriptorSetLayout(fluidSolver.textureSetLayout)
                    .addPushConstantRange(VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(temperatureRender.constants))
                .renderPass(renderPass)
                .subpass(0)
                .name("temperature_render")
                .pipelineCache(pipelineCache)
            .build(temperatureRender.layout);

    smokeRender.pipeline =
        builder
            .shaderStage()
                .fragmentShader(resource("smoke_render.frag.spv"))
            .layout().clear()
                .addDescriptorSetLayouts({fluidSolver.textureSetLayout})
                .addPushConstantRange(VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(smokeRender.constants))
            .name("smoke_render")
        .build(smokeRender.layout);

    emitter.pipeline =
        builder
            .shaderStage()
                .fragmentShader(resource("smoke_source.frag.spv"))
            .colorBlendState()
                .attachments(1)
            .layout().clear()
                .addDescriptorSetLayout(fluidSolver.textureSetLayout)
                .addPushConstantRange(VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(emitter.constants))
            .renderPass(fluidSolver.renderPass)
            .name("smoke_emitter")
        .build(emitter.layout);

    buoyancyForceGen.pipeline =
        builder
            .shaderStage()
                .fragmentShader(resource("buoyancy_force.frag.spv"))
            .layout().clear()
                .addDescriptorSetLayouts({fluidSolver.textureSetLayout, fluidSolver.textureSetLayout})
                .addPushConstantRange(VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(buoyancyForceGen.constants))
            .renderPass(fluidSolver.renderPass)
            .name("buoyancy_force")
        .build(buoyancyForceGen.layout);

    smokeDecay.pipeline =
        builder
            .shaderStage()
                .fragmentShader(resource("decay_smoke.frag.spv"))
            .layout().clear()
                .addDescriptorSetLayouts({fluidSolver.textureSetLayout})
                .addPushConstantRange(VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(smokeDecay.constants))
            .renderPass(fluidSolver.renderPass)
            .name("smoke_decay")
        .build(smokeDecay.layout);
    //    @formatter:on
}

void Smoke2D::createComputePipeline() {
    auto module = VulkanShaderModule{ "../../data/shaders/pass_through.comp.spv", device};
    auto stage = initializers::shaderStage({ module, VK_SHADER_STAGE_COMPUTE_BIT});

    compute.layout = device.createPipelineLayout();

    auto computeCreateInfo = initializers::computePipelineCreateInfo();
    computeCreateInfo.stage = stage;
    computeCreateInfo.layout = compute.layout;

    compute.pipeline = device.createComputePipeline(computeCreateInfo, pipelineCache);
}


void Smoke2D::onSwapChainDispose() {
    dispose(render.pipeline);
    dispose(compute.pipeline);
}

void Smoke2D::onSwapChainRecreation() {
    createRenderPipeline();
    createComputePipeline();
}

VkCommandBuffer *Smoke2D::buildCommandBuffers(uint32_t imageIndex, uint32_t &numCommandBuffers) {
    numCommandBuffers = 1;
    auto& commandBuffer = commandBuffers[imageIndex];

    VkCommandBufferBeginInfo beginInfo = initializers::commandBufferBeginInfo();
    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    static std::array<VkClearValue, 2> clearValues;
    clearValues[0].color = {1, 1, 1, 1};
    clearValues[1].depthStencil = {1.0, 0u};

    VkRenderPassBeginInfo rPassInfo = initializers::renderPassBeginInfo();
    rPassInfo.clearValueCount = COUNT(clearValues);
    rPassInfo.pClearValues = clearValues.data();
    rPassInfo.framebuffer = framebuffers[imageIndex];
    rPassInfo.renderArea.offset = {0u, 0u};
    rPassInfo.renderArea.extent = swapChain.extent;
    rPassInfo.renderPass = renderPass;

    vkCmdBeginRenderPass(commandBuffer, &rPassInfo, VK_SUBPASS_CONTENTS_INLINE);

//    fluidSolver.renderVectorField(commandBuffer);
//    renderSource(commandBuffer);
    renderSmoke(commandBuffer);
//    renderTemperature(commandBuffer);

    vkCmdEndRenderPass(commandBuffer);

    vkEndCommandBuffer(commandBuffer);

    return &commandBuffer;
}

void Smoke2D::renderTemperature(VkCommandBuffer commandBuffer) {
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, screenQuad, &offset);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, temperatureRender.pipeline);
    vkCmdPushConstants(commandBuffer, temperatureRender.layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(temperatureRender.constants), &temperatureRender.constants);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, temperatureRender.layout
            , 0, 1, &temperatureAndDensity.field.descriptorSet[in], 0
            , VK_NULL_HANDLE);

    vkCmdDraw(commandBuffer, 4, 1, 0, 0);
}

void Smoke2D::renderSmoke(VkCommandBuffer commandBuffer) {
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, screenQuad, &offset);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, smokeRender.pipeline);
    vkCmdPushConstants(commandBuffer, smokeRender.layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(smokeRender.constants), &smokeRender.constants);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, smokeRender.layout
            , 0, 1, &temperatureAndDensity.field.descriptorSet[in], 0
            , VK_NULL_HANDLE);

    vkCmdDraw(commandBuffer, 4, 1, 0, 0);
}

void Smoke2D::renderSource(VkCommandBuffer commandBuffer) {
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, screenQuad, &offset);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, temperatureRender.pipeline);
    vkCmdPushConstants(commandBuffer, temperatureRender.layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(temperatureRender.constants), &temperatureRender.constants);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, temperatureRender.layout
            , 0, 1, &temperatureAndDensity.source.descriptorSet[in], 0
            , VK_NULL_HANDLE);

    vkCmdDraw(commandBuffer, 4, 1, 0, 0);
}

void Smoke2D::update(float time) {
    auto title = fmt::format("{} - fps {}", this->title, framePerSecond);
    glfwSetWindowTitle(window, title.c_str());
    device.graphicsCommandPool().oneTimeCommand([&](auto commandBuffer){
       fluidSolver.runSimulation(commandBuffer);
    });
}

void Smoke2D::checkAppInputs() {
    VulkanBaseApp::checkAppInputs();
}

void Smoke2D::cleanup() {
    // TODO save pipeline cache
    VulkanBaseApp::cleanup();
}

void Smoke2D::onPause() {
    VulkanBaseApp::onPause();
}

void Smoke2D::initTemperatureAndDensityField() {
    std::vector<glm::vec4> field(width * height, {AMBIENT_TEMP, 0, 0, 0});

    textures::create(device, temperatureAndDensity.field.texture[0], VK_IMAGE_TYPE_2D, VK_FORMAT_R32G32B32A32_SFLOAT
            , field.data(), {width, height, 1}, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE
            , sizeof(float));
    textures::create(device, temperatureAndDensity.field.texture[1], VK_IMAGE_TYPE_2D, VK_FORMAT_R32G32B32A32_SFLOAT
            , field.data(), {width, height, 1}, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE
            , sizeof(float));

    std::vector<glm::vec4> allocation(width * height);
    textures::create(device, temperatureAndDensity.source.texture[0], VK_IMAGE_TYPE_2D, VK_FORMAT_R32G32B32A32_SFLOAT
            , allocation.data(), {width, height, 1}, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE
            , sizeof(float));
    textures::create(device, temperatureAndDensity.source.texture[1], VK_IMAGE_TYPE_2D, VK_FORMAT_R32G32B32A32_SFLOAT
            , allocation.data(), {width, height, 1}, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE
            , sizeof(float));

    temperatureAndDensity.name = "temperature_and_density";
    temperatureAndDensity.update = [&](VkCommandBuffer commandBuffer, Field& field){
        emitSmoke(commandBuffer, field);
    };
    temperatureAndDensity.postAdvect = [&](VkCommandBuffer commandBuffer, Field& field){
        return decaySmoke(commandBuffer, field);
    };

}

void Smoke2D::initSolver() {
    initTemperatureAndDensityField();
    fluidSolver = FluidSolver2D{&device, &descriptorPool, &renderPass, &fileManager, {width, height}};
    fluidSolver.init();
    fluidSolver.showVectors(true);
    fluidSolver.applyVorticity(true);
    fluidSolver.add(temperatureAndDensity);
    fluidSolver.add(buoyancyForce());
}

void Smoke2D::emitSmoke(VkCommandBuffer commandBuffer, Field &field) {
    emitter.constants.dt = fluidSolver.dt();
    fluidSolver.withRenderPass(commandBuffer, field.framebuffer[out], [&](auto commandBuffer){
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, emitter.pipeline);
        vkCmdPushConstants(commandBuffer, emitter.layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(emitter.constants), &emitter.constants);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, emitter.layout, 0, 1, &temperatureAndDensity.field.descriptorSet[in], 0, VK_NULL_HANDLE);
        vkCmdDraw(commandBuffer, 4, 1, 0, 0);
    });
    field.swap();
}

bool Smoke2D::decaySmoke(VkCommandBuffer commandBuffer, Field &field) {
    smokeDecay.constants.dt = fluidSolver.dt();
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, smokeDecay.pipeline);
    vkCmdPushConstants(commandBuffer, smokeDecay.layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(smokeDecay.constants), &smokeDecay.constants);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, smokeDecay.layout, 0, 1, &field.descriptorSet[in], 0, VK_NULL_HANDLE);
    vkCmdDraw(commandBuffer, 4, 1, 0, 0);
    return true;
}


ExternalForce Smoke2D::buoyancyForce() {
    return [&](VkCommandBuffer commandBuffer, VkDescriptorSet descriptorSet){
        static std::array<VkDescriptorSet, 2> sets;
        sets[0] = descriptorSet;
        sets[1] = temperatureAndDensity.field.descriptorSet[in];
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, buoyancyForceGen.pipeline);
        vkCmdPushConstants(commandBuffer, buoyancyForceGen.layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(buoyancyForceGen.constants), &buoyancyForceGen.constants);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, buoyancyForceGen.layout, 0, COUNT(sets), sets.data(), 0, VK_NULL_HANDLE);
        vkCmdDraw(commandBuffer, 4, 1, 0, 0);
    };
}


int main(){
    try{

        Settings settings;
        settings.width = 600;
        settings.height = 1000;
        settings.depthTest = true;

        auto app = Smoke2D{ settings };
        app.run();
    }catch(std::runtime_error& err){
        spdlog::error(err.what());
    }
}