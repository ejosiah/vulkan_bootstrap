#include "Smoke2D.hpp"
#include "GraphicsPipelineBuilder.hpp"
#include "DescriptorSetBuilder.hpp"
#include "gpu/algorithm.h"

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
}

void Smoke2D::initApp() {
    initAmbientTempBuffer();
    initFullScreenQuad();
    createDescriptorPool();
    createDescriptorSet();
    initSolver();
    updateDescriptorSets();
    createCommandPool();
    createPipelineCache();
    createRenderPipeline();
    createComputePipeline();
}

void Smoke2D::initAmbientTempBuffer() {
    auto temp = AMBIENT_TEMP;
    ambientTempBuffer = device.createCpuVisibleBuffer(&temp, sizeof(float), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    tempField = device.createBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
            , VMA_MEMORY_USAGE_CPU_TO_GPU, width * height * sizeof(float));
    debugBuffer = device.createStagingBuffer(sizeof(glm::vec4) * width * height);
    ambientTemp = reinterpret_cast<float*>(ambientTempBuffer.map());
    temps = reinterpret_cast<float*>(tempField.map());
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

void Smoke2D::createDescriptorSet() {
    ambientTempSet =
        device.descriptorSetLayoutBuilder()
            .binding(0)
                .descriptorType(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
                .descriptorCount(1)
                .shaderStages(VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)
            .binding(1)
                .descriptorType(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
                .descriptorCount(1)
                .shaderStages(VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)
        .createLayout();

    ambientTempDescriptorSet = descriptorPool.allocate({ ambientTempSet }).front();
}

void Smoke2D::updateDescriptorSets() {
    auto writes = initializers::writeDescriptorSets<2>();

    writes[0].dstSet = ambientTempDescriptorSet;
    writes[0].dstBinding = 0;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[0].descriptorCount = 1;
    VkDescriptorBufferInfo inInfo{ tempField, 0, VK_WHOLE_SIZE};
    writes[0].pBufferInfo = &inInfo;

    writes[1].dstSet = ambientTempDescriptorSet;
    writes[1].dstBinding = 1;
    writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[1].descriptorCount = 1;
    VkDescriptorBufferInfo outInfo{ambientTempBuffer, 0, VK_WHOLE_SIZE};
    writes[1].pBufferInfo = &outInfo;

    device.updateDescriptorSets(writes);

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
                .addDescriptorSetLayouts({fluidSolver.textureSetLayout, fluidSolver.textureSetLayout , ambientTempSet})
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
    auto module = VulkanShaderModule{ resource("temp_image_buffer.comp.spv"), device};
    auto stage = initializers::shaderStage({ module, VK_SHADER_STAGE_COMPUTE_BIT});

    compute.layout = device.createPipelineLayout({fluidSolver.textureSetLayout, ambientTempSet});

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
    auto title = fmt::format("{}, temperature {:.3f}, fps {}", this->title, *ambientTemp, framePerSecond);
    glfwSetWindowTitle(window, title.c_str());
    device.graphicsCommandPool().oneTimeCommand([&](auto commandBuffer){
       fluidSolver.runSimulation(commandBuffer);
    });
}

void Smoke2D::checkAppInputs() {
    if(mouse.left.released){
        static Camera camera{};
        glm::vec2 pos = mousePositionToWorldSpace(camera);
        pos.y *= -1;
        pos = .5f * pos + .5f;
        spdlog::info("cam pos: {}", pos);
        device.graphicsCommandPool().oneTimeCommand([&](auto commandBuffer){
           temperatureAndDensity.source.texture[in].image.copyToBuffer(commandBuffer, debugBuffer);
        });
        auto debug = reinterpret_cast<glm::vec2*>(debugBuffer.map());
        for(int i = 0; i < height; i++){
            for(int j = 0; j < width; j++){
                int index = i * width + j;
                auto value = debug[index];
                if(value.x > 0){
                   spdlog::info("temp at [{}, {}] => {}", j, i, value.x);
                }
            }
        }
        debugBuffer.unmap();
    }
}

void Smoke2D::cleanup() {
    ambientTempBuffer.unmap();
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
    emitter.constants.time = fluidSolver.elapsedTime();
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
        static std::array<VkDescriptorSet, 3> sets;
        sets[0] = descriptorSet;
        sets[1] = temperatureAndDensity.field.descriptorSet[in];
        sets[2] = ambientTempDescriptorSet;
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, buoyancyForceGen.pipeline);
        vkCmdPushConstants(commandBuffer, buoyancyForceGen.layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(buoyancyForceGen.constants), &buoyancyForceGen.constants);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, buoyancyForceGen.layout, 0, COUNT(sets), sets.data(), 0, VK_NULL_HANDLE);
        vkCmdDraw(commandBuffer, 4, 1, 0, 0);
    };
}

void Smoke2D::copy(VkCommandBuffer commandBuffer, Texture &source, const VulkanBuffer& destination) {
    static std::array<VkDescriptorSet, 2> sets;
    sets[0] = temperatureAndDensity.field.descriptorSet[in];
    sets[1] = ambientTempDescriptorSet;
    addImageMemoryBarriers(commandBuffer, { source.image });
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, compute.pipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, compute.layout,
                            0, COUNT(sets), sets.data(), 0, VK_NULL_HANDLE);

    uint32_t groupCountX = glm::min(1, width/32);
    uint32_t groupCountY = glm::min(1, height/32);
    vkCmdDispatch(commandBuffer, groupCountX, groupCountY, 1);
    addBufferMemoryBarriers(commandBuffer, { destination });
}


int main(){
    try{

        Settings settings;
        settings.width = 600;
        settings.height = 1000;

        auto app = Smoke2D{ settings };
        app.run();
    }catch(std::runtime_error& err){
        spdlog::error(err.what());
    }
}