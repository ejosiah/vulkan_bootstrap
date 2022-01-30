#include "SsaoDemo.hpp"
#include "GraphicsPipelineBuilder.hpp"
#include "DescriptorSetBuilder.hpp"

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
    createGlobalSampler();
    createDescriptorSetLayout();
    initGBuffer();
    initScreenQuad();
    updateDescriptorSet();
    loadModel();
    initCamera();
    createPipelineCache();
    createRenderPipeline();
    createComputePipeline();
}

void SsaoDemo::loadModel() {
    phong::load(resource("Sponza/sponza.obj"), device, descriptorPool, model);
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
                    .attachments(2)
                .layout()
                    .addPushConstantRange(Camera::pushConstant())
                .renderPass(gBuffer.renderpass)
                .subpass(0)
                .name("render")
                .pipelineCache(pipelineCache)
            .build(render.layout);

    quad.pipeline =
        builder
            .basePipeline(render.pipeline)
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
            .layout()
                .addDescriptorSetLayout(textureSetLayout)
            .renderPass(renderPass)
            .name("screen_quad")
            .subpass(0)
        .build(quad.layout);

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

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, quad.pipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, quad.layout, 0, 1, &quad.descriptorSet, 0, 0);

    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, quad.vertices, &offset);
    vkCmdDraw(commandBuffer, quad.numVertices, 1, 0, 0);

    vkCmdEndRenderPass(commandBuffer);

    vkEndCommandBuffer(commandBuffer);

    return &commandBuffer;
}

void SsaoDemo::renderScene(VkCommandBuffer commandBuffer) {
    static std::array<VkClearValue, 3> clearValues;
    clearValues[0].depthStencil = {1.0, 0u};
    clearValues[1].color = {0, 0, 0, 0};
    clearValues[2].color = {0, 0, 0, 0};

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
    model.draw(commandBuffer);

    vkCmdEndRenderPass(commandBuffer);
}

void SsaoDemo::update(float time) {
    cameraController->update(time);
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
                    VK_IMAGE_LAYOUT_GENERAL
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
                    VK_IMAGE_LAYOUT_GENERAL
                }
        };

        std::vector<SubpassDescription> subpasses(1);

        subpasses[0].colorAttachments = {
                {1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL},
                {2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL}
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
            gBuffer.normal.imageView
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
            .binding(0)
                .descriptorType(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
                .descriptorCount(1)
                .shaderStages(VK_SHADER_STAGE_FRAGMENT_BIT)
                .immutableSamplers(globalSampler)
        .createLayout();
}

void SsaoDemo::updateDescriptorSet() {
    auto sets = descriptorPool.allocate({textureSetLayout});
    quad.descriptorSet = sets[0];
    device.setName<VK_OBJECT_TYPE_DESCRIPTOR_SET>("screen_quad", quad.descriptorSet);

    auto writes = initializers::writeDescriptorSets<1>();

    writes[0].dstSet = quad.descriptorSet;
    writes[0].dstBinding = 0;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[0].descriptorCount = 1;

    VkDescriptorImageInfo imageInfo{VK_NULL_HANDLE, gBuffer.position.imageView, VK_IMAGE_LAYOUT_GENERAL};
    writes[0].pImageInfo = &imageInfo;

    device.updateDescriptorSets(writes);
}

void SsaoDemo::initScreenQuad() {
    auto shape = ClipSpace::Quad::positions;
    quad.vertices = device.createDeviceLocalBuffer(shape.data(), BYTE_SIZE(shape), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    quad.numVertices = static_cast<uint32_t>(shape.size());
}

int main(){
    try{

        Settings settings;
        settings.depthTest = true;
        settings.enabledFeatures.fillModeNonSolid = true;

        auto app = SsaoDemo{ settings };
        app.run();
    }catch(std::runtime_error& err){
        spdlog::error(err.what());
    }
}