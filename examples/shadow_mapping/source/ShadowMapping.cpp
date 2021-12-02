#include "ShadowMapping.hpp"
#include "GraphicsPipelineBuilder.hpp"
#include "DescriptorSetBuilder.hpp"

ShadowMapping::ShadowMapping(const Settings& settings) : VulkanBaseApp("Shadow mapping", settings) {
    fileManager.addSearchPath(".");
    fileManager.addSearchPath("../../examples/shadow_mapping/spv");
    fileManager.addSearchPath("../../data/shaders");
    fileManager.addSearchPath("../../data");
    fileManager.addSearchPath("../../data/models");
    fileManager.addSearchPath("../../data/textures");
}

void ShadowMapping::initApp() {
    createDescriptorPool();
    createCommandPool();
    initCamera();
    createFloor();
    createPipelineCache();
    createDescriptorSetLayouts();
    initShadowMap();
    initFrustum();
    createRenderPipeline();
    updateShadowMapDescriptorSet();
    createComputePipeline();
}

void ShadowMapping::createDescriptorPool() {
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

void ShadowMapping::createCommandPool() {
    commandPool = device.createCommandPool(*device.queueFamilyIndex.graphics, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    commandBuffers = commandPool.allocateCommandBuffers(swapChainImageCount);
}

void ShadowMapping::createPipelineCache() {
    pipelineCache = device.createPipelineCache();
}


void ShadowMapping::createRenderPipeline() {
    auto builder = device.graphicsPipelineBuilder();
    render.pipeline =
        builder
            .shaderStage()
                .vertexShader(load("cube.vert.spv"))
                .fragmentShader(load("cube.frag.spv"))
            .vertexInputState()
                .addVertexBindingDescription(0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX)
                .addVertexBindingDescription(1, sizeof(glm::mat4), VK_VERTEX_INPUT_RATE_INSTANCE)
                .addVertexAttributeDescription(0, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetOf(Vertex, position))
                .addVertexAttributeDescription(1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetOf(Vertex, normal))
                .addVertexAttributeDescription(2, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetOf(Vertex, color))
                .addVertexAttributeDescription(3, 1, VK_FORMAT_R32G32B32A32_SFLOAT, 0)
                .addVertexAttributeDescription(4, 1, VK_FORMAT_R32G32B32A32_SFLOAT, 16)
                .addVertexAttributeDescription(5, 1, VK_FORMAT_R32G32B32A32_SFLOAT, 32)
                .addVertexAttributeDescription(6, 1, VK_FORMAT_R32G32B32A32_SFLOAT, 48)
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
                    .attachment()
                    .add()
                .layout()
                    .addPushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Camera) + sizeof(glm::mat4))
                    .addDescriptorSetLayout(render.shadowMapDescriptorSetLayout)
                .renderPass(renderPass)
                .subpass(0)
                .name("render")
                .pipelineCache(pipelineCache)
            .build(render.layout);
}

void ShadowMapping::createComputePipeline() {
    auto module = VulkanShaderModule{ "../../data/shaders/pass_through.comp.spv", device};
    auto stage = initializers::shaderStage({ module, VK_SHADER_STAGE_COMPUTE_BIT});

    compute.layout = device.createPipelineLayout();

    auto computeCreateInfo = initializers::computePipelineCreateInfo();
    computeCreateInfo.stage = stage;
    computeCreateInfo.layout = compute.layout;

    compute.pipeline = device.createComputePipeline(computeCreateInfo, pipelineCache);
}


void ShadowMapping::onSwapChainDispose() {
    dispose(render.pipeline);
    dispose(compute.pipeline);
}

void ShadowMapping::onSwapChainRecreation() {
    createRenderPipeline();
    createComputePipeline();
}

VkCommandBuffer *ShadowMapping::buildCommandBuffers(uint32_t imageIndex, uint32_t &numCommandBuffers) {
    numCommandBuffers = 1;
    auto& commandBuffer = commandBuffers[imageIndex];


    VkCommandBufferBeginInfo beginInfo = initializers::commandBufferBeginInfo();
    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    constructShadowMap(commandBuffer);

    static std::array<VkBuffer, 2> buffers;
    buffers[0] = cubes.vertices;
    buffers[1] = cubes.transforms;
    static std::array<VkDeviceSize,2> offsets{0, 0};
    vkCmdBindVertexBuffers(commandBuffer, 0, COUNT(buffers), buffers.data(), offsets.data());
    vkCmdBindIndexBuffer(commandBuffer, cubes.indices, 0, VK_INDEX_TYPE_UINT32);


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

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, render.pipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, render.layout, 0, 1, &render.shadowMapDescriptorSet, 0, nullptr);
    cameraController->push(commandBuffer, render.layout);
//    Camera camera{ glm::mat4{1}, shadowMap.lightView, shadowMap.lightProjection};
//    vkCmdPushConstants(commandBuffer, render.layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Camera), &camera);
    vkCmdPushConstants(commandBuffer, render.layout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(Camera), sizeof(glm::mat4), &shadowMap.lightSpaceMatrix);
    drawCubes(commandBuffer);

    drawFrustum(commandBuffer);
    vkCmdEndRenderPass(commandBuffer);

    vkEndCommandBuffer(commandBuffer);

    return &commandBuffer;
}

void ShadowMapping::constructShadowMap(VkCommandBuffer commandBuffer) {

    static std::array<VkBuffer, 2> buffers;
    buffers[0] = cubes.vertices;
    buffers[1] = cubes.transforms;
    static std::array<VkDeviceSize,2> offsets{0, 0};
    vkCmdBindVertexBuffers(commandBuffer, 0, COUNT(buffers), buffers.data(), offsets.data());
    vkCmdBindIndexBuffer(commandBuffer, cubes.indices, 0, VK_INDEX_TYPE_UINT32);

    static std::array<VkClearValue, 1> clearValues;
    clearValues[0].depthStencil = {1.0, 0u};

    VkRenderPassBeginInfo rPassInfo = initializers::renderPassBeginInfo();
    rPassInfo.clearValueCount = COUNT(clearValues);
    rPassInfo.pClearValues = clearValues.data();
    rPassInfo.framebuffer = shadowMap.framebuffer;
    rPassInfo.renderArea.offset = {0u, 0u};
    rPassInfo.renderArea.extent = {shadowMap.size, shadowMap.size };
    rPassInfo.renderPass = shadowMap.renderPass;

    vkCmdBeginRenderPass(commandBuffer, &rPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    shadowMap.lightSpaceMatrix = shadowMap.lightProjection * shadowMap.lightView;

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shadowMap.pipeline);
    vkCmdPushConstants(commandBuffer, shadowMap.layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &shadowMap.lightSpaceMatrix);
    drawCubes(commandBuffer);

    vkCmdEndRenderPass(commandBuffer);

}

void ShadowMapping::drawCubes(VkCommandBuffer commandBuffer) {
    uint32_t indexCount = cubes.indices.size / sizeof(uint32_t);
    uint32_t instanceCount = cubes.instanceCount;
    vkCmdDrawIndexed(commandBuffer, indexCount, instanceCount, 0, 0, 0);
}

void ShadowMapping::update(float time) {
    cameraController->update(time);
}

void ShadowMapping::checkAppInputs() {
    cameraController->processInput();
}

void ShadowMapping::cleanup() {
    vkDestroyEvent(device, createShadowMapEvent, VK_NULL_HANDLE);
    vkDestroyEvent(device, shadowMapReadyEvent, VK_NULL_HANDLE);
}

void ShadowMapping::onPause() {
    VulkanBaseApp::onPause();
}

void ShadowMapping::initCamera() {
    CameraSettings settings{};
    settings.aspectRatio = static_cast<float>(swapChain.extent.width)/static_cast<float>(swapChain.extent.height);
    settings.rotationSpeed = 0.1f;
    settings.horizontalFov = true;
    settings.orbit.modelHeight = 0.5;
    cameraController = std::make_unique<CameraController>(device, swapChain.imageCount(), currentImageIndex, dynamic_cast<InputManager&>(*this), settings);
    cameraController->setMode(CameraMode::SPECTATOR);
    cameraController->lookAt({20, 7, 20}, {0, 0, 0}, {0, 1, 0});
}

void ShadowMapping::createFloor() {
    auto vertices = primitives::cube({0.6, 0.6, 0.6, 1.0});
    cubes.vertices = device.createDeviceLocalBuffer(vertices.vertices.data(), BYTE_SIZE(vertices.vertices), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    cubes.indices = device.createDeviceLocalBuffer(vertices.indices.data(), BYTE_SIZE(vertices.indices), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);

    auto spacing = 5.0f;
    auto halfSize = 0.5f;
    auto n = 20;
    auto width = (halfSize * 2 + spacing) * n;
    std::vector<glm::mat4> xforms;
    glm::mat4 base = glm::scale(glm::mat4(1), {1000, 0.2, 1000}) * glm::translate(glm::mat4(1), {0, -0.5, 0});
    xforms.push_back(base);

    auto offset = 0.5 - width * 0.5;
    for(int i = 0; i < n; i++){
        for(int j = 0; j < n; j++){
            float x = offset + (halfSize * 2 + spacing) * float(i);
            float z = offset + (halfSize * 2 + spacing) * float(j);
            auto pos = glm::vec3(x, 2.0, z);
//            spdlog::info("index: [{}, {}], position: {}", j, i, pos);
            glm::mat4 cube = glm::translate(glm::mat4(1), pos);
            xforms.push_back(cube);
        }
    }
    cubes.instanceCount = xforms.size();
    cubes.transforms = device.createDeviceLocalBuffer(xforms.data(), BYTE_SIZE(xforms), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
}

void ShadowMapping::initShadowMap() {
    VkFormat format = VK_FORMAT_D16_UNORM;
    auto createFrameBufferAttachment = [&]{
        VkImageCreateInfo createInfo = initializers::imageCreateInfo(
                VK_IMAGE_TYPE_2D,
                format,
                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                shadowMap.size,
                shadowMap.size);

        shadowMap.framebufferAttachment.image = device.createImage(createInfo, VMA_MEMORY_USAGE_GPU_ONLY);

        VkImageSubresourceRange subresourceRange = initializers::imageSubresourceRange(VK_IMAGE_ASPECT_DEPTH_BIT);
        shadowMap.framebufferAttachment.imageView = shadowMap.framebufferAttachment.image.createView(format, VK_IMAGE_VIEW_TYPE_2D, subresourceRange);

        auto samplerInfo = initializers::samplerCreateInfo();
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        samplerInfo.addressModeV = samplerInfo.addressModeU;
        samplerInfo.addressModeW = samplerInfo.addressModeU;
        samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        shadowMap.sampler = device.createSampler(samplerInfo);
    };

    auto createRenderPass = [&]{
        VkAttachmentDescription attachmentDesc{
            0,
            format,
            VK_SAMPLE_COUNT_1_BIT,
            VK_ATTACHMENT_LOAD_OP_CLEAR,
            VK_ATTACHMENT_STORE_OP_STORE,
            VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            VK_ATTACHMENT_STORE_OP_DONT_CARE,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
        };

        SubpassDescription subpass{};
        subpass.depthStencilAttachments = {0, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

        // Use subpass dependencies for layout transitions
        std::vector<VkSubpassDependency> dependencies(2);

        dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[0].dstSubpass = 0;
        dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        dependencies[1].srcSubpass = 0;
        dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[1].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dependencies[1].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        shadowMap.renderPass = device.createRenderPass({attachmentDesc}, {subpass}, {dependencies});
    };

    auto createFrameBuffer = [&]{
        std::vector<VkImageView> attachments{ shadowMap.framebufferAttachment.imageView };
        shadowMap.framebuffer = device.createFramebuffer(shadowMap.renderPass, attachments, shadowMap.size, shadowMap.size);
    };

    auto createPipeline = [&]{
        shadowMap.pipeline =
            device.graphicsPipelineBuilder()
                .shaderStage()
                    .vertexShader(load("shadowmap.vert.spv"))
                    .fragmentShader(load("shadowmap.frag.spv"))
                .vertexInputState()
                    .addVertexBindingDescription(0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX)
                    .addVertexBindingDescription(1, sizeof(glm::mat4), VK_VERTEX_INPUT_RATE_INSTANCE)
                    .addVertexAttributeDescription(0, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetOf(Vertex, position))
                    .addVertexAttributeDescription(1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetOf(Vertex, normal))
                    .addVertexAttributeDescription(2, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetOf(Vertex, color))
                    .addVertexAttributeDescription(3, 1, VK_FORMAT_R32G32B32A32_SFLOAT, 0)
                    .addVertexAttributeDescription(4, 1, VK_FORMAT_R32G32B32A32_SFLOAT, 16)
                    .addVertexAttributeDescription(5, 1, VK_FORMAT_R32G32B32A32_SFLOAT, 32)
                    .addVertexAttributeDescription(6, 1, VK_FORMAT_R32G32B32A32_SFLOAT, 48)
                .inputAssemblyState()
                    .triangles()
                .viewportState()
                    .viewport()
                        .origin(0, 0)
                        .dimension(shadowMap.size, shadowMap.size)
                        .minDepth(0)
                        .maxDepth(1)
                    .scissor()
                        .offset(0, 0)
                        .extent(shadowMap.size, shadowMap.size)
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
                    .attachment()
                    .add()
                .layout()
                    .addPushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4))
                .renderPass(shadowMap.renderPass)
                .subpass(0)
                .name("shadow_map")
                .pipelineCache(pipelineCache)
            .build(shadowMap.layout);
    };

    auto constructLightSpaceMatrix = [&]{
        shadowMap.lightProjection = vkn::ortho(-10.f, 10.f, -10.f, 10.f, 1.0f, 100.f);
        shadowMap.lightView = glm::lookAt(lightDir.xyz(), glm::vec3(0), {0, 1, 0});
    };

    auto createEvents = [&]{
        VkEventCreateInfo info{ VK_STRUCTURE_TYPE_EVENT_CREATE_INFO, VK_NULL_HANDLE };
        vkCreateEvent(device, &info, nullptr, &createShadowMapEvent);
        vkCreateEvent(device, &info, VK_NULL_HANDLE, &shadowMapReadyEvent);
        device.graphicsCommandPool().oneTimeCommand([&](auto commandBuffer){
            vkCmdSetEvent(commandBuffer, createShadowMapEvent, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
        });
    };

    createFrameBufferAttachment();
    createRenderPass();
    createFrameBuffer();
    createPipeline();
    constructLightSpaceMatrix();
    createEvents();
}

void ShadowMapping::waitToCreateShadowMap(VkCommandBuffer commandBuffer) {
    vkCmdWaitEvents(
            commandBuffer,
            1,
            &createShadowMapEvent,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0,
            VK_NULL_HANDLE,
            0,
            VK_NULL_HANDLE,
            0,
            VK_NULL_HANDLE
            );
}

void ShadowMapping::waitForShadowMapToBeReady(VkCommandBuffer commandBuffer) {
    VkImageMemoryBarrier imageMemoryBarrier{
      VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
      VK_NULL_HANDLE,
      VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
      VK_ACCESS_SHADER_READ_BIT,
      VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
    };
    imageMemoryBarrier.image = shadowMap.framebufferAttachment.image;
    imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    imageMemoryBarrier.subresourceRange.layerCount = 1;
    imageMemoryBarrier.subresourceRange.levelCount = 1;
    imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
    imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
    vkCmdWaitEvents(
            commandBuffer,
            1,
            &createShadowMapEvent,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0,
            VK_NULL_HANDLE,
            0,
            VK_NULL_HANDLE,
            1,
            &imageMemoryBarrier
    );
}

void ShadowMapping::createDescriptorSetLayouts() {
    auto builder = device.descriptorSetLayoutBuilder();

    render.shadowMapDescriptorSetLayout =
        builder
            .binding(0)
                .descriptorCount(1)
                .descriptorType(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
                .shaderStages(VK_SHADER_STAGE_FRAGMENT_BIT)
            .createLayout();
}

void ShadowMapping::updateShadowMapDescriptorSet() {
    render.shadowMapDescriptorSet = descriptorPool.allocate({render.shadowMapDescriptorSetLayout}).front();
    device.setName<VK_OBJECT_TYPE_DESCRIPTOR_SET>("shadow_map", render.shadowMapDescriptorSet);
    auto writes = initializers::writeDescriptorSets<1>();

    VkDescriptorImageInfo imageInfo{ shadowMap.sampler, shadowMap.framebufferAttachment.imageView, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL };
    writes[0].dstSet = render.shadowMapDescriptorSet;
    writes[0].dstBinding = 0;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[0].descriptorCount = 1;
    writes[0].pImageInfo = &imageInfo;
    
    device.updateDescriptorSets(writes);
}

void ShadowMapping::initFrustum() {
    auto createFrustum = [&]{
        auto vertices = Ndc::points;
        auto worldSpaceMatrix = glm::inverse(shadowMap.lightProjection * shadowMap.lightView);
        for(auto& vertex : vertices){
            vertex = worldSpaceMatrix * vertex;
        }
        frustum.vertices = device.createDeviceLocalBuffer(vertices.data(), BYTE_SIZE(vertices), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
        frustum.indices = device.createDeviceLocalBuffer(Ndc::indices.data(), BYTE_SIZE(Ndc::indices), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    };

    auto createFrustumPipeline = [&]{
        frustum.pipeline =
            device.graphicsPipelineBuilder()
                .shaderStage()
                    .vertexShader(load("frustum.vert.spv"))
                    .fragmentShader(load("frustum.frag.spv"))
                .vertexInputState()
                    .addVertexBindingDescription(0, sizeof(glm::vec4), VK_VERTEX_INPUT_RATE_VERTEX)
                    .addVertexAttributeDescription(0, 0, VK_FORMAT_R32G32B32A32_SFLOAT, 0)
                .inputAssemblyState()
                    .lines()
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
                    .cullNone()
                    .frontFaceCounterClockwise()
                    .polygonModeFill()
                    .lineWidth(3.0)
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
                .renderPass(renderPass)
                .subpass(0)
                .name("frustum")
                .pipelineCache(pipelineCache)
            .build(frustum.layout);

    };

    createFrustum();
    createFrustumPipeline();
}

void ShadowMapping::drawFrustum(VkCommandBuffer commandBuffer) {
    static VkDeviceSize offset = 0;
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, frustum.pipeline);
    cameraController->push(commandBuffer, frustum.layout);
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, frustum.vertices, &offset);
    vkCmdBindIndexBuffer(commandBuffer, frustum.indices, 0, VK_INDEX_TYPE_UINT32);
    uint32_t indexCount = frustum.indices.size/sizeof(uint32_t);
    vkCmdDrawIndexed(commandBuffer, indexCount, 1, 0, 0, 0);
}


int main(){
    try{

        Settings settings;
        settings.depthTest = true;
        settings.enabledFeatures.wideLines = true;
//        settings.deviceExtensions.push_back(VK_KHR_SEPARATE_DEPTH_STENCIL_LAYOUTS_EXTENSION_NAME);

        auto app = ShadowMapping{ settings };
        app.run();
    }catch(std::runtime_error& err){
        spdlog::error(err.what());
    }
}