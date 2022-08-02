#include "FluidDynamicsDemo.hpp"
#include "GraphicsPipelineBuilder.hpp"
#include "DescriptorSetBuilder.hpp"
#include "fluid_dynamics_cpu.h"
#include "ImGuiPlugin.hpp"

FluidDynamicsDemo::FluidDynamicsDemo(const Settings& settings) : VulkanBaseApp("Real-Time Fluid Dynamics", settings) {
    fileManager.addSearchPath(".");
    fileManager.addSearchPath("../../examples/fluid_dynamics");
    fileManager.addSearchPath("../../examples/fluid_dynamics/spv");
    fileManager.addSearchPath("../../examples/fluid_dynamics/models");
    fileManager.addSearchPath("../../examples/fluid_dynamics/textures");
    fileManager.addSearchPath("../../data/shaders");
    fileManager.addSearchPath("../../data/models");
    fileManager.addSearchPath("../../data/textures");
    fileManager.addSearchPath("../../data");
}

void FluidDynamicsDemo::initApp() {
    initGrid();
    initVectorRender();
    initDensityRender();
    initRenderBuffer();
    initSimData();
    initParticles();
    initCamera();
    initBrush();
    createUVTexture();
    initFluidSim();

    createDescriptorPool();
    createDescriptorSetLayouts();
    createCommandPool();

    updateDescriptorSets();
    createPipelineCache();
    createRenderPipeline();
    createComputePipeline();
}

void FluidDynamicsDemo::createDescriptorPool() {
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

void FluidDynamicsDemo::createCommandPool() {
    commandPool = device.createCommandPool(*device.queueFamilyIndex.graphics, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    commandBuffers = commandPool.allocateCommandBuffers(swapChainImageCount);
}

void FluidDynamicsDemo::createPipelineCache() {
    pipelineCache = device.createPipelineCache();
}


void FluidDynamicsDemo::createRenderPipeline() {
    //    @formatter:off
    auto builder = device.graphicsPipelineBuilder();
    grid.pipeline =
        builder
            .allowDerivatives()
            .shaderStage()
                .vertexShader(load("grid.vert.spv"))
                .tessellationControlShader(load("grid.tesc.spv"))
                .tessellationEvaluationShader(load("grid.tese.spv"))
                .fragmentShader(load("grid.frag.spv"))
            .vertexInputState()
                .addVertexBindingDescription(0, sizeof(PatchVertex), VK_VERTEX_INPUT_RATE_VERTEX)
                .addVertexAttributeDescription(0, 0, VK_FORMAT_R32G32_SFLOAT, 0)
                .addVertexAttributeDescription(1, 0, VK_FORMAT_R32_SFLOAT, offsetOf(PatchVertex, orientation))
            .inputAssemblyState()
                .patches()
            .tessellationState()
                .patchControlPoints(4)
                .domainOrigin(VK_TESSELLATION_DOMAIN_ORIGIN_LOWER_LEFT)
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
                    .addPushConstantRange(VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT, 0, sizeof(int))
                    .addDescriptorSetLayout(camSetLayout)
                .renderPass(renderPass)
                .subpass(0)
                .name("grid")
                .pipelineCache(pipelineCache)
            .build(grid.layout);
    auto builder2 = device.graphicsPipelineBuilder();

    brush.pipeline =
        builder
            .basePipeline(grid.pipeline)
            .shaderStage().clear()
                .vertexShader(load("brush.vert.spv"))
                .tessellationControlShader(load("brush.tesc.spv"))
                .tessellationEvaluationShader(load("brush.tese.spv"))
                .fragmentShader(load("brush.frag.spv"))
            .vertexInputState().clear()
                .addVertexBindingDescription(0, sizeof(glm::vec2), VK_VERTEX_INPUT_RATE_VERTEX)
                .addVertexAttributeDescription(0, 0, VK_FORMAT_R32G32_SFLOAT, 0)
            .tessellationState()
                .patchControlPoints(2)
            .layout().clear()
                .addPushConstantRange(VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, 0, sizeof(brush.constants))
                .addDescriptorSetLayout(camSetLayout)
            .name("brush")
        .build(brush.layout);

    auto builder1 = device.graphicsPipelineBuilder();
    vectorView.thick.pipeline =
            builder1
            .allowDerivatives()
            .shaderStage()
                .vertexShader(load("vector.vert.spv"))
                .fragmentShader(load("vector.frag.spv"))
            .vertexInputState()
                .addVertexBindingDescription(0, sizeof(glm::vec2), VK_VERTEX_INPUT_RATE_VERTEX)
                .addVertexAttributeDescription(0, 0, VK_FORMAT_R32G32_SFLOAT, 0)
            .inputAssemblyState()
                .triangles()
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
                    .addPushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(constants))
                    .addDescriptorSetLayout(camSetLayout)
                    .addDescriptorSetLayout(simData.setLayout)
                .renderPass(renderPass)
                .subpass(0)
                .name("vector_thick")
                .pipelineCache(pipelineCache)
            .build(vectorView.thick.layout);

    vectorView.thin.pipeline =
        builder1
            .basePipeline(vectorView.thick.pipeline)
            .inputAssemblyState()
                .lines()
            .rasterizationState()
                .lineWidth(1.5)
            .name("vector_thin")
        .build(vectorView.thin.layout);

    density.pipeline =
        builder1
            .basePipeline(vectorView.thick.pipeline)
            .shaderStage().clear()
                .vertexShader(load("density.vert.spv"))
                .fragmentShader(load("density.frag.spv"))
            .inputAssemblyState()
                .triangles()
            .colorBlendState()
                .attachment().clear()
                .enableBlend()
                .colorBlendOp().add()
                .alphaBlendOp().add()
                .srcColorBlendFactor().srcAlpha()
                .dstColorBlendFactor().oneMinusSrcAlpha()
                .srcAlphaBlendFactor().zero()
                .dstAlphaBlendFactor().one()
                .add()
            .name("density")
        .build(density.layout);

    particles.pipeline =
        builder1
            .basePipeline(vectorView.thick.pipeline)
            .shaderStage()
                .vertexShader(load("particle.vert.spv"))
                .geometryShader(load("particle.geom.spv"))
                .fragmentShader(load("particle.frag.spv"))
            .inputAssemblyState()
                .points()
            .colorBlendState()
                .attachments(1)
            .layout().clear()
                .addPushConstantRange( VK_SHADER_STAGE_GEOMETRY_BIT, 0, sizeof(constants))
                .addDescriptorSetLayout(camSetLayout)
                .addDescriptorSetLayout(simData.setLayout)
                .addDescriptorSetLayout(particles.setLayout)
            .name("particles")
        .build(particles.layout);

    render.pipeline =
        builder1
            .basePipeline(vectorView.thick.pipeline)
            .shaderStage().clear()
                .vertexShader(load("render.vert.spv"))
                .fragmentShader(load("render.frag.spv"))
            .vertexInputState().clear()
                .addVertexBindingDescriptions(ClipSpace::bindingDescription())
                .addVertexAttributeDescriptions(ClipSpace::attributeDescriptions())
            .inputAssemblyState()
                .triangleStrip()
            .layout().clear()
                .addDescriptorSetLayout(textureDescriptorSetLayout)
            .name("render")
        .build(render.layout);

    //    @formatter:on
}

void FluidDynamicsDemo::createComputePipeline() {
    auto module = VulkanShaderModule{ "../../data/shaders/pass_through.comp.spv", device};
    auto stage = initializers::shaderStage({ module, VK_SHADER_STAGE_COMPUTE_BIT});

    compute.layout = device.createPipelineLayout();

    auto computeCreateInfo = initializers::computePipelineCreateInfo();
    computeCreateInfo.stage = stage;
    computeCreateInfo.layout = compute.layout;

    compute.pipeline = device.createComputePipeline(computeCreateInfo, pipelineCache);
}


void FluidDynamicsDemo::onSwapChainDispose() {
    dispose(render.pipeline);
    dispose(compute.pipeline);
}

void FluidDynamicsDemo::onSwapChainRecreation() {
    createRenderPipeline();
    createComputePipeline();
}

VkCommandBuffer *FluidDynamicsDemo::buildCommandBuffers(uint32_t imageIndex, uint32_t &numCommandBuffers) {
    numCommandBuffers = 1;
    auto& commandBuffer = commandBuffers[imageIndex];

    VkCommandBufferBeginInfo beginInfo = initializers::commandBufferBeginInfo();
    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    static std::array<VkClearValue, 2> clearValues;
    clearValues[0].color = {0, 0, 0, 1};
    clearValues[1].depthStencil = {1.0, 0u};

    VkRenderPassBeginInfo rPassInfo = initializers::renderPassBeginInfo();
    rPassInfo.clearValueCount = COUNT(clearValues);
    rPassInfo.pClearValues = clearValues.data();
    rPassInfo.framebuffer = framebuffers[imageIndex];
    rPassInfo.renderArea.offset = {0u, 0u};
    rPassInfo.renderArea.extent = swapChain.extent;
    rPassInfo.renderPass = renderPass;

    vkCmdBeginRenderPass(commandBuffer, &rPassInfo, VK_SUBPASS_CONTENTS_INLINE);

//    renderImage(commandBuffer);
    renderGrid(commandBuffer);
    renderVectorField(commandBuffer);
    renderParticles(commandBuffer);
    renderBrush(commandBuffer);
    renderDensityField(commandBuffer);
    renderUI(commandBuffer);

    vkCmdEndRenderPass(commandBuffer);
    vkEndCommandBuffer(commandBuffer);

    return &commandBuffer;
}

void FluidDynamicsDemo::renderGrid(VkCommandBuffer commandBuffer) {
    if(!showGrid) return;
    int n = simData.N;
    VkDeviceSize offset = 0;
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, grid.pipeline);
    vkCmdPushConstants(commandBuffer, grid.layout, VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT, 0, sizeof(int), &n);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, grid.layout, 0, 1, &cameraSet, 0, VK_NULL_HANDLE);
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, grid.vertexBuffer, &offset);
    vkCmdDraw(commandBuffer, grid.vertexBuffer.sizeAs<PatchVertex>(), 1, 0, 0);
}

void FluidDynamicsDemo::renderVectorField(VkCommandBuffer commandBuffer) {
    if(!showVectorField) return;
    int n = (simData.N+2);

    VkDeviceSize  offset = 0;
    static std::array<VkDescriptorSet, 2> sets;
    sets[0] = cameraSet;
    sets[1] = simData.descriptorSets[fluidSim.v_in];
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vectorView.thin.pipeline);
    vkCmdPushConstants(commandBuffer, vectorView.thin.layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(constants), &constants);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vectorView.thin.layout, 0, COUNT(sets), sets.data(), 0, VK_NULL_HANDLE);
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vectorView.thin.vertexBuffer, &offset);
    vkCmdDraw(commandBuffer, vectorView.thin.vertexBuffer.sizeAs<glm::vec2>(), n * n, 0, 0);

}

void FluidDynamicsDemo::renderDensityField(VkCommandBuffer commandBuffer) {
    if(!showDensityField) return;
    int n = simData.N;

    VkDeviceSize  offset = 0;
    static std::array<VkDescriptorSet, 2> sets;
    sets[0] = cameraSet;
    sets[1] = simData.descriptorSets[0];
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, density.pipeline);
    vkCmdPushConstants(commandBuffer, density.layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(constants), &constants);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, density.layout, 0, COUNT(sets), sets.data(), 0, VK_NULL_HANDLE);
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, density.vertexBuffer, &offset);
    vkCmdDraw(commandBuffer, density.vertexBuffer.sizeAs<glm::vec2>(), n * n, 0, 0);
}

void FluidDynamicsDemo::renderParticles(VkCommandBuffer commandBuffer) {
    if(!showParticles) return;
    int n = simData.N * simData.N;

    VkDeviceSize  offset = 0;
    static std::array<VkDescriptorSet, 3> sets;
    sets[0] = cameraSet;
    sets[1] = simData.descriptorSets[0];
    sets[2] = particles.descriptorSet;
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, particles.pipeline);
    vkCmdPushConstants(commandBuffer, particles.layout, VK_SHADER_STAGE_GEOMETRY_BIT, 0, sizeof(constants), &constants);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, particles.layout, 0, COUNT(sets), sets.data(), 0, VK_NULL_HANDLE);
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, particles.vertexBuffer, &offset);
    vkCmdDraw(commandBuffer, 1, n, 0, 0);
}

void FluidDynamicsDemo::renderBrush(VkCommandBuffer commandBuffer) {
    if(!brush.active) return;
    VkDeviceSize offset = 0;
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, brush.pipeline);
    vkCmdPushConstants(commandBuffer, brush.layout, VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, 0, sizeof(brush.constants), &brush.constants);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, brush.layout, 0, 1, &cameraSet, 0, VK_NULL_HANDLE);
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, brush.patch, &offset);
    vkCmdDraw(commandBuffer, 2, 1, 0, 0);

}

void FluidDynamicsDemo::renderImage(VkCommandBuffer commandBuffer) {
    VkDeviceSize offset = 0;
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, render.pipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, render.layout, 0, 1, &textureDescriptorSet, 0, VK_NULL_HANDLE);
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, render.vertexBuffer, &offset);
    vkCmdDraw(commandBuffer, 4, 1, 0, 0);
}

void FluidDynamicsDemo::renderUI(VkCommandBuffer commandBuffer) {
//    ImGui::Begin("Fluid Sim");
//    ImGui::SetWindowSize({0, 0});
//    ImGui::Checkbox("particles", &showParticles); ImGui::SameLine();
//    ImGui::Checkbox("grid", &showGrid); ImGui::SameLine();
//    ImGui::Checkbox("vector field", &showVectorField);
//    ImGui::SliderFloat("speed", &speed, 0.01, 1.0);
//
//    bool open = true;
//    brush.active = ImGui::CollapsingHeader("paint", &open, ImGuiTreeNodeFlags_DefaultOpen);
//    if(brush.active){
////        ImGui::RadioButton("vector field", &paintState, PAINT_VECTOR_FIELD);
////        ImGui::RadioButton("density field", &paintState, PAINT_DENSITY_FIELD);
//        ImGui::RadioButton("Vector Field", &paintState, 0); ImGui::SameLine();
//        ImGui::RadioButton("Density Field", &paintState, 1); ImGui::SameLine();
//        ImGui::Checkbox("erase", &brush.erase);
//        ImGui::SliderFloat("brush size", &brush.constants.radius, 0.03, 0.25);
//    }
//    simStarted |= ImGui::Button("start sim");
//    ImGui::End();
//
//    plugin(IM_GUI_PLUGIN).draw(commandBuffer);
}

static bool first = true;

void FluidDynamicsDemo::update(float time) {
//    constants.dt = time * speed;
    constants.dt = 0.00833333333333333333333333333333f;
    if(simStarted){

//        runInBackground([&]{
//            auto dt = float(1.0f/simData.N);
//            fluid_sim(simData.N, dt, 1.0, 0, simData.u[1], simData.v[1], simData.u[0], simData.v[0], simData.density[0], simData.density[1]);
//        });
        gpuSimulation();
    }
}

void FluidDynamicsDemo::gpuSimulation() {
    simCommandPool.oneTimeCommand([&](auto commandBuffer){
//        fluidSim.advectVectorField(commandBuffer);
//        fluidSim.swapVectorFieldBuffers();
        fluidSim.velocityStep(commandBuffer, 0, 0);
    });
}

void FluidDynamicsDemo::checkAppInputs() {
    if(!ImGui::IsAnyItemActive()) {
        if (brush.active && mouse.left.held) {
            if(paintState == PAINT_VECTOR_FIELD) {
                updateVelocityField();
            }else if(paintState == PAINT_DENSITY_FIELD){
                updateDensityField();
            }

        }else if (brush.active && mouse.left.released){
            brush.trail.clear();
        }
        else if (debug && mouse.left.released) {
            auto out = fluidSim.v_out;
            auto n = simData.N + 2;
            auto pos = mousePositionToWorldSpace(camera).xy();
            glm::ivec2 gridId = floor(pos.xy() * static_cast<float>(n));
            int id = gridId.y * n + gridId.x;
            glm::vec2 v{simData.u[out][id], simData.v[out][id]};
            std::vector<glm::vec2> gridCellParticles;
            for (int i = 0; i < particles.particleBuffer.sizeAs<glm::vec2>(); i++) {
                auto p = particles.particlePtr[i];
                glm::ivec2 gridId = floor(p.xy() * static_cast<float>(n));
                int id1 = gridId.y * n + gridId.x;
                if (id == id1) {
                    gridCellParticles.push_back(p);
                }
            }
            gpuSimulation();
            out = fluidSim.v_in;
            spdlog::info("id: {}, gridID: {}, velocity: {}", id, glm::vec2(gridId), v);
            if(gridId.x == 1){
                id = gridId.y * n;
                v = glm::vec2{simData.u[out][id], simData.v[out][id]};
                spdlog::info("id: {}, gridID: {}, boundary: {}", id, glm::vec2(gridId), v);
            }
            if(gridId.x == simData.N){
                id = gridId.y * n + (simData.N + 1);
                v = glm::vec2{simData.u[out][id], simData.v[out][id]};
                spdlog::info("id: {}, gridID: {}, boundary: {}", id, glm::vec2(gridId), v);
            }
            if(gridId.y == 1){
                id = gridId.x;
                v = glm::vec2{simData.u[out][id], simData.v[out][id]};
                spdlog::info("id: {}, gridID: {}, boundary: {}", id, glm::vec2(gridId), v);
            }
            if(gridId.y == simData.N){
                id = (gridId.y+1) * n + gridId.x;
                v = glm::vec2{simData.u[out][id], simData.v[out][id]};
                spdlog::info("id: {}, gridID: {}, boundary: {}", id, glm::vec2(gridId), v);
            }
        }
    }
}

void FluidDynamicsDemo::cleanup() {
    simData.densityBuffer[0].unmap();
    simData.densityBuffer[1].unmap();

    simData.uBuffer[0].unmap();
    simData.uBuffer[1].unmap();

    simData.vBuffer[0].unmap();
    simData.vBuffer[1].unmap();
}

void FluidDynamicsDemo::onPause() {
    VulkanBaseApp::onPause();
}

void FluidDynamicsDemo::initSimData() {
    const int N = simData.N;
    simData.size = (N+2) * (N+2);
    auto byteSize =  simData.size  * sizeof(float);
    simData.densityBuffer[0] = device.createBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, byteSize,  "density_0");
    simData.densityBuffer[1] = device.createBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, byteSize,  "density_1");

    simData.uBuffer[0] = device.createBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, byteSize ,  "velocity_u_0");
    simData.uBuffer[1] = device.createBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, byteSize,  "velocity_u_1");

    simData.vBuffer[0] = device.createBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, byteSize ,  "velocity_v_0");
    simData.vBuffer[1] = device.createBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, byteSize,  "velocity_v_1");

    simData.density[0] = reinterpret_cast<float*>(simData.densityBuffer[0].map());
    simData.density[1] = reinterpret_cast<float*>(simData.densityBuffer[1].map());

    simData.u[0] = reinterpret_cast<float*>(simData.uBuffer[0].map());
    simData.u[1] = reinterpret_cast<float*>(simData.uBuffer[1].map());

    simData.v[0] = reinterpret_cast<float*>(simData.vBuffer[0].map());
    simData.v[1] = reinterpret_cast<float*>(simData.vBuffer[1].map());

    for(int i = 0; i < (N+2); i++){
        for(int j = 0; j < (N+2); j++){
            int id = i * (N+2) + j;
            float x = 2.f * static_cast<float>(j)/static_cast<float>(N) - 1;
            float y = 2.f * static_cast<float>(i)/static_cast<float>(N) - 1;
            float u = sin(2 * glm::pi<float>() * y);
            float v = cos(2 * glm::pi<float>() * x);

//            float x = static_cast<float>(i)/static_cast<float>(N) * glm::two_pi<float>();
//            float y = static_cast<float>(j)/static_cast<float>(N) * glm::pi<float>();

//            float u = glm::cos(x) * glm::sin(y);
//            float v = glm::sin(x) * glm::sin(y);

            simData.u[0][id] = simData.u[1][id]  = y;
            simData.v[0][id] = simData.v[1][id] = x;
        }
    }

    float maxMagnitude{std::numeric_limits<float>::min()};

    auto i = simData.N + 2;
    auto end = simData.N * (simData.N + 2) + simData.N + 1;
    for(i = 0; i < end; i++){
        glm::vec2 u{simData.u[0][i], simData.v[0][i]};
        auto magnitude = glm::length(u);
        maxMagnitude = glm::max(maxMagnitude, magnitude);
    }

    constants.N = simData.N;
    constants.maxMagnitude = maxMagnitude;
}

void FluidDynamicsDemo::initFluidSim() {
//    auto dt = (1.0f/static_cast<float>(simData.N) * 5)/constants.maxMagnitude;
//    auto dt = 1.0f/static_cast<float>(simData.N);
    float dt = 1.0f/static_cast<float>(simData.N) * 0.01;
    spdlog::info("dt: {}", dt);
    fluidSim = FluidSim{ &device, fileManager, simData.uBuffer[0], simData.vBuffer[0]
            , simData.uBuffer[1], simData.vBuffer[1], simData.N, dt, 1.0};
    fluidSim.init();
//    fluidSim.set(colorTexture);
    simCommandPool = device.createCommandPool(*device.queueFamilyIndex.compute, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);
    simCommandPool.oneTimeCommand([&](auto commandBuffer){
        fluidSim.setBoundary(commandBuffer, VELOCITY_FIELD_U, HORIZONTAL_COMPONENT_BOUNDARY);
        fluidSim.setBoundary(commandBuffer, VELOCITY_FIELD_V, VERTICAL_COMPONENT_BOUNDARY);
        fluidSim.swapVectorFieldBuffers();
        fluidSim.setBoundary(commandBuffer, VELOCITY_FIELD_U, HORIZONTAL_COMPONENT_BOUNDARY);
        fluidSim.setBoundary(commandBuffer, VELOCITY_FIELD_V, VERTICAL_COMPONENT_BOUNDARY);
        fluidSim.swapVectorFieldBuffers();
    });

}

void FluidDynamicsDemo::initGrid() {
    auto angle = glm::half_pi<float>();
    std::vector<PatchVertex> points{
            {{0, 0}, 0}, {{1, 0}, 0},
            {{1, 1}, 0}, {{0, 1}, 0},

            {{0, 0}, angle}, {{1, 0}, angle},
            {{1, 1}, angle}, {{0, 1}, angle},
    };
    grid.vertexBuffer = device.createDeviceLocalBuffer(points.data(), BYTE_SIZE(points), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
}

void FluidDynamicsDemo::initCamera() {
    camera.proj = vkn::ortho(0, 1, 0, 1, -1, 1);
//    camera.proj = vkn::ortho(-1, 1, -1, 1, -1, 1);
    camBuffer = device.createDeviceLocalBuffer(&camera, sizeof(camera), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
}

void FluidDynamicsDemo::createDescriptorSetLayouts() {
    camSetLayout =
        device.descriptorSetLayoutBuilder()
            .binding(0)
                .descriptorType(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
                .descriptorCount(1)
                .shaderStages(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT | VK_SHADER_STAGE_GEOMETRY_BIT)
            .createLayout();
    
    textureDescriptorSetLayout =
        device.descriptorSetLayoutBuilder()
            .binding(0)
                .descriptorType(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
                .descriptorCount(1)
                .shaderStages(VK_SHADER_STAGE_FRAGMENT_BIT)
            .createLayout();

    auto stages = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
    simData.setLayout =
        device.descriptorSetLayoutBuilder()
            .binding(0)
                .descriptorType(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
                .descriptorCount(1)
                .shaderStages(stages)
            .binding(1)
                .descriptorType(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
                .descriptorCount(1)
                .shaderStages(stages)
            .binding(2)
                .descriptorType(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
                .descriptorCount(1)
                .shaderStages(stages)
            .createLayout();

    particles.setLayout =
        device.descriptorSetLayoutBuilder()
            .binding(0)
                .descriptorType(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
                .descriptorCount(1)
                .shaderStages(VK_SHADER_STAGE_GEOMETRY_BIT)
            .createLayout();


    auto sets = descriptorPool.allocate({ camSetLayout, simData.setLayout, simData.setLayout, particles.setLayout, textureDescriptorSetLayout });
    cameraSet = sets[0];
    simData.descriptorSets[0] = sets[1];
    simData.descriptorSets[1] = sets[2];
    particles.descriptorSet = sets[3];
    textureDescriptorSet = sets[4];

    device.setName<VK_OBJECT_TYPE_DESCRIPTOR_SET>("sim_data", simData.descriptorSets[0]);
    device.setName<VK_OBJECT_TYPE_DESCRIPTOR_SET>("sim_data_prev", simData.descriptorSets[1]);
    device.setName<VK_OBJECT_TYPE_DESCRIPTOR_SET>("particles", particles.descriptorSet);
    device.setName<VK_OBJECT_TYPE_DESCRIPTOR_SET>("image_texture", textureDescriptorSet);
}

void FluidDynamicsDemo::updateDescriptorSets() {
    auto writes = initializers::writeDescriptorSets<9>();
    
    writes[0].dstSet = cameraSet;
    writes[0].dstBinding = 0;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writes[0].descriptorCount = 1;
    VkDescriptorBufferInfo camInfo{ camBuffer, 0, VK_WHOLE_SIZE};
    writes[0].pBufferInfo = &camInfo;

    VkDeviceSize  size = sizeof(float) * simData.size;
    writes[1].dstSet = simData.descriptorSets[0];
    writes[1].dstBinding = 0;
    writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[1].descriptorCount = 1;
    VkDescriptorBufferInfo uInfo{simData.uBuffer[0], 0, VK_WHOLE_SIZE};
    writes[1].pBufferInfo = &uInfo;

    writes[2].dstSet = simData.descriptorSets[0];
    writes[2].dstBinding = 1;
    writes[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[2].descriptorCount = 1;
    VkDescriptorBufferInfo vInfo{simData.vBuffer[0], 0, VK_WHOLE_SIZE};
    writes[2].pBufferInfo = &vInfo;

    writes[3].dstSet = simData.descriptorSets[0];
    writes[3].dstBinding = 2;
    writes[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[3].descriptorCount = 1;
    VkDescriptorBufferInfo densInfo{simData.densityBuffer[0], 0, VK_WHOLE_SIZE};
    writes[3].pBufferInfo = &densInfo;

    writes[4].dstSet = simData.descriptorSets[1];
    writes[4].dstBinding = 0;
    writes[4].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[4].descriptorCount = 1;
    VkDescriptorBufferInfo uInfo1{simData.uBuffer[1], 0, VK_WHOLE_SIZE};
    writes[4].pBufferInfo = &uInfo1;

    writes[5].dstSet = simData.descriptorSets[1];
    writes[5].dstBinding = 1;
    writes[5].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[5].descriptorCount = 1;
    VkDescriptorBufferInfo vInfo1{simData.vBuffer[1], 0, VK_WHOLE_SIZE};
    writes[5].pBufferInfo = &vInfo1;

    writes[6].dstSet = simData.descriptorSets[1];
    writes[6].dstBinding = 2;
    writes[6].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[6].descriptorCount = 1;
    VkDescriptorBufferInfo densInfo1{simData.densityBuffer[1], 0, size};
    writes[6].pBufferInfo = &densInfo1;

    writes[7].dstSet = particles.descriptorSet;
    writes[7].dstBinding = 0;
    writes[7].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[7].descriptorCount = 1;
    VkDescriptorBufferInfo particleInfo{particles.particleBuffer, 0, VK_WHOLE_SIZE};
    writes[7].pBufferInfo = &particleInfo;

    writes[8].dstSet = textureDescriptorSet;
    writes[8].dstBinding = 0;
    writes[8].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[8].descriptorCount = 1;
    VkDescriptorImageInfo imageInfo{colorTexture.sampler, colorTexture.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    writes[8].pImageInfo = &imageInfo;
    
    device.updateDescriptorSets(writes);
}

void FluidDynamicsDemo::initVectorRender() {
    std::vector<glm::vec2> vertices {
            {-0.5, 0.5}, {0.0, 0.0}, {0.5, 0.0},
            {0.5, 0.0}, {0.0, 0.0}, {-0.5, -0.5}
    };
    vectorView.thick.vertexBuffer = device.createDeviceLocalBuffer(vertices.data(), BYTE_SIZE(vertices), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);

    vertices = {
            {0.5, 0.0}, {0.25, 0.22},
            {0.5, 0.0}, {-0.5, 0.0},
            {0.5, 0.0}, {0.25, -0.25}
    };
    vectorView.thin.vertexBuffer = device.createDeviceLocalBuffer(vertices.data(), BYTE_SIZE(vertices), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
}

void FluidDynamicsDemo::initDensityRender() {
    std::vector<glm::vec2> vertices {
            {0, 0}, {1, 0}, {0, 1},
            {0, 1}, {1, 0}, {1, 1}
    };

    density.vertexBuffer = device.createDeviceLocalBuffer(vertices.data(), BYTE_SIZE(vertices), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
}

void FluidDynamicsDemo::initRenderBuffer() {
    auto vertices = ClipSpace::Quad::positions;
    render.vertexBuffer = device.createDeviceLocalBuffer(vertices.data(), BYTE_SIZE(vertices), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);

}

void FluidDynamicsDemo::initParticles() {
    particles.vertexBuffer = device.createBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_ONLY, sizeof(glm::vec2), "particles_vertex");

    std::vector<glm::vec2> data;
    auto n = static_cast<float>(simData.N);
    float halfSize = 0.5f/n;
    for(int j = 0; j < simData.N; j++){
        for(int i = 0; i < simData.N; i++){
            float x = static_cast<float>(i)/n + halfSize;
            float y = static_cast<float>(j)/n + halfSize;
            data.emplace_back(x, y);
        }
    }

    particles.particleBuffer = device.createCpuVisibleBuffer(data.data(), BYTE_SIZE(data), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    particles.particlePtr = reinterpret_cast<glm::vec2*>(particles.particleBuffer.map());
}

void FluidDynamicsDemo::updateVelocityField() {
    auto pos = mousePositionToWorldSpace(camera).xy();
    brush.trail.push_front(std::make_tuple(pos, elapsedTime));
    brush.constants.position = pos;
    float cellSize = 1.f/simData.N;
    int steps = (int(brush.constants.radius/cellSize));

    auto n = static_cast<float>(simData.N);
    int min = simData.N + 1;
    int max = simData.N * (simData.N+2) + simData.N;
    for(int i = -steps; i <= steps; i++){
        for(int j = -steps; j <= steps; j++){
            auto next = begin(brush.trail);

            auto [curPosition, time] = *next;
            auto offset = glm::vec2{i, j} * cellSize;
            auto cellId = glm::ivec2(glm::floor((curPosition +  offset) * n));
            cellId += 1;
            auto id = cellId.y * (simData.N+2) + cellId.x;
            if(id < min || id > max) continue;

            if(brush.erase){
                simData.u[0][id] = 0;
                simData.v[0][id] = 0;
                continue;
            }


            glm::vec2 prevCenter{0};
            float prevTime{0};
            std::advance(next, 1);
//                    if(brush.trail.size() > 10) {
//                        spdlog::info("{}", *next);
//                    }
            bool moved = false;
            while(next != end(brush.trail)){
                auto [prevPos, dt] = *next;
                for(int k = -steps; k <= steps; k++){
                    for(int l = -steps; l <= steps; l++){
                        auto preOffset = glm::vec2{k, l} * cellSize;
                        auto prevCellId = glm::ivec2(glm::floor((prevPos + preOffset) * n));
                        prevCellId += 1;
                        id = prevCellId.y * (simData.N+2) + prevCellId.x;
                        if(id < min || id > max) continue;
//                                spdlog::info("{} =? {}", cellId, prevCellId);
                        if(prevCellId == cellId){
                            prevCenter = prevPos;
                            prevTime = dt;

                        }
                    }
                }
//                        if(glm::all(glm::notEqual(glm::vec2(0), prevCenter))) {
//                            spdlog::info("pn+1: {}, tn+1: {}, pn: {}, tn: {}", curPosition, elapsedTime, prevCenter,
//                                         prevTime);
//                        }

                if(glm::all(glm::equal(curPosition, prevCenter))){
                    std::advance(next, 1);
                    continue;
                }

                auto t = time - prevTime;
                auto v = (curPosition - prevCenter)/t;
                v = glm::normalize(v) * 10.0f;
                id = cellId.y * (simData.N+2) + cellId.x;
                simData.u[0][id] = v.x;
                simData.v[0][id] = v.y;
                std::advance(next, 1);
            }
        }
    }
}

void FluidDynamicsDemo::updateDensityField() {
    auto pos = mousePositionToWorldSpace(camera).xy();
    brush.trail.push_front(std::make_tuple(pos, elapsedTime));
    brush.constants.position = pos;
    float cellSize = 1.f/simData.N;
    int steps = (int(brush.constants.radius/cellSize));

    auto n = static_cast<float>(simData.N);
    int min = simData.N + 1;
    int max = simData.N * (simData.N+2) + simData.N;
    for(int i = -steps; i <= steps; i++){
        for(int j = -steps; j <= steps; j++){
            auto next = begin(brush.trail);

            auto [curPosition, time] = *next;
            auto offset = glm::vec2{i, j} * cellSize;
            auto cellId = glm::ivec2(glm::floor((curPosition +  offset) * n));
            cellId += 1;
            auto id = cellId.y * (simData.N+2) + cellId.x;
            if(id < min || id > max) continue;
            simData.density[0][id] = brush.erase ? 0.0f : 1.0f;

        }
    }
}

void FluidDynamicsDemo::initBrush() {
    std::vector<glm::vec2> data(2, glm::vec2(0));
    brush.patch = device.createDeviceLocalBuffer(data.data(), BYTE_SIZE(data), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
}

void FluidDynamicsDemo::createUVTexture() {

    const uint32_t N = simData.N;
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_NEAREST;
    samplerInfo.minFilter = VK_FILTER_NEAREST;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

    colorTexture.sampler = device.createSampler(samplerInfo);

    textures::generate(device, colorTexture, N, N, [](auto i, auto j, auto w, auto h){
        using namespace glm;
        float x = 2 * (i/w) - 1;
        float y = 2 * (j/w) - 1;
        return vec3{
                step(1.0, mod(floor((x + 1.0) / 0.2) + floor((y + 1.0) / 0.2), 2.0)),
                step(1.0, mod(floor((x + 1.0) / 0.2) + floor((y + 1.0) / 0.2), 2.0)),
                step(1.0, mod(floor((x + 1.0) / 0.2) + floor((y + 1.0) / 0.2), 2.0))
        };
    });


}