#include "DeferredRendering.hpp"
#include "GraphicsPipelineBuilder.hpp"
#include "DescriptorSetBuilder.hpp"

const std::string DeferredRendering::kAttachment_GBUFFER_ALBDEO = "GBUFFER_ALBEDO_INDEX";
const std::string DeferredRendering::kAttachment_GBUFFER_NORMAL = "GBUFFER_NORMAL_INDEX";
const std::string DeferredRendering::kAttachment_GBUFFER_DEPTH = "GBUFFER_DEPTH_INDEX";

DeferredRendering::DeferredRendering(const Settings& settings) : VulkanBaseApp("Deferred Rendering", settings) {
    fileManager.addSearchPath(".");
    fileManager.addSearchPath("../../examples/deferred_rendering");
    fileManager.addSearchPath("../../data/shaders");
    fileManager.addSearchPath("../../data");
    fileManager.addSearchPath(R"(..\..\data\models)");
    fileManager.addSearchPath("../../data/textures");
}

void DeferredRendering::initApp() {
    createDescriptorPool();
    createDescriptorSetLayouts();
    createGBuffer();
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
                .layout()
                    .addPushConstantRange(Camera::pushConstant())
                    .addPushConstantRange(VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(Camera), sizeof(CameraProps))
                .renderPass(renderPass)
                .subpass(kSubpass_DEPTH)
                .name("render")
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
                .disableDepthTest()
                .disableDepthWrite()
            .layout()
                .addDescriptorSetLayout(sponza.descriptorSetLayout)
            .subpass(kSubpass_GBUFFER)
            .basePipeline(pipelines.depthPrePass.pipeline)
            .name("g_buffer")
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
            .layout()
                .clear()
                .addDescriptorSetLayout(gBuffer.setLayout)
            .renderPass(renderPass)
            .subpass(kSubpass_RENDER)
            .basePipeline(pipelines.gBuffer.pipeline)
            .name("render")
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

//    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.render.pipeline);
//    cameraController->push(commandBuffer, pipelines.render.layout);
//    vkCmdPushConstants(commandBuffer, pipelines.render.layout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(Camera), sizeof(CameraProps), &cameraProps);
//    sponza.draw(commandBuffer, pipelines.render.layout);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.depthPrePass.pipeline);
    cameraController->push(commandBuffer, pipelines.depthPrePass.layout);
    vkCmdPushConstants(commandBuffer, pipelines.depthPrePass.layout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(Camera), sizeof(CameraProps), &cameraProps);
    sponza.draw(commandBuffer);

    vkCmdNextSubpass(commandBuffer, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.gBuffer.pipeline);
    cameraController->push(commandBuffer, pipelines.gBuffer.layout);
    vkCmdPushConstants(commandBuffer, pipelines.gBuffer.layout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(Camera), sizeof(CameraProps), &cameraProps);
    sponza.draw(commandBuffer, pipelines.gBuffer.layout);

    VkDeviceSize offset = 0;
    vkCmdNextSubpass(commandBuffer, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.render.pipeline);
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, screenBuffer, &offset);
    vkCmdDraw(commandBuffer, 4, 1, 0, 0);

    vkCmdEndRenderPass(commandBuffer);

    vkEndCommandBuffer(commandBuffer);

    return &commandBuffer;
}

void DeferredRendering::update(float time) {
    cameraController->update(time);
}

void DeferredRendering::checkAppInputs() {
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
            .binding(kLayoutBinding_DEPTH)
                .descriptorType(VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT)
                .descriptorCount(1)
                .shaderStages(VK_SHADER_STAGE_FRAGMENT_BIT)
            .createLayout();
}

void DeferredRendering::createGBuffer() {
    auto imageCreatInfo = initializers::imageCreateInfo(
            VK_IMAGE_TYPE_2D,
            VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            swapChain.extent.width,
            swapChain.extent.height
            );

    gBuffer.albedo.image = device.createImage(imageCreatInfo);

    VkImageSubresourceRange subresourceRange = initializers::imageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);
    gBuffer.albedo.imageView =  gBuffer.albedo.image.createView(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_VIEW_TYPE_2D, subresourceRange);


    imageCreatInfo = initializers::imageCreateInfo(
            VK_IMAGE_TYPE_2D,
            VK_FORMAT_R32G32B32_SFLOAT,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            swapChain.extent.width,
            swapChain.extent.height
    );
    gBuffer.normal.image = device.createImage(imageCreatInfo);
    gBuffer.normal.imageView = gBuffer.normal.image.createView(VK_FORMAT_R32G32B32_SFLOAT, VK_IMAGE_VIEW_TYPE_2D, subresourceRange);

    imageCreatInfo = initializers::imageCreateInfo(
            VK_IMAGE_TYPE_2D,
            VK_FORMAT_D32_SFLOAT,
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            swapChain.extent.width,
            swapChain.extent.height
    );
    gBuffer.depth.image = device.createImage(imageCreatInfo);
    subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    gBuffer.depth.imageView  = gBuffer.depth.image.createView(VK_FORMAT_D32_SFLOAT, VK_IMAGE_VIEW_TYPE_2D, subresourceRange);
}

void DeferredRendering::updateDescriptorSets() {
    gBuffer.descriptorSet = descriptorPool.allocate({ gBuffer.setLayout}).front();
    device.setName<VK_OBJECT_TYPE_DESCRIPTOR_SET>("g_buffer", gBuffer.descriptorSet);

    VkDescriptorImageInfo albedoInfo{};
    albedoInfo.imageView = gBuffer.albedo.imageView;
    albedoInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    auto writes = initializers::writeDescriptorSets<3>();
    writes[kLayoutBinding_ALBDO].dstSet = gBuffer.descriptorSet;
    writes[kLayoutBinding_ALBDO].dstBinding = kLayoutBinding_ALBDO;
    writes[kLayoutBinding_ALBDO].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
    writes[kLayoutBinding_ALBDO].descriptorCount = 1;
    writes[kLayoutBinding_ALBDO].pImageInfo = &albedoInfo;

    VkDescriptorImageInfo normalInfo{};
    normalInfo.imageView = gBuffer.normal.imageView;
    normalInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    writes[kLayoutBinding_NORMAL].dstSet = gBuffer.descriptorSet;
    writes[kLayoutBinding_NORMAL].dstBinding = kLayoutBinding_ALBDO;
    writes[kLayoutBinding_NORMAL].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
    writes[kLayoutBinding_NORMAL].descriptorCount = 1;
    writes[kLayoutBinding_NORMAL].pImageInfo = &normalInfo;

    VkDescriptorImageInfo depthInfo{};
    depthInfo.imageView = gBuffer.depth.imageView;
    depthInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
    writes[kLayoutBinding_DEPTH].dstSet = gBuffer.descriptorSet;
    writes[kLayoutBinding_DEPTH].dstBinding = kLayoutBinding_DEPTH;
    writes[kLayoutBinding_DEPTH].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
    writes[kLayoutBinding_DEPTH].descriptorCount = 1;
    writes[kLayoutBinding_DEPTH].pImageInfo = &depthInfo;

    device.updateDescriptorSets(writes);
}

void DeferredRendering::createScreenBuffer() {
    auto screen = ClipSpace::Quad::positions;
    screenBuffer = device.createDeviceLocalBuffer(screen.data(), BYTE_SIZE(screen), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
}

RenderPassInfo DeferredRendering::buildRenderPass() {
    auto [attachments, subpassDesc, dependency] = VulkanBaseApp::buildRenderPass();

    VkAttachmentDescription albedoAttachmentDesc{};
    albedoAttachmentDesc.format = VK_FORMAT_R32G32B32A32_SFLOAT;
    albedoAttachmentDesc.samples = VK_SAMPLE_COUNT_1_BIT;
    albedoAttachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    albedoAttachmentDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    albedoAttachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    albedoAttachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    albedoAttachmentDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    albedoAttachmentDesc.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attachmentIndices[kAttachment_GBUFFER_ALBDEO] = attachments.size();
    attachments.push_back(albedoAttachmentDesc);

    VkAttachmentDescription normalAttachmentDesc{};
    normalAttachmentDesc.format = VK_FORMAT_R32G32B32_SFLOAT;
    normalAttachmentDesc.samples = VK_SAMPLE_COUNT_1_BIT;
    normalAttachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    normalAttachmentDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    normalAttachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    normalAttachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    normalAttachmentDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    normalAttachmentDesc.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attachmentIndices[kAttachment_GBUFFER_NORMAL] = attachments.size();
    attachments.push_back(normalAttachmentDesc);

    VkAttachmentDescription depthAttachmentDesc{};
    depthAttachmentDesc.format = VK_FORMAT_D32_SFLOAT;
    depthAttachmentDesc.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachmentDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depthAttachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachmentDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachmentDesc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
    attachmentIndices[kAttachment_GBUFFER_DEPTH] = attachments.size();
    attachments.push_back(depthAttachmentDesc);

    VkAttachmentReference depthRef{
        attachmentIndices[kAttachment_DEPTH],
        VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL
    };

    VkAttachmentReference gBufferAlbedoRef{
        attachmentIndices[kAttachment_GBUFFER_ALBDEO],
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };
    VkAttachmentReference gBufferNormalRef{
        attachmentIndices[kAttachment_GBUFFER_NORMAL],
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };
    VkAttachmentReference gBufferDepthRef{
        attachmentIndices[kAttachment_GBUFFER_DEPTH],
        VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
    };

    SubpassDescription depthPrepassDesc{
        0,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        {}, // input attachments
        {}, // out put attachments
        {}, // resolve attachments
        depthRef,
        {}  // preserve attachments
    };

    SubpassDescription gBufferDesc{
        0,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        {}, // input attachments
        {
            gBufferAlbedoRef,
            gBufferNormalRef,
            gBufferDepthRef
        }, // output attachments
        {}, // resolve attachments
        depthRef,
        {}, // preserve attachments
    };

    SubpassDescription renderDesc = subpassDesc.front();
    renderDesc.inputAttachments = {
            gBufferAlbedoRef,
            gBufferNormalRef,
            gBufferDepthRef
    };

    VkSubpassDependency gBufferOnDepthPrePass{
            kSubpass_DEPTH,
            kSubpass_GBUFFER,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            VK_ACCESS_SHADER_READ_BIT,
            VK_DEPENDENCY_BY_REGION_BIT
    };

    VkSubpassDependency renderOnGBuffer{
        kSubpass_GBUFFER,
        kSubpass_RENDER,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        VK_ACCESS_SHADER_READ_BIT,
        VK_DEPENDENCY_BY_REGION_BIT
    };

    subpassDesc = {depthPrepassDesc, gBufferDesc, renderDesc};
    dependency = { gBufferOnDepthPrePass, renderOnGBuffer};

    return std::make_tuple(attachments, subpassDesc, dependency);
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