#include "DeferredRendering.hpp"
#include "GraphicsPipelineBuilder.hpp"
#include "DescriptorSetBuilder.hpp"

const std::string DeferredRendering::kAttachment_GBUFFER_ALBDEO = "GBUFFER_ALBEDO_INDEX";
const std::string DeferredRendering::kAttachment_GBUFFER_NORMAL = "GBUFFER_NORMAL_INDEX";
const std::string DeferredRendering::kAttachment_GBUFFER_POSITION = "GBUFFER_DEPTH_INDEX";

DeferredRendering::DeferredRendering(const Settings& settings) : VulkanBaseApp("Deferred Rendering", settings) {
    fileManager.addSearchPath(".");
    fileManager.addSearchPath("../../examples/deferred_rendering");
    fileManager.addSearchPath("../../data/shaders");
    fileManager.addSearchPath("../../data");
    fileManager.addSearchPath(R"(..\..\data\models)");
    fileManager.addSearchPath("../../data/textures");
    selectOption = &mapToKey(Key::SPACE_BAR, "display option", Action::Behavior::DETECT_INITIAL_PRESS_ONLY);
}

void DeferredRendering::initApp() {
    setupInput();
    createDescriptorPool();
    createDescriptorSetLayouts();
    updateDescriptorSets();
    createCommandPool();

    createScreenBuffer();
    loadSponza();
    initCamera();

    createPipelineCache();
    createPipelines();
}

void DeferredRendering::createDescriptorPool() {
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

void DeferredRendering::createCommandPool() {
    commandPool = device.createCommandPool(*device.queueFamilyIndex.graphics, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    commandBuffers = commandPool.allocate(swapChainImageCount);
}

void DeferredRendering::createPipelineCache() {
    pipelineCache = device.createPipelineCache();
}

void DeferredRendering::createPipelines() {
    auto builder = device.graphicsPipelineBuilder();
    createDepthPrePassPipeline(builder);
    createGBufferPipeline(builder);
    createRenderPipeline(builder);
}

void DeferredRendering::createDepthPrePassPipeline(GraphicsPipelineBuilder &builder) {
    pipelines.depthPrePass.pipeline =
        builder
            .allowDerivatives()
            .shaderStage()
                .vertexShader(load("spv/vertex.vert.spv"))
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
                    .enableRasterizerDiscard()
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
                    .attachment()
                    .add()
                    .add()
                    .add()
                .layout()
                    .addPushConstantRange(Camera::pushConstant())
                .renderPass(renderPass)
                .subpass(kSubpass_DEPTH)
                .name("depth")
                .pipelineCache(pipelineCache)
            .build(pipelines.depthPrePass.layout);
}

void DeferredRendering::createGBufferPipeline(GraphicsPipelineBuilder &builder) {
	pipelines.gBuffer.pipeline =
        builder
            .setDerivatives()
            .shaderStage()
              .fragmentShader(load("spv/g_buffer.frag.spv"))
            .rasterizationState()
                .disableRasterizerDiscard()
            .depthStencilState()
                .enableDepthTest()
                .enableDepthWrite()
            .layout()
                .addPushConstantRange(VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(Camera), sizeof(CameraProps))
                .addDescriptorSetLayout(sponza.descriptorSetLayout)
            .subpass(kSubpass_GBUFFER)
            .name("g_buffer")
            .pipelineCache(pipelineCache)
            .basePipeline(pipelines.depthPrePass.pipeline)
            .build(pipelines.gBuffer.layout);
}

void DeferredRendering::createRenderPipeline(GraphicsPipelineBuilder& builder) {
    pipelines.render.pipeline =
        builder
            .setDerivatives()
            .shaderStage()
                .clear()
                .vertexShader(load("spv/render.vert.spv"))
                .fragmentShader(load("spv/render.frag.spv"))
            .vertexInputState()
                .clear()
                .addVertexBindingDescriptions(ClipSpace::bindingDescription())
                .addVertexAttributeDescriptions(ClipSpace::attributeDescriptions())
            .inputAssemblyState()
                .triangleStrip()
            .rasterizationState()
                .disableRasterizerDiscard()
            .depthStencilState()
                .disableDepthWrite()
                .disableDepthTest()
            .colorBlendState()
                .attachment()
                .clear()
                .add()
            .layout()
                .clear()
                .addPushConstantRange(VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(renderConstants))
                .addDescriptorSetLayout(gBuffer.setLayout)
            .subpass(kSubpass_RENDER)
            .name("render")
            .pipelineCache(pipelineCache)
            .basePipeline(pipelines.depthPrePass.pipeline)
            .build(pipelines.render.layout);
}


void DeferredRendering::onSwapChainDispose() {
    dispose(pipelines.render.pipeline);
    dispose(pipelines.depthPrePass.pipeline);
    dispose(pipelines.gBuffer.pipeline);
    dispose(pipelines.render.pipeline);
}

void DeferredRendering::onSwapChainRecreation() {
    createPipelines();
}

VkCommandBuffer *DeferredRendering::buildCommandBuffers(uint32_t imageIndex, uint32_t &numCommandBuffers) {
    numCommandBuffers = 1;
    auto& commandBuffer = commandBuffers[imageIndex];

    VkCommandBufferBeginInfo beginInfo = initializers::commandBufferBeginInfo();
    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    static std::array<VkClearValue, 5> clearValues;
    clearValues[0].color = {0, 0, 0, 1};
    clearValues[1].depthStencil = {1.0, 0u};
    clearValues[2].color = {0, 0, 0, 1};
    clearValues[3].color = {0, 0, 0, 1};
    clearValues[4].color = {0, 0, 0, 1};

    VkRenderPassBeginInfo rPassInfo = initializers::renderPassBeginInfo();
    rPassInfo.clearValueCount = COUNT(clearValues);
    rPassInfo.pClearValues = clearValues.data();
    rPassInfo.framebuffer = framebuffers[imageIndex];
    rPassInfo.renderArea.offset = {0u, 0u};
    rPassInfo.renderArea.extent = swapChain.extent;
    rPassInfo.renderPass = renderPass;

    vkCmdBeginRenderPass(commandBuffer, &rPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.depthPrePass.pipeline);
    cameraController->push(commandBuffer, pipelines.depthPrePass.layout);
    sponza.draw(commandBuffer);

    vkCmdNextSubpass(commandBuffer, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.gBuffer.pipeline);
    cameraController->push(commandBuffer, pipelines.gBuffer.layout);
    vkCmdPushConstants(commandBuffer, pipelines.gBuffer.layout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(Camera), sizeof(CameraProps), &cameraProps);
    sponza.draw(commandBuffer, pipelines.gBuffer.layout);

    VkDeviceSize offset = 0;
    vkCmdNextSubpass(commandBuffer, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.render.pipeline);
    vkCmdPushConstants(commandBuffer, pipelines.render.layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(renderConstants), &renderConstants);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.render.layout, 0, 1, &gBuffer.descriptorSets[imageIndex], 0, nullptr);
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, screenBuffer, &offset);
    vkCmdDraw(commandBuffer, 4, 1, 0, 0);


//    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.depthPrePass.pipeline);
//    cameraController->push(commandBuffer, pipelines.depthPrePass.layout);
//    vkCmdPushConstants(commandBuffer, pipelines.depthPrePass.layout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(Camera), sizeof(CameraProps), &cameraProps);
//    sponza.draw(commandBuffer);
//
//    vkCmdNextSubpass(commandBuffer, VK_SUBPASS_CONTENTS_INLINE);
//    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.render.pipeline);
//    cameraController->push(commandBuffer, pipelines.render.layout);
//    vkCmdPushConstants(commandBuffer, pipelines.render.layout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(Camera), sizeof(CameraProps), &cameraProps);
//    sponza.draw(commandBuffer, pipelines.render.layout);

    vkCmdEndRenderPass(commandBuffer);

    vkEndCommandBuffer(commandBuffer);

    return &commandBuffer;
}

void DeferredRendering::update(float time) {
    cameraController->update(time);
}

void DeferredRendering::checkAppInputs() {
    if(selectOption->isPressed()){
        renderConstants.displayOption++;
        renderConstants.displayOption %= MAX_DISPLAY_OPTIONS;
    }
    cameraController->processInput();
}

void DeferredRendering::cleanup() {
    // TODO save pipeline cache
    VulkanBaseApp::cleanup();
}

void DeferredRendering::onPause() {
    VulkanBaseApp::onPause();
}

void DeferredRendering::loadSponza() {
    phong::load(fileManager.getFullPath("Sponza\\sponza.obj")->string(), device, descriptorPool, sponza);
}

void DeferredRendering::initCamera() {
    CameraSettings settings{};
    settings.aspectRatio = static_cast<float>(swapChain.extent.width)/static_cast<float>(swapChain.extent.height);
    settings.rotationSpeed = 0.1f;
    settings.horizontalFov = true;
    settings.zNear = 1.0;
    settings.zFar = 100;
    cameraController = std::make_unique<CameraController>(device, swapChain.imageCount(), currentImageIndex, dynamic_cast<InputManager&>(*this), settings);
    cameraController->setMode(CameraMode::SPECTATOR);
    auto pos = (sponza.bounds.max + sponza.bounds.min) * 0.5f;
    cameraController->lookAt(pos, {0, 1, 0}, {0, 0, 1});
    cameraProps.near = cameraController->near();
    cameraProps.far = cameraController->far();
    renderConstants.lightPos = glm::vec4(pos, 1);
}

void DeferredRendering::createDescriptorSetLayouts() {
    gBuffer.setLayout =
        device.descriptorSetLayoutBuilder()
            .binding(kLayoutBinding_ALBDO)
                .descriptorType(VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT)
                .descriptorCount(1)
                .shaderStages(VK_SHADER_STAGE_FRAGMENT_BIT)
            .binding(kLayoutBinding_NORMAL)
                .descriptorType(VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT)
                .descriptorCount(1)
                .shaderStages(VK_SHADER_STAGE_FRAGMENT_BIT)
            .binding(kLayoutBinding_POSITION)
                .descriptorType(VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT)
                .descriptorCount(1)
                .shaderStages(VK_SHADER_STAGE_FRAGMENT_BIT)
            .createLayout();
}

void DeferredRendering::createGBuffer() {
    auto imageCreatInfo = initializers::imageCreateInfo(
            VK_IMAGE_TYPE_2D,
            VK_FORMAT_R32G32B32A32_SFLOAT,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
            swapChain.extent.width,
            swapChain.extent.height
            );

    gBuffer.albedo.resize(swapChainImageCount);
    gBuffer.albedo[0].image = device.createImage(imageCreatInfo);
    gBuffer.albedo[1].image = device.createImage(imageCreatInfo);
    gBuffer.albedo[2].image = device.createImage(imageCreatInfo);

    VkImageSubresourceRange subresourceRange = initializers::imageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);
    gBuffer.albedo[0].imageView =  gBuffer.albedo[0].image.createView(VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_VIEW_TYPE_2D, subresourceRange);
    gBuffer.albedo[1].imageView =  gBuffer.albedo[1].image.createView(VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_VIEW_TYPE_2D, subresourceRange);
    gBuffer.albedo[2].imageView =  gBuffer.albedo[2].image.createView(VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_VIEW_TYPE_2D, subresourceRange);


    imageCreatInfo = initializers::imageCreateInfo(
            VK_IMAGE_TYPE_2D,
            VK_FORMAT_R32G32B32A32_SFLOAT,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
            swapChain.extent.width,
            swapChain.extent.height
    );
    gBuffer.normal.resize(swapChainImageCount);
    gBuffer.normal[0].image = device.createImage(imageCreatInfo);
    gBuffer.normal[1].image = device.createImage(imageCreatInfo);
    gBuffer.normal[2].image = device.createImage(imageCreatInfo);
    gBuffer.normal[0].imageView = gBuffer.normal[0].image.createView(VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_VIEW_TYPE_2D, subresourceRange);
    gBuffer.normal[1].imageView = gBuffer.normal[1].image.createView(VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_VIEW_TYPE_2D, subresourceRange);
    gBuffer.normal[2].imageView = gBuffer.normal[2].image.createView(VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_VIEW_TYPE_2D, subresourceRange);

    imageCreatInfo = initializers::imageCreateInfo(
            VK_IMAGE_TYPE_2D,
            VK_FORMAT_R32G32B32A32_SFLOAT,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
            swapChain.extent.width,
            swapChain.extent.height
    );

    gBuffer.position.resize(swapChainImageCount);
    gBuffer.position[0].image = device.createImage(imageCreatInfo);
    gBuffer.position[1].image = device.createImage(imageCreatInfo);
    gBuffer.position[2].image = device.createImage(imageCreatInfo);
    subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    gBuffer.position[0].imageView  = gBuffer.position[0].image.createView(VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_VIEW_TYPE_2D, subresourceRange);
    gBuffer.position[1].imageView  = gBuffer.position[1].image.createView(VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_VIEW_TYPE_2D, subresourceRange);
    gBuffer.position[2].imageView  = gBuffer.position[2].image.createView(VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_VIEW_TYPE_2D, subresourceRange);
}

void DeferredRendering::updateDescriptorSets() {
    auto sets = descriptorPool.allocate({ gBuffer.setLayout, gBuffer.setLayout, gBuffer.setLayout});
    gBuffer.descriptorSets.clear();
    gBuffer.descriptorSets.insert(begin(gBuffer.descriptorSets), begin(sets), end(sets));

    for(int i = 0; i < swapChainImageCount; i++){
        device.setName<VK_OBJECT_TYPE_DESCRIPTOR_SET>(fmt::format("{}_{}", "g_buffer", i), gBuffer.descriptorSets[i]);

        VkDescriptorImageInfo albedoInfo{};
        albedoInfo.imageView = gBuffer.albedo[i].imageView;
        albedoInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        auto writes = initializers::writeDescriptorSets<3>();
        writes[kLayoutBinding_ALBDO].dstSet = gBuffer.descriptorSets[i];
        writes[kLayoutBinding_ALBDO].dstBinding = kLayoutBinding_ALBDO;
        writes[kLayoutBinding_ALBDO].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        writes[kLayoutBinding_ALBDO].descriptorCount = 1;
        writes[kLayoutBinding_ALBDO].pImageInfo = &albedoInfo;

        VkDescriptorImageInfo normalInfo{};
        normalInfo.imageView = gBuffer.normal[i].imageView;
        normalInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        writes[kLayoutBinding_NORMAL].dstSet = gBuffer.descriptorSets[i];
        writes[kLayoutBinding_NORMAL].dstBinding = kLayoutBinding_NORMAL;
        writes[kLayoutBinding_NORMAL].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        writes[kLayoutBinding_NORMAL].descriptorCount = 1;
        writes[kLayoutBinding_NORMAL].pImageInfo = &normalInfo;

        VkDescriptorImageInfo positionInfo{};
        positionInfo.imageView = gBuffer.position[i].imageView;
        positionInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        writes[kLayoutBinding_POSITION].dstSet = gBuffer.descriptorSets[i];
        writes[kLayoutBinding_POSITION].dstBinding = kLayoutBinding_POSITION;
        writes[kLayoutBinding_POSITION].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        writes[kLayoutBinding_POSITION].descriptorCount = 1;
        writes[kLayoutBinding_POSITION].pImageInfo = &positionInfo;

        device.updateDescriptorSets(writes);
    }

}

void DeferredRendering::createScreenBuffer() {
    auto screen = ClipSpace::Quad::positions;
    screenBuffer = device.createDeviceLocalBuffer(screen.data(), BYTE_SIZE(screen), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
}

RenderPassInfo DeferredRendering::buildRenderPass() {
    auto [attachments, subpassDesc, dependencies] = VulkanBaseApp::buildRenderPass();

   std::vector<VkAttachmentDescription> gBufferAttachments{
           {    // color attachment
               0,
               VK_FORMAT_R32G32B32A32_SFLOAT,
               VK_SAMPLE_COUNT_1_BIT,
               VK_ATTACHMENT_LOAD_OP_CLEAR,
               VK_ATTACHMENT_STORE_OP_DONT_CARE,
               VK_ATTACHMENT_LOAD_OP_DONT_CARE,
               VK_ATTACHMENT_STORE_OP_DONT_CARE,
               VK_IMAGE_LAYOUT_UNDEFINED,
               VK_IMAGE_LAYOUT_GENERAL
           },
           {    // normal attachment
               0,
               VK_FORMAT_R32G32B32A32_SFLOAT,
               VK_SAMPLE_COUNT_1_BIT,
               VK_ATTACHMENT_LOAD_OP_CLEAR,
               VK_ATTACHMENT_STORE_OP_DONT_CARE,
               VK_ATTACHMENT_LOAD_OP_DONT_CARE,
               VK_ATTACHMENT_STORE_OP_DONT_CARE,
               VK_IMAGE_LAYOUT_UNDEFINED,
               VK_IMAGE_LAYOUT_GENERAL
           },
           {    // position attachment
               0,
               VK_FORMAT_R32G32B32A32_SFLOAT,
               VK_SAMPLE_COUNT_1_BIT,
               VK_ATTACHMENT_LOAD_OP_CLEAR,
               VK_ATTACHMENT_STORE_OP_DONT_CARE,
               VK_ATTACHMENT_LOAD_OP_DONT_CARE,
               VK_ATTACHMENT_STORE_OP_DONT_CARE,
               VK_IMAGE_LAYOUT_UNDEFINED,
               VK_IMAGE_LAYOUT_GENERAL
           }
   };
   attachments.insert(end(attachments), begin(gBufferAttachments), end(gBufferAttachments));
   attachmentIndices[kAttachment_GBUFFER_ALBDEO] = attachments.size() - 3;
   attachmentIndices[kAttachment_GBUFFER_NORMAL] = attachments.size() - 2;
   attachmentIndices[kAttachment_GBUFFER_POSITION] = attachments.size() - 1;

    VkAttachmentReference depthRef{
        attachmentIndices[kAttachment_DEPTH],
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };

    VkAttachmentReference backRef{
        attachmentIndices[kAttachment_BACK],
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    VkAttachmentReference gBufferColorRef{
        attachmentIndices[kAttachment_GBUFFER_ALBDEO],
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    };

    VkAttachmentReference gBufferNormalRef{
        attachmentIndices[kAttachment_GBUFFER_NORMAL],
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    };

    VkAttachmentReference gBufferPositionRef{
        attachmentIndices[kAttachment_GBUFFER_POSITION],
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    };

    subpassDesc.clear();
    SubpassDescription prePassSubpass{};
    prePassSubpass.depthStencilAttachments = depthRef;
    subpassDesc.push_back(prePassSubpass);

    SubpassDescription gBufferSubpass{};
    gBufferSubpass.colorAttachments = {
            {   // albdeo
                attachmentIndices[kAttachment_GBUFFER_ALBDEO],
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
            },
            {   // normal
                attachmentIndices[kAttachment_GBUFFER_NORMAL],
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
            },
            {   // position
                attachmentIndices[kAttachment_GBUFFER_POSITION],
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
            },
    };
    gBufferSubpass.depthStencilAttachments = depthRef;
    subpassDesc.push_back(gBufferSubpass);

    SubpassDescription renderSubpass{};
    renderSubpass.inputAttachments = {
        {   // albdeo
                attachmentIndices[kAttachment_GBUFFER_ALBDEO],
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        },
        {   // normal
                attachmentIndices[kAttachment_GBUFFER_NORMAL],
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        },
        {   // position
                attachmentIndices[kAttachment_GBUFFER_POSITION],
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        },
};
    renderSubpass.colorAttachments.push_back(backRef);
    renderSubpass.depthStencilAttachments = depthRef;
    subpassDesc.push_back(renderSubpass);

    dependencies.clear();
    VkSubpassDependency gBufferOnPrePassDependency{};
    gBufferOnPrePassDependency.srcSubpass = kSubpass_DEPTH;
    gBufferOnPrePassDependency.dstSubpass = kSubpass_GBUFFER;
    gBufferOnPrePassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    gBufferOnPrePassDependency.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    gBufferOnPrePassDependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    gBufferOnPrePassDependency.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    gBufferOnPrePassDependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
    dependencies.push_back(gBufferOnPrePassDependency);

    VkSubpassDependency renderOnGBufferDependency{};
    renderOnGBufferDependency.srcSubpass = kSubpass_GBUFFER;
    renderOnGBufferDependency.dstSubpass = kSubpass_RENDER;
    renderOnGBufferDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    renderOnGBufferDependency.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    renderOnGBufferDependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    renderOnGBufferDependency.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    renderOnGBufferDependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
    dependencies.push_back(renderOnGBufferDependency);

    return std::make_tuple(attachments, subpassDesc, dependencies);
}

void DeferredRendering::createFramebuffer() {
    assert(renderPass.renderPass != VK_NULL_HANDLE);

    framebuffers.clear();
    framebuffers.resize(swapChainImageCount);
    for(int i = 0; i< swapChainImageCount; i++){
        std::vector<VkImageView> attachments(attachmentIndices.size());
        attachments[attachmentIndices[kAttachment_BACK]] = swapChain.imageViews[i];
        attachments[attachmentIndices[kAttachment_DEPTH]] = depthBuffer.imageView;
        attachments[attachmentIndices[kAttachment_GBUFFER_ALBDEO]] = gBuffer.albedo[i].imageView;
        attachments[attachmentIndices[kAttachment_GBUFFER_NORMAL]] = gBuffer.normal[i].imageView;
        attachments[attachmentIndices[kAttachment_GBUFFER_POSITION]] = gBuffer.position[i].imageView;
        framebuffers[i] = device.createFramebuffer(renderPass, attachments,  static_cast<uint32_t>(width), static_cast<uint32_t>(height) );
    }
}

void DeferredRendering::setupInput() {
    selectOption = &mapToKey(Key::SPACE_BAR, "display option", Action::Behavior::DETECT_INITIAL_PRESS_ONLY);
}

void DeferredRendering::swapChainReady() {
    createGBuffer();
}


int main(){
    try{

        Settings settings;
        settings.depthTest = true;

        auto app = DeferredRendering{ settings };
        app.run();
    }catch(std::runtime_error& err){
        spdlog::error(err.what());
    }
}