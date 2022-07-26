#include "FluidSimulation.hpp"
#include "GraphicsPipelineBuilder.hpp"
#include "DescriptorSetBuilder.hpp"

FluidSimulation::FluidSimulation(const Settings& settings) : VulkanBaseApp("Fluid Simulation", settings) {
    fileManager.addSearchPath(".");
    fileManager.addSearchPath("../../examples/fluid_simulation");
    fileManager.addSearchPath("../../examples/fluid_simulation/spv");
    fileManager.addSearchPath("../../examples/fluid_simulation/models");
    fileManager.addSearchPath("../../examples/fluid_simulation/textures");
    fileManager.addSearchPath("../../data/shaders");
    fileManager.addSearchPath("../../data/models");
    fileManager.addSearchPath("../../data/textures");
    fileManager.addSearchPath("../../data");
    epsilon = 1.0f/float(width);    // TODO 2d epsilon
}

void FluidSimulation::initApp() {
    initAdvectStage();
    initVectorField();
    initColorField();
    initFullScreenQuad();
    createDescriptorPool();
    createCommandPool();
    createDescriptorSetLayouts();
    updateDescriptorSets();

    createPipelineCache();
    createRenderPipeline();
}

void FluidSimulation::initVectorField() {
    float dx = 1.0f/float(width);
    std::vector<glm::vec4> field;
    float maxLength = MIN_FLOAT;
    constexpr auto two_pi = glm::two_pi<float>();
    for(int i = 0; i < height; i++){
        for(int j = 0; j < width; j++){
            auto x = 2 * float(j)/float(width) - 1;
            auto y = 2 * float(i)/float(height) - 1;
            glm::vec2 u{glm::sin(two_pi * y), glm::sin(two_pi * x)};
            maxLength = glm::max(glm::length(u), maxLength);
            field.emplace_back(u , 0, 0);
//            field.emplace_back(1, glm::sin(two_pi * x), 0, 0);
//            field.emplace_back(x, y, 0, 0); // divergent fields 1
//            field.emplace_back(glm::sin(two_pi * x), 0, 0, 0); // divergent fields 2
//            field.emplace_back(y, x, 0, 0); // divergent fields 3
        }
    }

    const int interval = 30;
    std::vector<glm::vec2> triangles{
            {0, 0.2}, {1, 0}, {0, -0.2}
    };
    std::vector<Vector> vectors;
    for(auto i = interval/2; i < height; i += interval){
        for(auto j = interval/2; j < width; j += interval){
            for(int k = 0; k < 3; k++){
                auto vertex = triangles[k];
                glm::vec2 position{2 * (float(j)/float(width)) - 1
                                   , 2 * (float(i)/float(height)) - 1};
                vectors.push_back(Vector{vertex, position});
            }
        }
    }

    arrows.vertexBuffer = device.createDeviceLocalBuffer(vectors.data(), BYTE_SIZE(vectors), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    arrows.numArrows = vectors.size();

    textures::create(device, vectorField.texture[0], VK_IMAGE_TYPE_2D, VK_FORMAT_R32G32B32A32_SFLOAT
                     , field.data(), {width, height, 1}, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE
                     , sizeof(float));
    textures::create(device, vectorField.texture[1], VK_IMAGE_TYPE_2D, VK_FORMAT_R32G32B32A32_SFLOAT
                     , field.data(), {width, height, 1}, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE
                     , sizeof(float));

    for(auto i = 0; i < 2; i++){
        vectorField.framebuffer[i] = device.createFramebuffer(advectPipeline.renderPass, { vectorField.texture[i].imageView }, width, height);
        device.setName<VK_OBJECT_TYPE_FRAMEBUFFER>(fmt::format("{}_{}", "vector_field", i), vectorField.framebuffer[i].frameBuffer);
    }
//    advectPipeline.constants.dt = (5.0f * dx)/maxLength;
    spdlog::info("dt: {}", advectPipeline.constants.dt);

}

void FluidSimulation::initColorField() {

    auto checkerboard = [](int i, int j, float w, float h){
        auto x = 2 * (float(j)/w) - 1;
        auto y = 2 * (float(i)/h) - 1;
        return glm::vec3{
                glm::step(1.0, glm::mod(floor((x + 1.0) / 0.2) + floor((y + 1.0) / 0.2), 2.0)),
                glm::step(1.0, glm::mod(floor((x + 1.0) / 0.2) + floor((y + 1.0) / 0.2), 2.0)),
                glm::step(1.0, glm::mod(floor((x + 1.0) / 0.2) + floor((y + 1.0) / 0.2), 2.0))
        };
    };
    std::vector<glm::vec4> field;
    for(auto i = 0; i < height; i++){
        for(auto j = 0; j < width; j++){
            auto color = checkerboard(j, i, float(width), float(height));
            field.emplace_back(color, 1);
        }
    }

    textures::create(device, colorField.texture[0], VK_IMAGE_TYPE_2D, VK_FORMAT_R32G32B32A32_SFLOAT
            , field.data(), {width, height, 1}, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE
            , sizeof(float));
    textures::create(device, colorField.texture[1], VK_IMAGE_TYPE_2D, VK_FORMAT_R32G32B32A32_SFLOAT
            , field.data(), {width, height, 1}, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE
            , sizeof(float));

    device.setName<VK_OBJECT_TYPE_IMAGE>(fmt::format("{}_{}", "color_field", 0), colorField.texture[0].image.image);
    device.setName<VK_OBJECT_TYPE_IMAGE>(fmt::format("{}_{}", "color_field", 1), colorField.texture[1].image.image);

    device.setName<VK_OBJECT_TYPE_IMAGE_VIEW>(fmt::format("{}_{}", "color_field", 0), colorField.texture[0].imageView.handle);
    device.setName<VK_OBJECT_TYPE_IMAGE_VIEW>(fmt::format("{}_{}", "color_field", 1), colorField.texture[1].imageView.handle);

    for(auto i = 0; i < 2; i++){
        colorField.framebuffer[i] = device.createFramebuffer(advectPipeline.renderPass, { colorField.texture[i].imageView }, width, height);
        device.setName<VK_OBJECT_TYPE_FRAMEBUFFER>(fmt::format("{}_{}", "color_field", i), colorField.framebuffer[i].frameBuffer);
    }
}

void FluidSimulation::initFullScreenQuad() {
    auto quad = ClipSpace::Quad::positions;
    screenQuad.vertices = device.createDeviceLocalBuffer(quad.data(), BYTE_SIZE(quad), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
}

void FluidSimulation::createDescriptorPool() {
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

void FluidSimulation::createDescriptorSetLayouts() {
    textureSetLayout =
        device.descriptorSetLayoutBuilder()
            .name("texture_set")
            .binding(0)
                .descriptorType(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
                .descriptorCount(1)
                .shaderStages(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)
            .createLayout();
    
    auto sets = descriptorPool.allocate({ textureSetLayout, textureSetLayout, textureSetLayout, textureSetLayout});

    vectorField.descriptorSet[0] = sets[0];
    vectorField.descriptorSet[1] = sets[1];
    colorField.descriptorSet[0] = sets[2];
    colorField.descriptorSet[1] = sets[3];

    device.setName<VK_OBJECT_TYPE_DESCRIPTOR_SET>(fmt::format("{}_{}", "vector_field", 0), vectorField.descriptorSet[0]);
    device.setName<VK_OBJECT_TYPE_DESCRIPTOR_SET>(fmt::format("{}_{}", "vector_field", 1), vectorField.descriptorSet[1]);

    device.setName<VK_OBJECT_TYPE_DESCRIPTOR_SET>(fmt::format("{}_{}", "color_field", 0), colorField.descriptorSet[0]);
    device.setName<VK_OBJECT_TYPE_DESCRIPTOR_SET>(fmt::format("{}_{}", "color_field", 1), colorField.descriptorSet[1]);
}

void FluidSimulation::updateDescriptorSets() {
    auto writes = initializers::writeDescriptorSets<4>();
    
    writes[0].dstSet = vectorField.descriptorSet[0];
    writes[0].dstBinding = 0;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[0].descriptorCount = 1;
    VkDescriptorImageInfo v0Info{vectorField.texture[0].sampler
                                 , vectorField.texture[0].imageView
                                 , VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    writes[0].pImageInfo = &v0Info;

    writes[1].dstSet = vectorField.descriptorSet[1];
    writes[1].dstBinding = 0;
    writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[1].descriptorCount = 1;
    VkDescriptorImageInfo v1Info{vectorField.texture[1].sampler
                                 , vectorField.texture[1].imageView
                                 , VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    writes[1].pImageInfo = &v1Info;

    writes[2].dstSet = colorField.descriptorSet[0];
    writes[2].dstBinding = 0;
    writes[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[2].descriptorCount = 1;
    VkDescriptorImageInfo c0Info{colorField.texture[0].sampler
                                 , colorField.texture[0].imageView
                                 , VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    writes[2].pImageInfo = &c0Info;

    writes[3].dstSet = colorField.descriptorSet[1];
    writes[3].dstBinding = 0;
    writes[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[3].descriptorCount = 1;
    VkDescriptorImageInfo c1Info{colorField.texture[1].sampler
                                 , colorField.texture[1].imageView
                                 , VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    writes[3].pImageInfo = &c1Info;

    device.updateDescriptorSets(writes);
}

void FluidSimulation::createCommandPool() {
    commandPool = device.createCommandPool(*device.queueFamilyIndex.graphics, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    commandBuffers = commandPool.allocateCommandBuffers(swapChainImageCount);
}

void FluidSimulation::createPipelineCache() {
    pipelineCache = device.createPipelineCache();
}


void FluidSimulation::createRenderPipeline() {
    //    @formatter:off
    auto builder = device.graphicsPipelineBuilder();
    render.pipeline =
        builder
            .allowDerivatives()
            .shaderStage()
                .vertexShader(resource("pass_through.vert.spv"))
                .fragmentShader(resource("pass_through.frag.spv"))
            .vertexInputState()
                .addVertexBindingDescriptions(Vertex::bindingDisc())
                .addVertexAttributeDescriptions(Vertex::attributeDisc())
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
                .renderPass(renderPass)
                .subpass(0)
                .name("render")
                .pipelineCache(pipelineCache)
            .build(render.layout);

    arrows.pipeline =
        builder
            .basePipeline(render.pipeline)
            .shaderStage()
                .vertexShader(resource("vectorField.vert.spv"))
                .fragmentShader(resource("vectorField.frag.spv"))
            .vertexInputState().clear()
                .addVertexBindingDescription(0, sizeof(Vector), VK_VERTEX_INPUT_RATE_VERTEX)
                .addVertexAttributeDescription(0, 0, VK_FORMAT_R32G32_SFLOAT, offsetOf(Vector, vertex))
                .addVertexAttributeDescription(1, 0, VK_FORMAT_R32G32_SFLOAT, offsetOf(Vector, position))
            .layout()
                .addDescriptorSetLayout(textureSetLayout)
            .name("vector_field")
        .build(arrows.layout);

    screenQuad.pipeline =
        builder
            .basePipeline(render.pipeline)
            .shaderStage()
                .vertexShader(resource("quad.vert.spv"))
                .fragmentShader(resource("quad.frag.spv"))
            .vertexInputState().clear()
                .addVertexBindingDescriptions(ClipSpace::bindingDescription())
                .addVertexAttributeDescriptions(ClipSpace::attributeDescriptions())
            .inputAssemblyState()
                .triangleStrip()
            .layout()
                .addDescriptorSetLayout(textureSetLayout)
            .name("fullscreen_quad")
        .build(screenQuad.layout);

    advectPipeline.pipeline =
        builder
            .basePipeline(render.pipeline)
            .shaderStage()
                .vertexShader(resource("quad.vert.spv"))
                .fragmentShader(resource("advect.frag.spv"))
            .layout().clear()
                .addDescriptorSetLayouts({textureSetLayout, textureSetLayout})
                .addPushConstantRange(VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(advectPipeline.constants))
            .renderPass(advectPipeline.renderPass)
            .name("advect")
        .build(advectPipeline.layout);

    //    @formatter:on
}

void FluidSimulation::initAdvectStage() {

    VkAttachmentDescription attachmentDescription{
        0,
        VK_FORMAT_R32G32B32A32_SFLOAT,
        VK_SAMPLE_COUNT_1_BIT,
        VK_ATTACHMENT_LOAD_OP_CLEAR,
        VK_ATTACHMENT_STORE_OP_STORE,
        VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        VK_ATTACHMENT_STORE_OP_STORE,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    };

    std::vector<SubpassDescription> subpasses(1);
    subpasses[0].colorAttachments.push_back({0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});

    std::vector<VkSubpassDependency> dependencies(2);

    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;
    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    dependencies[1].srcSubpass = 0;
    dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    advectPipeline.renderPass = device.createRenderPass({attachmentDescription}, subpasses, dependencies);
    device.setName<VK_OBJECT_TYPE_RENDER_PASS>("advect", advectPipeline.renderPass.renderPass);
}


void FluidSimulation::onSwapChainDispose() {
    dispose(render.pipeline);
    dispose(compute.pipeline);
}

void FluidSimulation::onSwapChainRecreation() {
    initVectorField();
    updateDescriptorSets();
    createRenderPipeline();
}

VkCommandBuffer *FluidSimulation::buildCommandBuffers(uint32_t imageIndex, uint32_t &numCommandBuffers) {
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

    renderVectorField(commandBuffer);
    renderColorField(commandBuffer);

    vkCmdEndRenderPass(commandBuffer);
    vkEndCommandBuffer(commandBuffer);

    return &commandBuffer;
}

void FluidSimulation::renderVectorField(VkCommandBuffer commandBuffer) {
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, arrows.vertexBuffer, &offset);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, arrows.pipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, arrows.layout
            , 0, 1, &vectorField.descriptorSet[0], 0
            , VK_NULL_HANDLE);

    vkCmdDraw(commandBuffer, arrows.numArrows, 1, 0, 0);
}

void FluidSimulation::renderColorField(VkCommandBuffer commandBuffer) {
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, screenQuad.vertices, &offset);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, screenQuad.pipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, screenQuad.layout
            , 0, 1, &colorField.descriptorSet[in], 0
            , VK_NULL_HANDLE);

    vkCmdDraw(commandBuffer, 4, 1, 0, 0);
}

void FluidSimulation::update(float time) {
    auto title = fmt::format("{} - fps {}", this->title, framePerSecond);
    glfwSetWindowTitle(window, title.c_str());
    runSimulation();
}

void FluidSimulation::runSimulation() {
    device.graphicsCommandPool().oneTimeCommand([&](auto commandBuffer){
        if(options.advectVField) {
            advectVectorField(commandBuffer);
            vectorField.swap();
        }
        advectColor(commandBuffer);
        colorField.swap();
    });
}

void FluidSimulation::advectColor(VkCommandBuffer commandBuffer) {

    static std::array<VkDescriptorSet, 2> sets;
    sets[0] = vectorField.descriptorSet[in];
    sets[1] = colorField.descriptorSet[in];

    advect(commandBuffer, sets,colorField.framebuffer[out]);
}

void FluidSimulation::advectVectorField(VkCommandBuffer commandBuffer) {
    static std::array<VkDescriptorSet, 2> sets;
    sets[0] = vectorField.descriptorSet[in];
    sets[1] = vectorField.descriptorSet[in];

    advect(commandBuffer, sets,vectorField.framebuffer[out]);
}

void FluidSimulation::advect(VkCommandBuffer commandBuffer, const std::array<VkDescriptorSet, 2> &sets,
                             VulkanFramebuffer &framebuffer) {

    static std::array<VkClearValue, 1> clearValues;
    clearValues[0].color = {0, 0, 0, 1};

    VkRenderPassBeginInfo rPassInfo = initializers::renderPassBeginInfo();
    rPassInfo.clearValueCount = COUNT(clearValues);
    rPassInfo.pClearValues = clearValues.data();
    rPassInfo.framebuffer = framebuffer;
    rPassInfo.renderArea.offset = {0u, 0u};
    rPassInfo.renderArea.extent = swapChain.extent;
    rPassInfo.renderPass = advectPipeline.renderPass;

    vkCmdBeginRenderPass(commandBuffer, &rPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, screenQuad.vertices, &offset);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, advectPipeline.pipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, advectPipeline.layout
            , 0, COUNT(sets), sets.data(), 0, VK_NULL_HANDLE);

    vkCmdPushConstants(commandBuffer, advectPipeline.layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(advectPipeline.constants), &advectPipeline.constants);
    vkCmdDraw(commandBuffer, 4, 1, 0, 0);

    vkCmdEndRenderPass(commandBuffer);
}

void FluidSimulation::checkAppInputs() {
    VulkanBaseApp::checkAppInputs();
}

void FluidSimulation::cleanup() {
    // TODO save pipeline cache
    VulkanBaseApp::cleanup();
}

void FluidSimulation::onPause() {
    VulkanBaseApp::onPause();
}


int main(){
    try{

        Settings settings;
        settings.width = settings.height = 600;
        settings.depthTest = true;
        settings.vSync = true;

        auto app = FluidSimulation{settings };
        app.run();
    }catch(std::runtime_error& err){
        spdlog::error(err.what());
    }
}