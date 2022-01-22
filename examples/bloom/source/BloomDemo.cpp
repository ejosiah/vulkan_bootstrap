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
    initSceneFrameBuffer();
    initPostProcessFrameBuffer();
    createDescriptorPool();
    createCommandPool();
    createDescriptorSetLayouts();
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

    postProcess.pipeline  =
        builder
            .allowDerivatives()
            .basePipeline(scene.pipeline)
            .shaderStage()
                .vertexShader(load("quad.vert.spv"))
                .fragmentShader(load("postprocess.frag.spv"))
            .vertexInputState().clear()
                .addVertexBindingDescriptions(ClipSpace::bindingDescription())
                .addVertexAttributeDescriptions(ClipSpace::attributeDescriptions())
            .inputAssemblyState()
                .triangleStrip()
            .rasterizationState()
                .cullNone()
            .colorBlendState()
                .attachments(1)
            .layout().clear()
                .addPushConstantRange(VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(uint32_t))
                .addDescriptorSetLayout(postProcessSetLayout)
            .name("post_process")
            .subpass(0)
        .build(postProcess.layout);
    

    quad.pipeline =
        builder
            .basePipeline(postProcess.pipeline)
            .shaderStage()
                .fragmentShader(load("quad.frag.spv"))
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
                .shaderStages(VK_SHADER_STAGE_FRAGMENT_BIT)
        .createLayout();

    lightLayoutSet =
        device.descriptorSetLayoutBuilder()
            .binding(0)
                .descriptorType(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
                .descriptorCount(1)
                .shaderStages(VK_SHADER_STAGE_FRAGMENT_BIT)
        .createLayout();

    postProcessSetLayout =
        device.descriptorSetLayoutBuilder()
            .binding(0)
                .descriptorType(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
                .descriptorCount(1)
                .shaderStages(VK_SHADER_STAGE_FRAGMENT_BIT)
            .binding(1)
                .descriptorType(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
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
}

void BloomDemo::initTextures() {
    textures::fromFile(device, textures.wood, resource("wood.png"), true, VK_FORMAT_R8G8B8A8_SRGB);
    textures::fromFile(device, textures.container, resource("container.png"), true, VK_FORMAT_R8G8B8A8_SRGB);
}

void BloomDemo::updateDescriptorSets() {
    auto sets = descriptorPool.allocate({ textureLayoutSet, textureLayoutSet, lightLayoutSet, textureLayoutSet});
    descriptorSets.wood = sets[0];
    descriptorSets.container = sets[1];
    descriptorSets.light = sets[2];
    quad.descriptorSet = sets[3];

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

    // TODO update blur set
    
    // TODO post process set


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
    ImGui::SetWindowSize({220, 130});

    static bool bloomOn = true;
    ImGui::Checkbox("bloom", &bloomOn);


    static bool gammaOn = true;
    ImGui::Checkbox("gamma correct", &gammaOn);

    static bool hdrOn = true;
    ImGui::Checkbox("hdr", &hdrOn);
    if(hdrOn){
        ImGui::SliderFloat("exposure", &compositeConstants.exposure, 0.1f, 5.0f);
    }


    ImGui::End();

    plugin(IM_GUI_PLUGIN).draw(commandBuffer);

    compositeConstants.gammaOn = static_cast<int>(gammaOn);
    compositeConstants.hdrOn = static_cast<int>(hdrOn);
    compositeConstants.bloomOn = static_cast<int>(bloomOn);
}

void BloomDemo::renderOffScreenFramebuffer(VkCommandBuffer commandBuffer) {

    renderScene(commandBuffer);
    blurImage(commandBuffer);
    postPrcessing(commandBuffer);

}

void BloomDemo::renderScene(VkCommandBuffer commandBuffer) {
    static std::array<VkClearValue, 2> clearValues;
    clearValues[0].depthStencil = {1.0, 0u};
    clearValues[1].color = {0, 0, 0, 1};


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
        vkCmdPushConstants(commandBuffer, lightRender.layout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(Camera), sizeof(glm::vec3), &light.color);
        vkCmdDrawIndexed(commandBuffer, cube.indices.size/sizeof(uint32_t), 1, 0, 0, 0);
    }

    vkCmdEndRenderPass(commandBuffer);
}

void BloomDemo::postPrcessing(VkCommandBuffer commandBuffer) {
    static std::array<VkClearValue, 1> clearValues;
    clearValues[1].color = {0, 0, 0, 1};


    VkRenderPassBeginInfo rPassInfo = initializers::renderPassBeginInfo();
    rPassInfo.clearValueCount = COUNT(clearValues);
    rPassInfo.pClearValues = clearValues.data();
    rPassInfo.framebuffer = postProcess.framebuffer;
    rPassInfo.renderArea.offset = {0u, 0u};
    rPassInfo.renderArea.extent = swapChain.extent;
    rPassInfo.renderPass = postProcess.renderPass;

    vkCmdBeginRenderPass(commandBuffer, &rPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdEndRenderPass(commandBuffer);

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