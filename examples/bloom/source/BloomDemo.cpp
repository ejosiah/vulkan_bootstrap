#include "BloomDemo.hpp"
#include "GraphicsPipelineBuilder.hpp"
#include "DescriptorSetBuilder.hpp"
#include "ImGuiPlugin.hpp"

BloomDemo::BloomDemo(const Settings& settings) : VulkanBaseApp("Bloom effect", settings) {
    fileManager.addSearchPath(".");
    fileManager.addSearchPath("../../examples/bloom");
    fileManager.addSearchPath("../../examples/bloom/spv");
    fileManager.addSearchPath("../../examples/bloom/models");
    fileManager.addSearchPath("../../examples/bloom/textures");
    fileManager.addSearchPath("../../data/shaders");
    fileManager.addSearchPath("../../data/models");
    fileManager.addSearchPath("../../data/textures");
    fileManager.addSearchPath("../../data");
}

void BloomDemo::initApp() {
    createCommandPool();
    createDescriptorPool();
    createGlobalSampler();
    createDescriptorSetLayouts();

    initSceneFrameBuffer();
    initBlur();
    initPostProcessing();
    initCamera();
    initLights();
    initTextures();
    initQuad();
    createSceneObjects();
    updateDescriptorSets();
    createPipelineCache();
    createRenderPipeline();
}

void BloomDemo::createCommandPool() {
    commandPool = device.createCommandPool(*device.queueFamilyIndex.graphics, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    commandBuffers = commandPool.allocateCommandBuffers(swapChainImageCount);
}

void BloomDemo::createDescriptorPool() {
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


void BloomDemo::createPipelineCache() {
    pipelineCache = device.createPipelineCache();
}


void BloomDemo::createRenderPipeline() {
    auto builder = device.graphicsPipelineBuilder();
    scene.pipeline =
        builder
            .allowDerivatives()
            .shaderStage()
                .vertexShader(load("scene.vert.spv"))
                .fragmentShader(load("scene.frag.spv"))
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
                .depthStencilState()
                    .enableDepthWrite()
                    .enableDepthTest()
                    .compareOpLess()
                    .minDepthBounds(0)
                    .maxDepthBounds(1)
                .colorBlendState()
                    .attachments(2)
                .layout()
                    .addPushConstantRange(Camera::pushConstant())
                    .addDescriptorSetLayout(lightLayoutSet)
                    .addDescriptorSetLayout(textureLayoutSet)
                .renderPass(scene.renderPass)
                .subpass(0)
                .name("scene")
                .pipelineCache(pipelineCache)
            .build(scene.layout);

    lightRender.pipeline =
        builder
            .basePipeline(scene.pipeline)
            .shaderStage()
                .vertexShader(load("light.vert.spv"))
                .fragmentShader(load("light.frag.spv"))
            .vertexInputState()
                .clear()
                .addVertexBindingDescription(0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX)
                .addVertexAttributeDescription(0, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetOf(Vertex, position))
            .layout()
                .clear()
                .addPushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Camera) + sizeof(glm::vec3))
            .name("light")
            .subpass(0)
        .build(lightRender.layout);

    quad.pipeline =
        builder
            .basePipeline(scene.pipeline)
            .shaderStage()
                .vertexShader(load("quad.vert.spv"))
                .fragmentShader(load("quad.frag.spv"))
            .vertexInputState().clear()
                .addVertexBindingDescriptions(ClipSpace::bindingDescription())
                .addVertexAttributeDescriptions(ClipSpace::attributeDescriptions())
            .inputAssemblyState()
                .triangleStrip()
            .colorBlendState()
                .attachments(1)
            .multisampleState()
                .rasterizationSamples(settings.msaaSamples)
            .layout().clear()
                .addDescriptorSetLayout(textureLayoutSet)
            .name("quad")
            .renderPass(renderPass)
            .subpass(0)
        .build(quad.layout);
    
    // TODO compute pipeline builder
    auto blurShader = VulkanShaderModule{load("blur.comp.spv"), device};
    auto stage = initializers::computeShaderStage({ blurShader, VK_SHADER_STAGE_COMPUTE_BIT});
    VkPushConstantRange range{VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(blur.constants)};
    blur.layout = device.createPipelineLayout({ textureLayoutSet, blur.imageSetLayout }, {range});
    
    auto createInfo = initializers::computePipelineCreateInfo();
    createInfo.stage = stage;
    createInfo.layout = blur.layout;
    blur.pipeline = device.createComputePipeline(createInfo);
    device.setName<VK_OBJECT_TYPE_PIPELINE>("blur", blur.pipeline.handle);
    device.setName<VK_OBJECT_TYPE_PIPELINE_LAYOUT>("blur", blur.layout.pipelineLayout);

    auto bloomShader = VulkanShaderModule{load("bloom.comp.spv"), device};
    stage = initializers::computeShaderStage({ bloomShader, VK_SHADER_STAGE_COMPUTE_BIT});
    range.size = sizeof(postProcess.constants);
    postProcess.layout = device.createPipelineLayout({postProcess.setLayout}, {range});
    createInfo.stage = stage;
    createInfo.layout = postProcess.layout;
    postProcess.pipeline = device.createComputePipeline(createInfo);

    device.setName<VK_OBJECT_TYPE_PIPELINE>("post_process", postProcess.pipeline.handle);
    device.setName<VK_OBJECT_TYPE_PIPELINE_LAYOUT>("post_process", postProcess.layout.pipelineLayout);
}



void BloomDemo::onSwapChainDispose() {
    descriptorPool.free({descriptorSets.wood, descriptorSets.container});
}

void BloomDemo::onSwapChainRecreation() {
    cameraController->perspective(swapChain.aspectRatio());
    updateDescriptorSets();
    createRenderPipeline();
}

VkCommandBuffer *BloomDemo::buildCommandBuffers(uint32_t imageIndex, uint32_t &numCommandBuffers) {
    numCommandBuffers = 1;
    auto& commandBuffer = commandBuffers[imageIndex];

    VkCommandBufferBeginInfo beginInfo = initializers::commandBufferBeginInfo();
    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    renderOffScreenFramebuffer(commandBuffer);

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

    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, quad.vertices, &offset);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, quad.pipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, quad.layout, 0, 1, &quad.descriptorSet, 0, VK_NULL_HANDLE);
    vkCmdDraw(commandBuffer, quad.vertices.size/sizeof(glm::vec2), 1, 0, 0);

    renderUI(commandBuffer);
    vkCmdEndRenderPass(commandBuffer);

    vkEndCommandBuffer(commandBuffer);

    return &commandBuffer;
}

void BloomDemo::update(float time) {
    glfwSetWindowTitle(window, fmt::format("{} - FPS {}", title, framePerSecond).c_str());
    if(!ImGui::IsAnyItemActive()){
        cameraController->update(time);
    }
}

void BloomDemo::checkAppInputs() {
    cameraController->processInput();
}

void BloomDemo::cleanup() {
    // TODO save pipeline cache
    VulkanBaseApp::cleanup();
}


void BloomDemo::onPause() {
    VulkanBaseApp::onPause();
}

void BloomDemo::initCamera() {
    FirstPersonSpectatorCameraSettings settings;
    settings.aspectRatio = swapChain.aspectRatio();
    settings.fieldOfView = 60.0f;
    cameraController = std::make_unique<SpectatorCameraController>(device, swapChainImageCount, currentImageIndex
            , dynamic_cast<InputManager&>(*this), settings);
    cameraController->lookAt({5, 5, 5}, glm::vec3(0, 0, 0), {0, 1, 0});
}

void BloomDemo::initLights() {
    Light light{ {0.0f, 0.5f,  1.5f}, {5.0f,   5.0f,  5.0f}};
    lights.push_back(light);

    light = Light{ {-4.0f, 0.5f, -3.0f}, {10.0f,  0.0f,  0.0f} };
    lights.push_back(light);

    light = Light{ {3.0f, 0.5f,  1.0f}, {0.0f,   0.0f,  15.0f} };
    lights.push_back(light);

    light = Light{ {-.8f,  2.4f, -1.0f}, {0.0f,   5.0f,  0.0f} };
    lights.push_back(light);

    lightBuffer = device.createDeviceLocalBuffer(lights.data(), BYTE_SIZE(lights), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
}

void BloomDemo::createDescriptorSetLayouts() {
    textureLayoutSet =
        device.descriptorSetLayoutBuilder()
            .binding(0)
                .descriptorType(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
                .descriptorCount(1)
                .shaderStages(VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT)
                .immutableSamplers(linearSampler)
        .createLayout();

    lightLayoutSet =
        device.descriptorSetLayoutBuilder()
            .binding(0)
                .descriptorType(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
                .descriptorCount(1)
                .shaderStages(VK_SHADER_STAGE_FRAGMENT_BIT)
        .createLayout();

    blur.imageSetLayout =
        device.descriptorSetLayoutBuilder()
            .binding(0)
            .descriptorType(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
            .descriptorCount(1)
            .shaderStages(VK_SHADER_STAGE_COMPUTE_BIT)
        .createLayout();

    postProcess.setLayout =
        device.descriptorSetLayoutBuilder()
            .binding(0)
                .descriptorType(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
                .descriptorCount(1)
                .shaderStages(VK_SHADER_STAGE_COMPUTE_BIT)
                .immutableSamplers(globalSampler)
            .binding(1)
                .descriptorType(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
                .descriptorCount(1)
                .shaderStages(VK_SHADER_STAGE_COMPUTE_BIT)
                .immutableSamplers(globalSampler)
            .binding(2)
                .descriptorType(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
                .descriptorCount(1)
                .shaderStages(VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)
            .createLayout();

}

void BloomDemo::initTextures() {
    textures::fromFile(device, textures.wood, resource("wood.png"), true, VK_FORMAT_R8G8B8A8_SRGB);
    textures::fromFile(device, textures.container, resource("container.png"), true, VK_FORMAT_R8G8B8A8_SRGB);
}

void BloomDemo::updateDescriptorSets() {
    auto sets = descriptorPool.allocate(
            { textureLayoutSet, textureLayoutSet, textureLayoutSet, lightLayoutSet
              ,textureLayoutSet, textureLayoutSet, blur.imageSetLayout, postProcess.setLayout });
    descriptorSets.wood = sets[0];
    device.setName<VK_OBJECT_TYPE_DESCRIPTOR_SET>("wood", descriptorSets.wood);

    descriptorSets.container = sets[1];
    device.setName<VK_OBJECT_TYPE_DESCRIPTOR_SET>("container", descriptorSets.container);

    quad.descriptorSet = sets[2];
    device.setName<VK_OBJECT_TYPE_DESCRIPTOR_SET>("final_render", quad.descriptorSet);

    descriptorSets.light = sets[3];
    device.setName<VK_OBJECT_TYPE_DESCRIPTOR_SET>("lights", descriptorSets.light);

    std::vector<VkWriteDescriptorSet> writes = initializers::writeDescriptorSets<3>();

    writes[0].dstSet = descriptorSets.wood;
    writes[0].dstBinding = 0;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[0].descriptorCount = 1;

    VkDescriptorImageInfo woodImageInfo{textures.wood.sampler, textures.wood.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    writes[0].pImageInfo = &woodImageInfo;

    writes[1].dstSet = descriptorSets.container;
    writes[1].dstBinding = 0;
    writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[1].descriptorCount = 1;

    VkDescriptorImageInfo containerImageInfo{textures.container.sampler, textures.container.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    writes[1].pImageInfo = &containerImageInfo;

    writes[2].dstSet = descriptorSets.light;
    writes[2].dstBinding = 0;
    writes[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[2].descriptorCount = 1;

    VkDescriptorBufferInfo lightsInfo{ lightBuffer, 0, VK_WHOLE_SIZE};
    writes[2].pBufferInfo = &lightsInfo;

    device.updateDescriptorSets(writes);

    // update blur set
    blur.intensitySet = sets[4];
    device.setName<VK_OBJECT_TYPE_DESCRIPTOR_SET>("blur_intensity_in", blur.intensitySet);

    blur.inSet = sets[5];
    device.setName<VK_OBJECT_TYPE_DESCRIPTOR_SET>("blur_in", blur.inSet);

    blur.outSet = sets[6];
    device.setName<VK_OBJECT_TYPE_DESCRIPTOR_SET>("blur_out", blur.outSet);

    writes = initializers::writeDescriptorSets<3>();
    writes[0].dstSet = blur.intensitySet;
    writes[0].dstBinding = 0;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[0].descriptorCount = 1;

    VkDescriptorImageInfo intensityInImageInfo{VK_NULL_HANDLE, scene.intensityAttachment.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    writes[0].pImageInfo = &intensityInImageInfo;

    writes[1].dstSet = blur.inSet;
    writes[1].dstBinding = 0;
    writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[1].descriptorCount = 1;

    VkDescriptorImageInfo blurInImageInfo{VK_NULL_HANDLE, blur.texture.imageView, VK_IMAGE_LAYOUT_GENERAL};
    writes[1].pImageInfo = &blurInImageInfo;

    writes[2].dstSet = blur.outSet;
    writes[2].dstBinding = 0;
    writes[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    writes[2].descriptorCount = 1;

    VkDescriptorImageInfo blurOutImageInfo{VK_NULL_HANDLE, blur.texture.imageView, VK_IMAGE_LAYOUT_GENERAL};
    writes[2].pImageInfo = &blurOutImageInfo;

    device.updateDescriptorSets(writes);

    // update post process set
    postProcess.descriptorSet = sets[7];
    device.setName<VK_OBJECT_TYPE_DESCRIPTOR_SET>("post_process", postProcess.descriptorSet);

    writes = initializers::writeDescriptorSets<3>();

    writes[0].dstSet = postProcess.descriptorSet;
    writes[0].dstBinding = 0;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[0].descriptorCount = 1;

    VkDescriptorImageInfo colorImageInfo{VK_NULL_HANDLE, scene.colorAttachment.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    writes[0].pImageInfo = &colorImageInfo;

    writes[1].dstSet = postProcess.descriptorSet;
    writes[1].dstBinding = 1;
    writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[1].descriptorCount = 1;

    VkDescriptorImageInfo intensityInfo{VK_NULL_HANDLE, blur.texture.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    writes[1].pImageInfo = &intensityInfo;

    writes[2].dstSet = postProcess.descriptorSet;
    writes[2].dstBinding = 2;
    writes[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    writes[2].descriptorCount = 1;

    VkDescriptorImageInfo outImageInfo{VK_NULL_HANDLE, postProcess.texture.imageView, VK_IMAGE_LAYOUT_GENERAL};
    writes[2].pImageInfo = &outImageInfo;

    device.updateDescriptorSets(writes);

    // quad, final rendering output
    writes = initializers::writeDescriptorSets<1>();
    writes[0].dstSet = quad.descriptorSet;
    writes[0].dstBinding = 0;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[0].descriptorCount = 1;

    VkDescriptorImageInfo  imageInfo{VK_NULL_HANDLE, postProcess.texture.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    writes[0].pImageInfo = &imageInfo;
    device.updateDescriptorSets(writes);

}

void BloomDemo::createSceneObjects() {
    auto cubeMesh = primitives::cube();

    glm::mat4 scale = glm::scale(glm::mat4(1), glm::vec3(2));
    for(auto& v : cubeMesh.vertices){
        v.position = scale * v.position;
    }

    cube.vertices = device.createDeviceLocalBuffer(cubeMesh.vertices.data(), BYTE_SIZE(cubeMesh.vertices), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    cube.indices = device.createDeviceLocalBuffer(cubeMesh.indices.data(), BYTE_SIZE(cubeMesh.indices), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);

    // create floor instance
    glm::mat4 model{1};
    model = glm::translate(model, glm::vec3(0.0f, -1.0f, 0.0));
    model = glm::scale(model, glm::vec3(12.5f, 0.5f, 12.5f));
    cube.instanceTransforms.push_back(model);

    // create scenary cubes
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0.0f, 1.5f, 0.0));
    model = glm::scale(model, glm::vec3(0.5f));
    cube.instanceTransforms.push_back(model);

    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(2.0f, 0.0f, 1.0));
    model = glm::scale(model, glm::vec3(0.5f));
    cube.instanceTransforms.push_back(model);

    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(-1.0f, -1.0f, 2.0));
    model = glm::rotate(model, glm::radians(60.0f), glm::normalize(glm::vec3(1.0, 0.0, 1.0)));
    cube.instanceTransforms.push_back(model);

    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0.0f, 2.7f, 4.0));
    model = glm::rotate(model, glm::radians(23.0f), glm::normalize(glm::vec3(1.0, 0.0, 1.0)));
    model = glm::scale(model, glm::vec3(1.25));
    cube.instanceTransforms.push_back(model);

    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(-2.0f, 1.0f, -3.0));
    model = glm::rotate(model, glm::radians(124.0f), glm::normalize(glm::vec3(1.0, 0.0, 1.0)));
    cube.instanceTransforms.push_back(model);

    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(-3.0f, 0.0f, 0.0));
    model = glm::scale(model, glm::vec3(0.5f));
    cube.instanceTransforms.push_back(model);

}

void BloomDemo::initQuad() {
    VkDeviceSize size = sizeof(glm::vec2) * ClipSpace::Quad::positions.size();
    auto data = ClipSpace::Quad::positions;
    quad.vertices = device.createDeviceLocalBuffer(data.data(), BYTE_SIZE(data), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
}

void BloomDemo::renderUI(VkCommandBuffer commandBuffer) {
    ImGui::Begin("settings");
    ImGui::SetWindowSize({250, 150});

    static bool bloomOn = true;
    ImGui::Checkbox("bloom", &bloomOn);
    if(bloomOn){
        ImGui::SliderInt("blur steps", &blur.steps, 1, 10);
    }

    static bool gammaOn = true;
    ImGui::Checkbox("gamma correct", &gammaOn);

    static bool hdrOn = true;
    ImGui::Checkbox("hdr", &hdrOn);
    if(hdrOn){
        ImGui::SliderFloat("exposure", &postProcess.constants.exposure, 0.1f, 5.0f);
    }


    ImGui::End();

    plugin(IM_GUI_PLUGIN).draw(commandBuffer);

    postProcess.constants.gammaOn = static_cast<int>(gammaOn);
    postProcess.constants.hdrOn = static_cast<int>(hdrOn);
    postProcess.constants.bloomOn = static_cast<int>(bloomOn);
}

void BloomDemo::renderOffScreenFramebuffer(VkCommandBuffer commandBuffer) {

    renderScene(commandBuffer);
    blurImage(commandBuffer);
    applyPostProcessing(commandBuffer);

}

void BloomDemo::renderScene(VkCommandBuffer commandBuffer) {
    static std::array<VkClearValue, 3> clearValues;
    clearValues[0].depthStencil = {1.0, 0u};
    clearValues[1].color = {0, 0, 0, 1};
    clearValues[2].color = {0, 0, 0, 1};


    VkRenderPassBeginInfo rPassInfo = initializers::renderPassBeginInfo();
    rPassInfo.clearValueCount = COUNT(clearValues);
    rPassInfo.pClearValues = clearValues.data();
    rPassInfo.framebuffer = scene.framebuffer;
    rPassInfo.renderArea.offset = {0u, 0u};
    rPassInfo.renderArea.extent = swapChain.extent;
    rPassInfo.renderPass = scene.renderPass;

    vkCmdBeginRenderPass(commandBuffer, &rPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    cameraController->setModel(glm::mat4(1));

    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, cube.vertices, &offset);
    vkCmdBindIndexBuffer(commandBuffer, cube.indices, 0, VK_INDEX_TYPE_UINT32);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, scene.pipeline);

    static std::array<VkDescriptorSet, 2> sets;
    sets[0] = descriptorSets.light;
    sets[1] = descriptorSets.wood;
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, scene.layout, 0, COUNT(sets), sets.data(), 0, VK_NULL_HANDLE);
    cameraController->push(commandBuffer, scene.layout, cube.instanceTransforms[0]);
    vkCmdDrawIndexed(commandBuffer, cube.indices.size/sizeof(uint32_t), 1, 0, 0, 0);

    sets[1] = descriptorSets.container;
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, scene.layout, 0, COUNT(sets), sets.data(), 0, VK_NULL_HANDLE);
    for(int i = 1; i < cube.instanceTransforms.size(); i++){
        cameraController->push(commandBuffer, scene.layout, cube.instanceTransforms[i]);
        vkCmdDrawIndexed(commandBuffer, cube.indices.size/sizeof(uint32_t), 1, 0, 0, 0);
    }

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, lightRender.pipeline);
    for(auto& light : lights){
        auto model = glm::translate(glm::mat4(1), light.position);
        model = glm::scale(model, glm::vec3(0.5));
        cameraController->push(commandBuffer, lightRender.layout, model);
        vkCmdPushConstants(commandBuffer, lightRender.layout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(Camera), sizeof(glm::vec3), &light.position);
        vkCmdDrawIndexed(commandBuffer, cube.indices.size/sizeof(uint32_t), 1, 0, 0, 0);
    }

    vkCmdEndRenderPass(commandBuffer);
}

void BloomDemo::blurImage(VkCommandBuffer commandBuffer) {
    static std::array<VkDescriptorSet, 2> sets;
    sets[0] = blur.intensitySet;
    sets[1] = blur.outSet;

    // TODO blits intensity image for blur
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, blur.pipeline);
    int blurAmount = blur.steps;
    for(int i = 0; i < blurAmount; i++){
        blur.constants.horizontal = i%2 == 0;
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, blur.layout, 0, 2, sets.data(), 0, VK_NULL_HANDLE);
        vkCmdPushConstants(commandBuffer, blur.layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(blur.constants), &blur.constants);
        vkCmdDispatch(commandBuffer, swapChain.width(), swapChain.height(), 1);

        if(i < blurAmount - 1) {
            VkImageMemoryBarrier barrier = initializers::ImageMemoryBarrier();
            barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.image = blur.texture.image;
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.layerCount = 1;
            barrier.subresourceRange.levelCount = 1;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.baseArrayLayer = 0;
            vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                 VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, VK_NULL_HANDLE, 0, VK_NULL_HANDLE, 1,
                                 &barrier);
        }

        sets[0] = blur.inSet;
    }

    VkImageMemoryBarrier barrier = initializers::ImageMemoryBarrier();
    barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = blur.texture.image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.baseArrayLayer = 0;
    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT , VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT , 0
            , 0, VK_NULL_HANDLE, 0, VK_NULL_HANDLE, 1, &barrier);
}

void BloomDemo::applyPostProcessing(VkCommandBuffer commandBuffer) {
    // Transition back to general after use, but not on first use
    static bool firstUse = true;
    if(!firstUse){
        VkImageMemoryBarrier barrier = initializers::ImageMemoryBarrier();
        barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        barrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = postProcess.texture.image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.layerCount = 1;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.baseMipLevel = 0;
        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0
                , 0, VK_NULL_HANDLE, 0, VK_NULL_HANDLE, 1, &barrier);
    }

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, postProcess.pipeline);
    vkCmdPushConstants(commandBuffer, postProcess.layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(postProcess.constants), &postProcess.constants);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, postProcess.layout, 0, 1, &postProcess.descriptorSet, 0, VK_NULL_HANDLE);
    vkCmdDispatch(commandBuffer, swapChain.width(), swapChain.height(), 1);
    firstUse = false;

    VkImageMemoryBarrier barrier = initializers::ImageMemoryBarrier();
    barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = postProcess.texture.image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.baseMipLevel = 0;
    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT , VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT , 0
            , 0, VK_NULL_HANDLE, 0, VK_NULL_HANDLE, 1, &barrier);

    barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    barrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = blur.texture.image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.baseArrayLayer = 0;
    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT , VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT , 0
            , 0, VK_NULL_HANDLE, 0, VK_NULL_HANDLE, 1, &barrier);

}

void BloomDemo::initBlur() {
    // TODO create Image with half of swapChain size
    initComputeImage(blur.texture, swapChain.extent.width, swapChain.extent.height);
    device.setName<VK_OBJECT_TYPE_IMAGE>("blur", blur.texture.image.image);
    device.setName<VK_OBJECT_TYPE_IMAGE_VIEW>("blur", blur.texture.imageView.handle);
}


void BloomDemo::initPostProcessing() {
    initComputeImage(postProcess.texture, swapChain.extent.width, swapChain.extent.height);
    device.setName<VK_OBJECT_TYPE_IMAGE>("post_process", postProcess.texture.image.image);
    device.setName<VK_OBJECT_TYPE_IMAGE_VIEW>("post_process", postProcess.texture.imageView.handle);
}

void BloomDemo::initComputeImage(Texture &texture, uint32_t width, uint32_t height) {
    VkImageCreateInfo info = initializers::imageCreateInfo(VK_IMAGE_TYPE_2D, VK_FORMAT_R32G32B32A32_SFLOAT,
                                                           VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, width, height, 1);

    texture.image = device.createImage(info, VMA_MEMORY_USAGE_GPU_ONLY);
    commandPool.oneTimeCommand( [&](auto commandBuffer) {
        auto barrier = initializers::ImageMemoryBarrier();
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = texture.image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0,
                             0, nullptr, 0, nullptr, 1, &barrier);

    });

    VkImageSubresourceRange subresourceRange{};
    subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceRange.baseMipLevel = 0;
    subresourceRange.levelCount = 1;
    subresourceRange.baseArrayLayer = 0;
    subresourceRange.layerCount = 1;

    texture.imageView = texture.image.createView(VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_VIEW_TYPE_2D,
                                                           subresourceRange);
}

void BloomDemo::createGlobalSampler() {
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_NEAREST;
    samplerInfo.minFilter = VK_FILTER_NEAREST;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST ;

    globalSampler = device.createSampler(samplerInfo);

    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    linearSampler = device.createSampler(samplerInfo);
}

void BloomDemo::initSceneFrameBuffer() {
    VkFormat format = VK_FORMAT_R32G32B32A32_SFLOAT;
    auto depthFormat = findDepthFormat();
    auto width = swapChain.extent.width;
    auto height = swapChain.extent.height;
    auto createAttachments = [&]{

        auto subresourceRange = initializers::imageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);
        auto createImageInfo = initializers::imageCreateInfo(VK_IMAGE_TYPE_2D, format,
                                                             VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
                                                             | VK_IMAGE_USAGE_SAMPLED_BIT,
                                                             width,
                                                             height);

        // color attachment
        scene.colorAttachment.image = device.createImage(createImageInfo);
        scene.colorAttachment.imageView = scene.colorAttachment.image.createView(format, VK_IMAGE_VIEW_TYPE_2D, subresourceRange);
        device.setName<VK_OBJECT_TYPE_IMAGE>("color", scene.colorAttachment.image.image);
        device.setName<VK_OBJECT_TYPE_IMAGE_VIEW>("color", scene.colorAttachment.imageView.handle);

        // intensity attachment
        scene.intensityAttachment.image = device.createImage(createImageInfo);
        scene.intensityAttachment.imageView = scene.intensityAttachment.image.createView(format, VK_IMAGE_VIEW_TYPE_2D, subresourceRange);
        device.setName<VK_OBJECT_TYPE_IMAGE>("intensity", scene.intensityAttachment.image.image);
        device.setName<VK_OBJECT_TYPE_IMAGE_VIEW>("intensity", scene.intensityAttachment.imageView.handle);

        // depth attachment
        createImageInfo.format = depthFormat;
        createImageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        scene.depthAttachment.image = device.createImage(createImageInfo);
        scene.depthAttachment.imageView = scene.depthAttachment.image.createView(depthFormat, VK_IMAGE_VIEW_TYPE_2D, subresourceRange);
        device.setName<VK_OBJECT_TYPE_IMAGE>("depth", scene.depthAttachment.image.image);
        device.setName<VK_OBJECT_TYPE_IMAGE_VIEW>("depth", scene.depthAttachment.imageView.handle);

    };

    auto createRenderPass = [&]{
        std::vector<VkAttachmentDescription> attachments{
                {   // depth buffer attachment
                    0,
                    depthFormat,
                    VK_SAMPLE_COUNT_1_BIT,
                    VK_ATTACHMENT_LOAD_OP_CLEAR,
                    VK_ATTACHMENT_STORE_OP_DONT_CARE,
                    VK_ATTACHMENT_LOAD_OP_DONT_CARE,    // stencil load op
                    VK_ATTACHMENT_STORE_OP_DONT_CARE,   // stencil store op
                    VK_IMAGE_LAYOUT_UNDEFINED,
                    VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                },
                {   // color buffer attachment
                    0,
                    format,
                    VK_SAMPLE_COUNT_1_BIT,
                    VK_ATTACHMENT_LOAD_OP_CLEAR,
                    VK_ATTACHMENT_STORE_OP_STORE,
                    VK_ATTACHMENT_LOAD_OP_DONT_CARE,    // stencil load op
                    VK_ATTACHMENT_STORE_OP_DONT_CARE,   // stencil store op
                    VK_IMAGE_LAYOUT_UNDEFINED,
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                },
                {   // intensity buffer attachment
                    0,
                    format,
                    VK_SAMPLE_COUNT_1_BIT,
                    VK_ATTACHMENT_LOAD_OP_CLEAR,
                    VK_ATTACHMENT_STORE_OP_STORE,
                    VK_ATTACHMENT_LOAD_OP_DONT_CARE,    // stencil load op
                    VK_ATTACHMENT_STORE_OP_DONT_CARE,   // stencil store op
                    VK_IMAGE_LAYOUT_UNDEFINED,
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                }
        };

        std::vector<SubpassDescription> subpasses(1);
        subpasses[0].colorAttachments = { {1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL}, {2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL} };
        subpasses[0].depthStencilAttachments = VkAttachmentReference {0, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

        std::vector<VkSubpassDependency> dependencies(2);
        dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[0].dstSubpass = 0;
        dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dependencies[0].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        dependencies[0].dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        dependencies[1].srcSubpass = 0;
        dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[1].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dependencies[1].srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        scene.renderPass = device.createRenderPass(attachments, subpasses, dependencies);
    };

    auto createFrameBuffer = [&]{
        std::vector<VkImageView> attachments{
            scene.depthAttachment.imageView,
            scene.colorAttachment.imageView,
            scene.intensityAttachment.imageView
        };
        scene.framebuffer = device.createFramebuffer(scene.renderPass, attachments, width, height);
        device.setName<VK_OBJECT_TYPE_FRAMEBUFFER>("scene", scene.framebuffer.frameBuffer);
    };

    createAttachments();
    createRenderPass();
    createFrameBuffer();
}


int main(){
    try{

        Settings settings;
        settings.depthTest = true;
        auto app = BloomDemo{ settings };

        std::unique_ptr<Plugin> plugin = std::make_unique<ImGuiPlugin>();
        app.addPlugin(plugin);
        app.run();
    }catch(std::runtime_error& err){
        spdlog::error(err.what());
    }
}