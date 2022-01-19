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
    initBloomFramebuffer();
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
    subpasses.scene.pipeline =
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
                .subpass(subpasses.scene.subpass)
                .name("scene")
                .pipelineCache(pipelineCache)
            .build(subpasses.scene.layout);

    lightRender.pipeline =
        builder
            .basePipeline(subpasses.scene.pipeline)
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

    subpasses.blur.pipeline  =
        builder
            .allowDerivatives()
            .basePipeline(subpasses.scene.pipeline)
            .shaderStage()
                .vertexShader(load("quad.vert.spv"))
                .fragmentShader(load("blur.frag.spv"))
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
                .addDescriptorSetLayout(blurSetLayout)
            .name("blur")
            .subpass(subpasses.blur.subpass)
        .build(subpasses.blur.layout);


    subpasses.composite.pipeline =
        builder
            .basePipeline(subpasses.blur.pipeline)
            .shaderStage()
                .fragmentShader(load("composite.frag.spv"))
            .layout().clear()
                .addPushConstantRange(VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(compositeConstants))
                .addDescriptorSetLayout(compositeSetLayout)
            .subpass(subpasses.composite.subpass)
            .name("composite")
        .build(subpasses.composite.layout);

    quad.pipeline =
        builder
            .basePipeline(subpasses.blur.pipeline)
            .shaderStage()
                .fragmentShader(load("quad.frag.spv"))
            .layout().clear()
                .addDescriptorSetLayout(textureLayoutSet)
            .name("quad")
            .renderPass(renderPass)
            .subpass(0)
        .build(quad.layout);

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

    blurSetLayout =
        device.descriptorSetLayoutBuilder()
            .binding(0)
                .descriptorType(VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT)
                .descriptorCount(1)
                .shaderStages(VK_SHADER_STAGE_FRAGMENT_BIT)
            .binding(1)
                .descriptorType(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
                .descriptorCount(1)
                .shaderStages(VK_SHADER_STAGE_FRAGMENT_BIT)
            .createLayout();

    compositeSetLayout =
        device.descriptorSetLayoutBuilder()
            .binding(0)
                .descriptorType(VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT)
                .descriptorCount(1)
                .shaderStages(VK_SHADER_STAGE_FRAGMENT_BIT)
            .binding(1)
                .descriptorType(VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT)
                .descriptorCount(1)
                .shaderStages(VK_SHADER_STAGE_FRAGMENT_BIT)
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

    // update blur set
    sets = descriptorPool.allocate({ blurSetLayout, blurSetLayout, compositeSetLayout});
    subpasses.blur.brightnessSet = sets[0];
    subpasses.blur.blurSet = sets[1];
    subpasses.composite.colorAttachmentsSet = sets[2];

    writes = initializers::writeDescriptorSets<2>();

    writes[0].dstSet = subpasses.blur.brightnessSet;
    writes[0].dstBinding = 0;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
    writes[0].descriptorCount = 1;

    VkDescriptorImageInfo brightnessAttachmentInfo{VK_NULL_HANDLE, scene.brightnessAttachment[0].imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    writes[0].pImageInfo = &brightnessAttachmentInfo;

    writes[1].dstSet = subpasses.blur.brightnessSet;
    writes[1].dstBinding = 1;
    writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[1].descriptorCount = 1;

    VkDescriptorImageInfo brightnessImageInfo{scene.sampler, scene.brightnessAttachment[0].imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    writes[1].pImageInfo = &brightnessImageInfo;

    device.updateDescriptorSets(writes);

    // update blur descriptor

    writes[0].dstSet = subpasses.blur.blurSet;
    writes[0].dstBinding = 0;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
    writes[0].descriptorCount = 1;

    brightnessAttachmentInfo = VkDescriptorImageInfo{VK_NULL_HANDLE, scene.brightnessAttachment[1].imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    writes[0].pImageInfo = &brightnessAttachmentInfo;


    writes[1].dstSet = subpasses.blur.blurSet;
    writes[1].dstBinding = 1;
    writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[1].descriptorCount = 1;

    brightnessImageInfo = VkDescriptorImageInfo{scene.sampler, scene.brightnessAttachment[1].imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    writes[1].pImageInfo = &brightnessImageInfo;
    device.updateDescriptorSets(writes);

    // TODO composite writes
    writes = initializers::writeDescriptorSets<2>();
    writes[0].dstSet = subpasses.composite.colorAttachmentsSet;
    writes[0].dstBinding = 0;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
    writes[0].descriptorCount = 1;

    VkDescriptorImageInfo colorImageInfo{VK_NULL_HANDLE, scene.colorAttachment.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    writes[0].pImageInfo = &colorImageInfo;

    writes[1].dstSet = subpasses.composite.colorAttachmentsSet;
    writes[1].dstBinding = 1;
    writes[1].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
    writes[1].descriptorCount = 1;

    VkDescriptorImageInfo blurImageInfo{VK_NULL_HANDLE, scene.brightnessAttachment[0].imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    writes[1].pImageInfo = &blurImageInfo;
    device.updateDescriptorSets(writes);

    // final image writes
    writes = initializers::writeDescriptorSets<1>();

    writes[0].dstSet = quad.descriptorSet;
    writes[0].dstBinding = 0;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[0].descriptorCount =  1;

    VkDescriptorImageInfo finalImageInfo{testTexture.sampler, testTexture.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
//    VkDescriptorImageInfo finalImageInfo{scene.sampler, scene.compositeAttachment.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    writes[0].pImageInfo = &finalImageInfo;

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

void BloomDemo::initBloomFramebuffer() {
    const auto colorFormat = VK_FORMAT_R32G32B32A32_SFLOAT;
    const auto depthFormat = findDepthFormat();

    auto createFrameBufferAttachments = [&] {
        auto subresourceRange = initializers::imageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);

        auto createImageInfo = initializers::imageCreateInfo(VK_IMAGE_TYPE_2D, colorFormat,
                                                                 VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
                                                                    | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
                                                             swapChain.width(),
                                                             swapChain.height());

        scene.colorAttachment.image = device.createImage(createImageInfo);
        scene.colorAttachment.imageView = scene.colorAttachment.image.createView(colorFormat, VK_IMAGE_VIEW_TYPE_2D,
                                                                                 subresourceRange);


        createImageInfo.usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
        scene.brightnessAttachment[0].image = device.createImage(createImageInfo);
        scene.brightnessAttachment[0].imageView = scene.brightnessAttachment[0].image.createView(colorFormat,
                                                                                                 VK_IMAGE_VIEW_TYPE_2D,
                                                                                                 subresourceRange);

        scene.brightnessAttachment[1].image = device.createImage(createImageInfo);
        scene.brightnessAttachment[1].imageView = scene.brightnessAttachment[1].image.createView(colorFormat,
                                                                                                 VK_IMAGE_VIEW_TYPE_2D,
                                                                                                 subresourceRange);
        scene.compositeAttachment.image = device.createImage(createImageInfo);
        scene.compositeAttachment.imageView = scene.colorAttachment.image.createView(colorFormat, VK_IMAGE_VIEW_TYPE_2D,
                                                                                     subresourceRange);

        createImageInfo.format = depthFormat;
        createImageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        scene.depthAttachment.image = device.createImage(createImageInfo);
        scene.depthAttachment.imageView = scene.depthAttachment.image.createView(depthFormat, VK_IMAGE_VIEW_TYPE_2D,
                                                                                 subresourceRange);
    };

    auto createRenderPass = [&]{
        VkAttachmentDescription colorAttachment{
            0, // flags
            colorFormat,
            settings.msaaSamples, // TODO may need to add resolve if more than 1
            VK_ATTACHMENT_LOAD_OP_CLEAR,
            VK_ATTACHMENT_STORE_OP_STORE,
            VK_ATTACHMENT_LOAD_OP_DONT_CARE,    // stencil load op
            VK_ATTACHMENT_STORE_OP_DONT_CARE,   // stencil store op
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_GENERAL
        };

        auto brightnessAttachment = colorAttachment;
        auto blurredAttachment = colorAttachment;
        auto compositeAttachment = colorAttachment;

        auto depthAttachment = colorAttachment;
        depthAttachment.format = depthFormat;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        std::vector<VkAttachmentDescription> attachments{
            depthAttachment,
            colorAttachment,
            brightnessAttachment,
            blurredAttachment,
            compositeAttachment
        };

        static constexpr uint32_t DEPTH_ATTACHMENT = 0;
        static constexpr uint32_t COLOR_ATTACHMENT = 1;
        static constexpr uint32_t BRIGHTNESS_ATTACHMENT = 2;
        static constexpr uint32_t BLURRED_ATTACHMENT = 3;
        static constexpr uint32_t COMPOSITE_ATTACHMENT = 4;

        std::vector<SubpassDescription> subpasses{};
        SubpassDescription sceneSubpass{};
        sceneSubpass.colorAttachments.push_back({COLOR_ATTACHMENT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});
        sceneSubpass.colorAttachments.push_back({BRIGHTNESS_ATTACHMENT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});
        sceneSubpass.depthStencilAttachments = VkAttachmentReference{DEPTH_ATTACHMENT, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
        static constexpr uint32_t SUBPASS_SCENE = 0;
        subpasses.push_back(sceneSubpass);

        SubpassDescription blurSubpass{};
        blurSubpass.inputAttachments.push_back({BRIGHTNESS_ATTACHMENT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL});
        blurSubpass.colorAttachments.push_back({BLURRED_ATTACHMENT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});
        blurSubpass.preserveAttachments.push_back(COLOR_ATTACHMENT);
        static constexpr uint32_t SUBPASS_BLUR = 1;
        subpasses.push_back(blurSubpass);


        SubpassDescription compositeSubpass{};
        compositeSubpass.inputAttachments.push_back({COLOR_ATTACHMENT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL});
        compositeSubpass.inputAttachments.push_back({BLURRED_ATTACHMENT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL});
        compositeSubpass.colorAttachments.push_back({COMPOSITE_ATTACHMENT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});
        static constexpr uint32_t SUBPASS_COMPOSITE = 2;
        subpasses.push_back(compositeSubpass);

        static constexpr uint32_t EXTERNAL_TO_SCENE = 0;
        static constexpr uint32_t SCENE_TO_BLUR = 1;
        static constexpr uint32_t BLUR_TO_BLUR = 2;
        static constexpr uint32_t BLUR_TO_COMPOSITE = 3;
        static constexpr uint32_t COMPOSITE_TO_EXTERNAL = 4;

        std::vector<VkSubpassDependency> dependencies(5);
        dependencies[EXTERNAL_TO_SCENE].srcSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[EXTERNAL_TO_SCENE].dstSubpass = SUBPASS_SCENE;
        dependencies[EXTERNAL_TO_SCENE].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dependencies[EXTERNAL_TO_SCENE].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dependencies[EXTERNAL_TO_SCENE].srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        dependencies[EXTERNAL_TO_SCENE].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        dependencies[EXTERNAL_TO_SCENE].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        dependencies[SCENE_TO_BLUR].srcSubpass = SUBPASS_SCENE;
        dependencies[SCENE_TO_BLUR].dstSubpass = SUBPASS_BLUR;
        dependencies[SCENE_TO_BLUR].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dependencies[SCENE_TO_BLUR].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dependencies[SCENE_TO_BLUR].srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        dependencies[SCENE_TO_BLUR].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        dependencies[SCENE_TO_BLUR].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        dependencies[BLUR_TO_BLUR].srcSubpass = SUBPASS_BLUR;
        dependencies[BLUR_TO_BLUR].dstSubpass = SUBPASS_BLUR;
        dependencies[BLUR_TO_BLUR].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dependencies[BLUR_TO_BLUR].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dependencies[BLUR_TO_BLUR].srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        dependencies[BLUR_TO_BLUR].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        dependencies[BLUR_TO_BLUR].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        dependencies[BLUR_TO_COMPOSITE].srcSubpass = SUBPASS_BLUR;
        dependencies[BLUR_TO_COMPOSITE].dstSubpass = SUBPASS_COMPOSITE;
        dependencies[BLUR_TO_COMPOSITE].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dependencies[BLUR_TO_COMPOSITE].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dependencies[BLUR_TO_COMPOSITE].srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        dependencies[BLUR_TO_COMPOSITE].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        dependencies[BLUR_TO_COMPOSITE].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
        
        dependencies[COMPOSITE_TO_EXTERNAL].srcSubpass = SUBPASS_COMPOSITE;
        dependencies[COMPOSITE_TO_EXTERNAL].dstSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[COMPOSITE_TO_EXTERNAL].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dependencies[COMPOSITE_TO_EXTERNAL].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dependencies[COMPOSITE_TO_EXTERNAL].srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        dependencies[COMPOSITE_TO_EXTERNAL].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        dependencies[COMPOSITE_TO_EXTERNAL].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        scene.renderPass = device.createRenderPass(attachments, subpasses, dependencies);
        device.setName<VK_OBJECT_TYPE_RENDER_PASS>("scene", scene.renderPass.renderPass);
    };

    auto createFrameBuffer = [&]{
        assert(scene.renderPass.renderPass);
        std::vector<VkImageView> imageViews;
        imageViews.push_back(scene.depthAttachment.imageView);
        imageViews.push_back(scene.colorAttachment.imageView);
        imageViews.push_back(scene.brightnessAttachment[0].imageView);
        imageViews.push_back(scene.brightnessAttachment[1].imageView);
        imageViews.push_back(scene.compositeAttachment.imageView);
        scene.framebuffer = device.createFramebuffer(scene.renderPass, imageViews, swapChain.width(), swapChain.height());
        device.setName<VK_OBJECT_TYPE_FRAMEBUFFER>("scene", scene.framebuffer.frameBuffer);
    };

    auto createSampler = [&]{
        VkSamplerCreateInfo createInfo = initializers::samplerCreateInfo();
        createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        createInfo.magFilter = VK_FILTER_LINEAR;
        createInfo.minFilter = VK_FILTER_LINEAR;
        createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        createInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        scene.sampler = device.createSampler(createInfo);
    };

    createFrameBufferAttachments();
    createRenderPass();
    createFrameBuffer();
    createSampler();
    textures::fromFile(device, testTexture, resource("portrait.jpg"));
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
    static std::array<VkClearValue, 5> clearValues;
    clearValues[0].depthStencil = {1.0, 0u};
    clearValues[1].color = {0, 0, 0, 1};
    clearValues[2].color = {0, 0, 0, 1};
    clearValues[3].color = {0, 0, 0, 1};
    clearValues[4].color = {0, 0, 0, 1};

    VkRenderPassBeginInfo rPassInfo = initializers::renderPassBeginInfo();
    rPassInfo.clearValueCount = COUNT(clearValues);
    rPassInfo.pClearValues = clearValues.data();
    rPassInfo.framebuffer = scene.framebuffer;
    rPassInfo.renderArea.offset = {0u, 0u};
    rPassInfo.renderArea.extent = swapChain.extent;
    rPassInfo.renderPass = scene.renderPass;

    vkCmdBeginRenderPass(commandBuffer, &rPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    renderScene(commandBuffer);
    applyBloom(commandBuffer);

    vkCmdEndRenderPass(commandBuffer);
}

void BloomDemo::renderScene(VkCommandBuffer commandBuffer) {
    cameraController->setModel(glm::mat4(1));

    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, cube.vertices, &offset);
    vkCmdBindIndexBuffer(commandBuffer, cube.indices, 0, VK_INDEX_TYPE_UINT32);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, subpasses.scene.pipeline);

    static std::array<VkDescriptorSet, 2> sets;
    sets[0] = descriptorSets.light;
    sets[1] = descriptorSets.wood;
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, subpasses.scene.layout, 0, COUNT(sets), sets.data(), 0, VK_NULL_HANDLE);
    cameraController->push(commandBuffer, subpasses.scene.layout, cube.instanceTransforms[0]);
    vkCmdDrawIndexed(commandBuffer, cube.indices.size/sizeof(uint32_t), 1, 0, 0, 0);

    sets[1] = descriptorSets.container;
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, subpasses.scene.layout, 0, COUNT(sets), sets.data(), 0, VK_NULL_HANDLE);
    for(int i = 1; i < cube.instanceTransforms.size(); i++){
        cameraController->push(commandBuffer, subpasses.scene.layout, cube.instanceTransforms[i]);
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
}

void BloomDemo::applyBloom(VkCommandBuffer commandBuffer) {
    vkCmdNextSubpass(commandBuffer, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, subpasses.blur.pipeline);
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, quad.vertices, &offset);

    int horizontal = 0;
    static int blurAmount = 10;
    for(int i = 0; i < blurAmount; i++){
        horizontal = i%2;
        auto set = horizontal == 0 ? subpasses.blur.brightnessSet : subpasses.blur.blurSet;
        vkCmdPushConstants(commandBuffer, subpasses.blur.layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(int), &horizontal);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, subpasses.blur.layout, 0, 1, &set, 0, VK_NULL_HANDLE);
        vkCmdDraw(commandBuffer, quad.vertices.size/sizeof(glm::vec2), 1, 0, 0);
    }

    vkCmdNextSubpass(commandBuffer, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, subpasses.composite.pipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, subpasses.composite.layout, 0, 1, &subpasses.composite.colorAttachmentsSet, 0, VK_NULL_HANDLE);
    vkCmdDraw(commandBuffer, quad.vertices.size/sizeof(glm::vec2), 1, 0, 0);

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