#include "SsaoDemo.hpp"
#include "GraphicsPipelineBuilder.hpp"
#include "ImGuiPlugin.hpp"

SsaoDemo::SsaoDemo(const Settings& settings) : VulkanBaseApp("Screen Space Ambiant Occulsion", settings) {
    fileManager.addSearchPath(".");
    fileManager.addSearchPath("../../examples/ssao");
    fileManager.addSearchPath("../../examples/ssao/spv");
    fileManager.addSearchPath("../../examples/ssao/models");
    fileManager.addSearchPath("../../examples/ssao/textures");
    fileManager.addSearchPath("../../data/shaders");
    fileManager.addSearchPath("../../data/models");
    fileManager.addSearchPath("../../data/textures");
    fileManager.addSearchPath("../../data");
}

void SsaoDemo::initApp() {
    createDescriptorPool();
    createCommandPool();
    createSsaoSamplingData();
    createGlobalSampler();
    createDescriptorSetLayout();
    initGBuffer();
    createSsaoFrameBuffer();
    createBlurFrameBuffer();
    initScreenQuad();
    updateDescriptorSet();
    loadModel();
    initCamera();
    createPipelineCache();
    createRenderPipeline();
    createComputePipeline();
}

void SsaoDemo::loadModel() {
    phong::load(resource("leaving_room/living_room.obj"), device, descriptorPool, model);
}

void SsaoDemo::initCamera() {
    FirstPersonSpectatorCameraSettings settings;
    settings.fieldOfView = 60.0f;
    settings.aspectRatio = swapChain.aspectRatio();

    cameraController = std::make_unique<SpectatorCameraController>(device, swapChainImageCount, currentImageIndex
                                                                   , dynamic_cast<InputManager&>(*this), settings);
    auto target = (model.bounds.min + model.bounds.max) * 0.5f;
    auto position = target + glm::vec3(0, 0, 1);
    cameraController->lookAt(position, target, {0, 1, 0});
}

void SsaoDemo::createDescriptorPool() {
    constexpr uint32_t maxSets = 300;
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

void SsaoDemo::createCommandPool() {
    commandPool = device.createCommandPool(*device.queueFamilyIndex.graphics, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    commandBuffers = commandPool.allocateCommandBuffers(swapChainImageCount);
}

void SsaoDemo::createPipelineCache() {
    pipelineCache = device.createPipelineCache();
}


void SsaoDemo::createRenderPipeline() {
    auto builder = device.graphicsPipelineBuilder();
    render.pipeline =
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
                    .attachments(3)
                .layout()
                    .addPushConstantRange(Camera::pushConstant())
                    .addDescriptorSetLayout(model.descriptorSetLayout)
                .renderPass(gBuffer.renderpass)
                .subpass(0)
                .name("render")
                .pipelineCache(pipelineCache)
            .build(render.layout);

    lighting.pipeline =
        builder
            .basePipeline(render.pipeline)
            .shaderStage()
                .vertexShader(load("quad.vert.spv"))
                .fragmentShader(load("lighting.frag.spv"))
            .vertexInputState().clear()
                .addVertexBindingDescriptions(ClipSpace::bindingDescription())
                .addVertexAttributeDescriptions(ClipSpace::attributeDescriptions())
            .inputAssemblyState()
                .triangleStrip()
            .colorBlendState()
                .attachments(1)
            .layout().clear()
                .addPushConstantRange(VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(lighting.constants))
                .addDescriptorSetLayout(ssao.setLayout)
            .renderPass(renderPass)
            .name("lighting_pass")
            .subpass(0)
        .build(lighting.layout);

    ssao.pipeline =
        builder
            .basePipeline(lighting.pipeline)
            .shaderStage()
                .fragmentShader(load("ssao.frag.spv"))
            .layout().clear()
                .addPushConstantRange(VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(glm::mat4) + sizeof(ssao.constants))
                .addDescriptorSetLayout(ssao.setLayout)
            .renderPass(ssao.renderpass)
            .name("ssao")
            .subpass(0)
        .build(ssao.layout);

    ssao.blur.pipeline =
        builder
            .basePipeline(lighting.pipeline)
            .shaderStage()
                .fragmentShader(load("blur.frag.spv"))
            .layout().clear()
                .addPushConstantRange(VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(int))
                .addDescriptorSetLayout(textureSetLayout)
            .renderPass(ssao.blur.renderpass)
            .name("ssao_blur")
            .subpass(0)
        .build(ssao.blur.layout);
}

void SsaoDemo::createComputePipeline() {
    auto module = VulkanShaderModule{ "../../data/shaders/pass_through.comp.spv", device};
    auto stage = initializers::shaderStage({ module, VK_SHADER_STAGE_COMPUTE_BIT});

    compute.layout = device.createPipelineLayout();

    auto computeCreateInfo = initializers::computePipelineCreateInfo();
    computeCreateInfo.stage = stage;
    computeCreateInfo.layout = compute.layout;

    compute.pipeline = device.createComputePipeline(computeCreateInfo, pipelineCache);
}


void SsaoDemo::onSwapChainDispose() {
    dispose(render.pipeline);
    dispose(compute.pipeline);
}

void SsaoDemo::onSwapChainRecreation() {
    createRenderPipeline();
    createComputePipeline();
}

VkCommandBuffer *SsaoDemo::buildCommandBuffers(uint32_t imageIndex, uint32_t &numCommandBuffers) {
    numCommandBuffers = 1;
    auto& commandBuffer = commandBuffers[imageIndex];

    VkCommandBufferBeginInfo beginInfo = initializers::commandBufferBeginInfo();
    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    renderScene(commandBuffer);
    ssaoPass(commandBuffer);
    blurPass(commandBuffer);

    static std::array<VkClearValue, 2> clearValues;
    clearValues[0].color = {0, 0, 1, 1};
    clearValues[1].depthStencil = {1.0, 0u};

    VkRenderPassBeginInfo rPassInfo = initializers::renderPassBeginInfo();
    rPassInfo.clearValueCount = COUNT(clearValues);
    rPassInfo.pClearValues = clearValues.data();
    rPassInfo.framebuffer = framebuffers[imageIndex];
    rPassInfo.renderArea.offset = {0u, 0u};
    rPassInfo.renderArea.extent = swapChain.extent;
    rPassInfo.renderPass = renderPass;

    vkCmdBeginRenderPass(commandBuffer, &rPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, lighting.pipeline);

    lighting.constants.view = cameraController->cam().view;
    vkCmdPushConstants(commandBuffer, lighting.layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(lighting.constants), &lighting.constants);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, lighting.layout, 0, 1, &ssao.descriptorSet, 0, VK_NULL_HANDLE);

    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, quad.vertices, &offset);
    vkCmdDraw(commandBuffer, quad.numVertices, 1, 0, 0);

    renderUI(commandBuffer);

    vkCmdEndRenderPass(commandBuffer);

    vkEndCommandBuffer(commandBuffer);

    return &commandBuffer;
}

void SsaoDemo::renderScene(VkCommandBuffer commandBuffer) {
    static std::array<VkClearValue, 4> clearValues;
    clearValues[0].depthStencil = {1.0, 0u};
    clearValues[1].color = {0, 0, 0, 0};
    clearValues[2].color = {0, 0, 0, 0};
    clearValues[3].color = {0, 0, 0, 0};

    VkRenderPassBeginInfo rPassInfo = initializers::renderPassBeginInfo();
    rPassInfo.clearValueCount = COUNT(clearValues);
    rPassInfo.pClearValues = clearValues.data();
    rPassInfo.framebuffer = gBuffer.framebuffer;
    rPassInfo.renderArea.offset = {0u, 0u};
    rPassInfo.renderArea.extent = swapChain.extent;
    rPassInfo.renderPass = gBuffer.renderpass;

    vkCmdBeginRenderPass(commandBuffer, &rPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, render.pipeline);
    cameraController->push(commandBuffer, render.layout);
    model.draw(commandBuffer, render.layout);

    vkCmdEndRenderPass(commandBuffer);
}

void SsaoDemo::update(float time) {
    if(!ImGui::IsAnyItemActive()) {
        cameraController->update(time);
    }
}

void SsaoDemo::checkAppInputs() {
    cameraController->processInput();
}

void SsaoDemo::cleanup() {
    // TODO save pipeline cache
    VulkanBaseApp::cleanup();
}

void SsaoDemo::onPause() {
    VulkanBaseApp::onPause();
}

void SsaoDemo::initGBuffer() {
    auto colorFormat = VK_FORMAT_R32G32B32A32_SFLOAT;
    auto depthFormat = findDepthFormat();
    auto width = swapChain.width();
    auto height = swapChain.height();

    auto name = [&](const FramebufferAttachment& attachment, const std::string& name){
        device.setName<VK_OBJECT_TYPE_IMAGE>(name, attachment.image.image);
        device.setName<VK_OBJECT_TYPE_IMAGE_VIEW>(name, attachment.imageView.handle);
    };

    auto createAttachments = [&]{
        auto imageInfo = initializers::imageCreateInfo(VK_IMAGE_TYPE_2D, colorFormat
                                                       , VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
                                                       | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT
                                                       | VK_IMAGE_USAGE_SAMPLED_BIT
                                                       , width, height, 1);
        auto subRangeResource = initializers::imageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);

        gBuffer.position.image = device.createImage(imageInfo);
        gBuffer.position.imageView = gBuffer.position.image.createView(colorFormat, VK_IMAGE_VIEW_TYPE_2D, subRangeResource);
        name(gBuffer.position, "g_buffer_color");

        gBuffer.normal.image = device.createImage(imageInfo);
        gBuffer.normal.imageView = gBuffer.normal.image.createView(colorFormat, VK_IMAGE_VIEW_TYPE_2D, subRangeResource);
        name(gBuffer.normal, "g_buffer_normal");

        gBuffer.color.image = device.createImage(imageInfo);
        gBuffer.color.imageView = gBuffer.color.image.createView(colorFormat, VK_IMAGE_VIEW_TYPE_2D, subRangeResource);
        name(gBuffer.color, "g_buffer_color");

        imageInfo.format = depthFormat;
        imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        subRangeResource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        gBuffer.depth.image = device.createImage(imageInfo);
        gBuffer.depth.imageView = gBuffer.depth.image.createView(depthFormat, VK_IMAGE_VIEW_TYPE_2D, subRangeResource);
        name(gBuffer.depth, "g_buffer_depth");

    };

    auto createRenderPass = [&]{
        std::vector<VkAttachmentDescription> attachments = {
                {   // depth buffer attachment
                    0,
                    depthFormat,
                    VK_SAMPLE_COUNT_1_BIT,
                    VK_ATTACHMENT_LOAD_OP_CLEAR,
                    VK_ATTACHMENT_STORE_OP_DONT_CARE,
                    VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                    VK_ATTACHMENT_STORE_OP_DONT_CARE,
                    VK_IMAGE_LAYOUT_UNDEFINED,
                    VK_IMAGE_LAYOUT_GENERAL
                },
                {   // position buffer attachment
                    0,
                    colorFormat,
                    VK_SAMPLE_COUNT_1_BIT,
                    VK_ATTACHMENT_LOAD_OP_CLEAR,
                    VK_ATTACHMENT_STORE_OP_STORE,
                    VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                    VK_ATTACHMENT_STORE_OP_DONT_CARE,
                    VK_IMAGE_LAYOUT_UNDEFINED,
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                },
                {   // normal buffer attachment
                    0,
                    colorFormat,
                    VK_SAMPLE_COUNT_1_BIT,
                    VK_ATTACHMENT_LOAD_OP_CLEAR,
                    VK_ATTACHMENT_STORE_OP_STORE,
                    VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                    VK_ATTACHMENT_STORE_OP_DONT_CARE,
                    VK_IMAGE_LAYOUT_UNDEFINED,
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                },
                {   // color buffer attachment
                    0,
                    colorFormat,
                    VK_SAMPLE_COUNT_1_BIT,
                    VK_ATTACHMENT_LOAD_OP_CLEAR,
                    VK_ATTACHMENT_STORE_OP_STORE,
                    VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                    VK_ATTACHMENT_STORE_OP_DONT_CARE,
                    VK_IMAGE_LAYOUT_UNDEFINED,
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                }
        };

        std::vector<SubpassDescription> subpasses(1);

        subpasses[0].colorAttachments = {
                {1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL},
                {2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL},
                {3, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL}
        };
        subpasses[0].depthStencilAttachments = {0, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

        std::vector<VkSubpassDependency> dependencies{
                {
                    VK_SUBPASS_EXTERNAL,
                    0,
                    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                    VK_ACCESS_SHADER_READ_BIT,
                    VK_ACCESS_SHADER_WRITE_BIT,
                    VK_DEPENDENCY_BY_REGION_BIT
                },
                {
                    0,
                    VK_SUBPASS_EXTERNAL,
                    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                    VK_ACCESS_SHADER_WRITE_BIT,
                    VK_ACCESS_SHADER_READ_BIT,
                    VK_DEPENDENCY_BY_REGION_BIT
                }
        };

        gBuffer.renderpass = device.createRenderPass(attachments, subpasses, dependencies);
        device.setName<VK_OBJECT_TYPE_RENDER_PASS>("g_buffer", gBuffer.renderpass.renderPass);
    };

    auto createFrameBuffer = [&]{
        std::vector<VkImageView> attachments{
            gBuffer.depth.imageView,
            gBuffer.position.imageView,
            gBuffer.normal.imageView,
            gBuffer.color.imageView
        };

        gBuffer.framebuffer = device.createFramebuffer(gBuffer.renderpass, attachments, width, height);
        device.setName<VK_OBJECT_TYPE_FRAMEBUFFER>("g_buffer", gBuffer.framebuffer.frameBuffer);
    };

    createAttachments();
    createRenderPass();
    createFrameBuffer();
}

void SsaoDemo::createGlobalSampler() {
    auto info = initializers::samplerCreateInfo();
    info.magFilter = VK_FILTER_LINEAR;
    info.minFilter = VK_FILTER_LINEAR;
    info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    info.addressModeV = info.addressModeU;
    info.addressModeW = info.addressModeU;
    globalSampler = device.createSampler(info);
}

void SsaoDemo::createDescriptorSetLayout() {
    textureSetLayout =
        device.descriptorSetLayoutBuilder()
            .name("textureLayout")
            .binding(0)
                .descriptorType(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
                .descriptorCount(1)
                .shaderStages(VK_SHADER_STAGE_FRAGMENT_BIT)
                .immutableSamplers(globalSampler)
        .createLayout();

    ssao.setLayout =
        device.descriptorSetLayoutBuilder()
            .name("ssao_layout")
            .binding(0) // position
                .descriptorType(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
                .descriptorCount(1)
                .shaderStages(VK_SHADER_STAGE_FRAGMENT_BIT)
                .immutableSamplers(globalSampler)
            .binding(1) // normal
                .descriptorType(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
                .descriptorCount(1)
                .shaderStages(VK_SHADER_STAGE_FRAGMENT_BIT)
                .immutableSamplers(globalSampler)
            .binding(2) // color
                .descriptorType(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
                .descriptorCount(1)
                .shaderStages(VK_SHADER_STAGE_FRAGMENT_BIT)
                .immutableSamplers(globalSampler)
            .binding(3) // occlusion
                .descriptorType(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
                .descriptorCount(1)
                .shaderStages(VK_SHADER_STAGE_FRAGMENT_BIT)
                .immutableSamplers(globalSampler)
            .binding(4) // noise
                .descriptorType(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
                .descriptorCount(1)
                .shaderStages(VK_SHADER_STAGE_FRAGMENT_BIT)
            .binding(5)  // hemisphere samples
                .descriptorType(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
                .descriptorCount(1)
                .shaderStages(VK_SHADER_STAGE_FRAGMENT_BIT)
            .createLayout();

}

void SsaoDemo::updateDescriptorSet() {
//    auto sets = descriptorPool.allocate({textureSetLayout});
//    quad.descriptorSet = sets[0];
//    device.setName<VK_OBJECT_TYPE_DESCRIPTOR_SET>("screen_quad", quad.descriptorSet);
//
//    auto writes = initializers::writeDescriptorSets<1>();
//
//    writes[0].dstSet = quad.descriptorSet;
//    writes[0].dstBinding = 0;
//    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
//    writes[0].descriptorCount = 1;
//
//    VkDescriptorImageInfo imageInfo{VK_NULL_HANDLE, ssao.blur.blurOutput.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
//    writes[0].pImageInfo = &imageInfo;
//
//    device.updateDescriptorSets(writes);
    updateSsaoDescriptorSet();
}

void SsaoDemo::updateSsaoDescriptorSet() {
    auto sets = descriptorPool.allocate({ ssao.setLayout, textureSetLayout });
    ssao.descriptorSet = sets[0];
    ssao.blur.descriptorSet = sets[1];

    device.setName<VK_OBJECT_TYPE_DESCRIPTOR_SET>("ssao", ssao.descriptorSet);
    device.setName<VK_OBJECT_TYPE_DESCRIPTOR_SET>("ssao_blur", ssao.blur.descriptorSet);

    auto writes = initializers::writeDescriptorSets<7>();

    writes[0].dstSet = ssao.descriptorSet;
    writes[0].dstBinding = 0;   // position
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[0].descriptorCount = 1;

    VkDescriptorImageInfo posImageInfo{VK_NULL_HANDLE, gBuffer.position.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    writes[0].pImageInfo = &posImageInfo;

    writes[1].dstSet = ssao.descriptorSet;
    writes[1].dstBinding = 1;   // normal
    writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[1].descriptorCount = 1;

    VkDescriptorImageInfo normalImageInfo{VK_NULL_HANDLE, gBuffer.normal.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    writes[1].pImageInfo = &normalImageInfo;

    writes[2].dstSet = ssao.descriptorSet;
    writes[2].dstBinding = 2;   // color
    writes[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[2].descriptorCount = 1;

    VkDescriptorImageInfo colorImageInfo{VK_NULL_HANDLE, gBuffer.color.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    writes[2].pImageInfo = &colorImageInfo;

    writes[3].dstSet = ssao.descriptorSet;
    writes[3].dstBinding = 3;   // occlusion
    writes[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[3].descriptorCount = 1;

    VkDescriptorImageInfo aoImageInfo{VK_NULL_HANDLE, ssao.blur.blurOutput.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    writes[3].pImageInfo = &aoImageInfo;

    writes[4].dstSet = ssao.descriptorSet;
    writes[4].dstBinding = 4;   // noise
    writes[4].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[4].descriptorCount = 1;

    VkDescriptorImageInfo noiseImageInfo{ssao.noise.sampler, ssao.noise.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    writes[4].pImageInfo = &noiseImageInfo;

    writes[5].dstSet = ssao.blur.descriptorSet;
    writes[5].dstBinding = 0;
    writes[5].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[5].descriptorCount = 1;

    VkDescriptorImageInfo imageInfo{VK_NULL_HANDLE, ssao.occlusion.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    writes[5].pImageInfo = &imageInfo;

    writes[6].dstSet = ssao.descriptorSet;
    writes[6].dstBinding = 5;   // samples
    writes[6].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[6].descriptorCount = 1;

    VkDescriptorBufferInfo samplesInfo{ssao.samples, 0, VK_WHOLE_SIZE};
    writes[6].pBufferInfo = &samplesInfo;

    device.updateDescriptorSets(writes);

}

void SsaoDemo::initScreenQuad() {
    auto shape = ClipSpace::Quad::positions;
    quad.vertices = device.createDeviceLocalBuffer(shape.data(), BYTE_SIZE(shape), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    quad.numVertices = static_cast<uint32_t>(shape.size());
}

void SsaoDemo::renderUI(VkCommandBuffer commandBuffer) {

    ImGui::Begin("Settings");
    ImGui::SetWindowSize({250, 250});

    static bool aoOn = lighting.constants.aoOn;

    ImGui::Checkbox("Ambient Occlusion on", &aoOn);
    lighting.constants.aoOn = aoOn;

    if(aoOn) {
        ImGui::SliderInt("Kernel size", &ssao.constants.kernelSize, 0.f, 64.f);
        ImGui::SliderFloat("radius", &ssao.constants.radius, 0.f, 5.f);
        ImGui::SliderFloat("bias", &ssao.constants.bias, 0.f, 1.f);

        static bool aoOnly = lighting.constants.aoOnly;
        ImGui::Checkbox("Ambient Occlusion Only", &aoOnly);
        lighting.constants.aoOnly = aoOnly;
    }

    static bool blurOn = ssao.blur.on;
    ImGui::Checkbox("blur", &blurOn);
    ssao.blur.on = blurOn;

    ImGui::Text("%d fps", framePerSecond);
    ImGui::End();


    plugin(IM_GUI_PLUGIN).draw(commandBuffer);
}

void SsaoDemo::createSsaoSamplingData() {
    auto x = rng(-1, 1);
    auto y = rng(-1, 1);
    auto z = rng(0, 1);
    auto r = rng(0, 1);

    auto lerp = [](float a, float b, float f){
        return a + f * (b - a);
    };

    std::vector<glm::vec4> kernel;
    kernel.reserve(64);
    for(int i = 0; i < 64; i++){
        glm::vec4 sample{x(), y(), z(), 0};
        sample = glm::normalize(sample) * r();

        auto scale = static_cast<float>(i)/64.0f;
        scale = lerp(0.1f, 1.0f, scale * scale);
        sample *= scale;
        kernel.push_back(sample);
    }
    ssao.samples = device.createDeviceLocalBuffer(kernel.data(), BYTE_SIZE(kernel), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);

    std::vector<glm::vec4> noises;
    noises.reserve(16);
    for(int i = 0; i < 16; i++){
        noises.emplace_back(x(), y(), 0, 0);
    }

    textures::create(device, ssao.noise, VK_IMAGE_TYPE_2D, VK_FORMAT_R32G32B32A32_SFLOAT, noises.data()
                     , {4, 4, 1}, VK_SAMPLER_ADDRESS_MODE_REPEAT, sizeof(float));
}

void SsaoDemo::createSsaoFrameBuffer() {
    auto colorFormat = VK_FORMAT_R32G32B32A32_SFLOAT;

    auto createAttachments = [&]{
        auto imageCreateInfo = initializers::imageCreateInfo(VK_IMAGE_TYPE_2D, colorFormat
                                                             , VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
                                                             , swapChain.width(), swapChain.height());

        auto subResource = initializers::imageSubresourceRange();
        ssao.occlusion.image = device.createImage(imageCreateInfo);
        device.setName<VK_OBJECT_TYPE_IMAGE>("ssao_occlusion", ssao.occlusion.image.image);

        ssao.occlusion.imageView = ssao.occlusion.image.createView(colorFormat, VK_IMAGE_VIEW_TYPE_2D, subResource);
        device.setName<VK_OBJECT_TYPE_IMAGE_VIEW>("ssao_occlusion", ssao.occlusion.imageView.handle);
    };

    auto createRenderPass = [&]{
        std::vector<VkAttachmentDescription> attachments{
                {
                    0,
                    colorFormat,
                    VK_SAMPLE_COUNT_1_BIT,
                    VK_ATTACHMENT_LOAD_OP_CLEAR,
                    VK_ATTACHMENT_STORE_OP_STORE,
                    VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                    VK_ATTACHMENT_STORE_OP_DONT_CARE,
                    VK_IMAGE_LAYOUT_UNDEFINED,
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                }
        };

        std::vector<SubpassDescription> subpasses(1);
        subpasses[0].colorAttachments.push_back({0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});

        std::vector<VkSubpassDependency> dependencies(2);
        dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[0].dstSubpass = 0;
        dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dependencies[0].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dependencies[0].srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        dependencies[0].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        dependencies[1].srcSubpass = 0;
        dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[1].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dependencies[1].srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        ssao.renderpass = device.createRenderPass(attachments, subpasses, dependencies);

    };

    auto createFramebuffer = [&]{
        std::vector<VkImageView> attachments{ ssao.occlusion.imageView };
        ssao.framebuffer = device.createFramebuffer(ssao.renderpass, attachments, swapChain.width(), swapChain.height());
    };

    createAttachments();
    createRenderPass();
    createFramebuffer();
}

void SsaoDemo::createBlurFrameBuffer() {
    auto colorFormat = VK_FORMAT_R32G32B32A32_SFLOAT;

    auto createAttachments = [&]{
        auto imageCreateInfo = initializers::imageCreateInfo(VK_IMAGE_TYPE_2D, colorFormat
                , VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
                , swapChain.width(), swapChain.height());

        auto subResource = initializers::imageSubresourceRange();
        ssao.blur.blurOutput.image = device.createImage(imageCreateInfo);
        device.setName<VK_OBJECT_TYPE_IMAGE>("ssao_blur", ssao.blur.blurOutput.image.image);

        ssao.blur.blurOutput.imageView = ssao.blur.blurOutput.image.createView(colorFormat, VK_IMAGE_VIEW_TYPE_2D, subResource);
        device.setName<VK_OBJECT_TYPE_IMAGE_VIEW>("ssao_blur", ssao.blur.blurOutput.imageView.handle);
    };

    auto createRenderPass = [&]{
        std::vector<VkAttachmentDescription> attachments{
                {
                        0,
                        colorFormat,
                        VK_SAMPLE_COUNT_1_BIT,
                        VK_ATTACHMENT_LOAD_OP_CLEAR,
                        VK_ATTACHMENT_STORE_OP_STORE,
                        VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                        VK_ATTACHMENT_STORE_OP_DONT_CARE,
                        VK_IMAGE_LAYOUT_UNDEFINED,
                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                }
        };

        std::vector<SubpassDescription> subpasses(1);
        subpasses[0].colorAttachments.push_back({0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});

        std::vector<VkSubpassDependency> dependencies(2);
        dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[0].dstSubpass = 0;
        dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dependencies[0].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dependencies[0].srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        dependencies[0].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        dependencies[1].srcSubpass = 0;
        dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[1].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dependencies[1].srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        ssao.blur.renderpass = device.createRenderPass(attachments, subpasses, dependencies);

    };

    auto createFramebuffer = [&]{
        std::vector<VkImageView> attachments{ ssao.blur.blurOutput.imageView };
        ssao.blur.framebuffer = device.createFramebuffer(ssao.blur.renderpass, attachments, swapChain.width(), swapChain.height());
    };

    createAttachments();
    createRenderPass();
    createFramebuffer();
}

void SsaoDemo::ssaoPass(VkCommandBuffer commandBuffer) {
    static std::array<VkClearValue, 1> clearValues;
    clearValues[0].color = {0, 0, 1, 1};

    VkRenderPassBeginInfo rPassInfo = initializers::renderPassBeginInfo();
    rPassInfo.clearValueCount = COUNT(clearValues);
    rPassInfo.pClearValues = clearValues.data();
    rPassInfo.framebuffer = ssao.framebuffer;
    rPassInfo.renderArea.offset = {0u, 0u};
    rPassInfo.renderArea.extent = swapChain.extent;
    rPassInfo.renderPass = ssao.renderpass;

    vkCmdBeginRenderPass(commandBuffer, &rPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, ssao.pipeline);
    ssao.constants.projection = cameraController->cam().proj;

    vkCmdPushConstants(commandBuffer, ssao.layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(ssao.constants), &ssao.constants);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, ssao.layout, 0, 1, &ssao.descriptorSet, 0, VK_NULL_HANDLE);

    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, quad.vertices, &offset);
    vkCmdDraw(commandBuffer, quad.numVertices, 1, 0, 0);

    vkCmdEndRenderPass(commandBuffer);
}

void SsaoDemo::blurPass(VkCommandBuffer commandBuffer) {
    static std::array<VkClearValue, 1> clearValues;
    clearValues[0].color = {0, 0, 1, 1};

    VkRenderPassBeginInfo rPassInfo = initializers::renderPassBeginInfo();
    rPassInfo.clearValueCount = COUNT(clearValues);
    rPassInfo.pClearValues = clearValues.data();
    rPassInfo.framebuffer = ssao.blur.framebuffer;
    rPassInfo.renderArea.offset = {0u, 0u};
    rPassInfo.renderArea.extent = swapChain.extent;
    rPassInfo.renderPass = ssao.blur.renderpass;

    vkCmdBeginRenderPass(commandBuffer, &rPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, ssao.blur.pipeline);

    vkCmdPushConstants(commandBuffer, ssao.blur.layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(int), &ssao.blur.on);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, ssao.blur.layout, 0, 1, &ssao.blur.descriptorSet, 0, VK_NULL_HANDLE);

    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, quad.vertices, &offset);
    vkCmdDraw(commandBuffer, quad.numVertices, 1, 0, 0);

    vkCmdEndRenderPass(commandBuffer);
}

int main(){
    try{

        Settings settings;
        settings.depthTest = true;
        settings.enabledFeatures.fillModeNonSolid = true;

        auto app = SsaoDemo{ settings };
        std::unique_ptr<Plugin> imGui = std::make_unique<ImGuiPlugin>();
        app.addPlugin(imGui);
        app.run();
    }catch(std::runtime_error& err){
        spdlog::error(err.what());
    }
}