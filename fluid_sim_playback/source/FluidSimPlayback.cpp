#include "FluidSimPlayback.hpp"

FluidSimPlayback::FluidSimPlayback(const Settings& settings) : VulkanBaseApp("fluid sim playback", settings) {

}

void FluidSimPlayback::initApp() {
    loadAnimation();
    initCamera();
    createDescriptorPool();
    createCommandPool();
    createPipelineCache();
    createRenderPipeline();
    createComputePipeline();
}

void FluidSimPlayback::loadAnimation() {
    animation = buildAnimation(R"(C:\Users\Josiah\OneDrive\media\water_drop_pcs\)", numFrames, numPoints, fps);
//    animation = buildAnimation(R"(C:\Users\Josiah\OneDrive\media\dam_break\)", numFrames, numPoints, fps);
    vertexBuffer = device.createBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                 VMA_MEMORY_USAGE_CPU_TO_GPU, numPoints * sizeof(glm::vec4));
}

void FluidSimPlayback::createDescriptorPool() {
    constexpr uint32_t maxSets = 100;
    std::array<VkDescriptorPoolSize, 17> poolSizes{
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
                    { VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 100 * maxSets },
                    { VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV, 100 * maxSets },

            }
    };
    descriptorPool = device.createDescriptorPool(maxSets, poolSizes, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT);

}

void FluidSimPlayback::createCommandPool() {
    commandPool = device.createCommandPool(*device.queueFamilyIndex.graphics, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    commandBuffers = commandPool.allocate(swapChainImageCount);
}

void FluidSimPlayback::createPipelineCache() {
    pipelineCache = device.createPipelineCache();
}


void FluidSimPlayback::createRenderPipeline() {
    auto vertModule = VulkanShaderModule{ "../../data/shaders/point.vert.spv", device};
    auto fragModule = VulkanShaderModule{"../../data/shaders/point.frag.spv", device};

    auto shaderStages = initializers::vertexShaderStages({
             { vertModule, VK_SHADER_STAGE_VERTEX_BIT },
             {fragModule, VK_SHADER_STAGE_FRAGMENT_BIT}
     });

    auto bindings = Vertex::bindingDisc();
    auto attribs = Vertex::attributeDisc();

    auto vertexInputState = initializers::vertexInputState(bindings, attribs);
    auto inputAssemblyState = initializers::inputAssemblyState();
    inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;

    auto viewport = initializers::viewport(swapChain.extent);
    auto scissor = initializers::scissor(swapChain.extent);

    auto viewportState = initializers::viewportState(viewport, scissor);

    auto rasterizationState = initializers::rasterizationState();
    rasterizationState.cullMode = VK_CULL_MODE_NONE;
    rasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;

    auto multisampleState = initializers::multisampleState();

    auto depthStencilState = initializers::depthStencilState();
    depthStencilState.depthTestEnable = VK_TRUE;
    depthStencilState.depthWriteEnable = VK_TRUE;
    depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencilState.minDepthBounds = 0.0;
    depthStencilState.maxDepthBounds = 1.0;

    auto colorBlendAttachment = initializers::colorBlendStateAttachmentStates();
    auto colorBlendState = initializers::colorBlendState(colorBlendAttachment);

    render.layout = device.createPipelineLayout({}, { {VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Camera) + sizeof(glm::vec4) }});

    auto createInfo = initializers::graphicsPipelineCreateInfo();
    createInfo.flags = VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT;
    createInfo.stageCount = COUNT(shaderStages);
    createInfo.pStages = shaderStages.data();
    createInfo.pVertexInputState = &vertexInputState;
    createInfo.pInputAssemblyState = &inputAssemblyState;
    createInfo.pViewportState = &viewportState;
    createInfo.pRasterizationState = &rasterizationState;
    createInfo.pMultisampleState = &multisampleState;
    createInfo.pDepthStencilState = &depthStencilState;
    createInfo.pColorBlendState = &colorBlendState;
    createInfo.layout = render.layout;
    createInfo.renderPass = renderPass;
    createInfo.subpass = 0;

    render.pipeline = device.createGraphicsPipeline(createInfo, pipelineCache);
}

void FluidSimPlayback::createComputePipeline() {
    auto module = VulkanShaderModule{ "../../data/shaders/pass_through.comp.spv", device};
    auto stage = initializers::shaderStage({ module, VK_SHADER_STAGE_COMPUTE_BIT});

    compute.layout = device.createPipelineLayout();

    auto computeCreateInfo = initializers::computePipelineCreateInfo();
    computeCreateInfo.stage = stage;
    computeCreateInfo.layout = compute.layout;

    compute.pipeline = device.createComputePipeline(computeCreateInfo, pipelineCache);
}


void FluidSimPlayback::onSwapChainDispose() {
    dispose(render.pipeline);
    dispose(compute.pipeline);
}

void FluidSimPlayback::onSwapChainRecreation() {
    createRenderPipeline();
    createComputePipeline();
}

VkCommandBuffer *FluidSimPlayback::buildCommandBuffers(uint32_t imageIndex, uint32_t &numCommandBuffers) {
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

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, render.pipeline);
    vkCmdPushConstants(commandBuffer, render.layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Camera), &camera->cam());
    vkCmdPushConstants(commandBuffer, render.layout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(Camera), sizeof(glm::vec4), &pointColor);
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffer, &offset);
    vkCmdDraw(commandBuffer, numPoints, 1, 0, 0);
    vkCmdEndRenderPass(commandBuffer);

    vkEndCommandBuffer(commandBuffer);

    return &commandBuffer;
}

void FluidSimPlayback::update(float time) {
    camera->update(time);
    auto points = animation.next(uint64_t(elapsedTime * 1000u));
    if(!points.empty()){
        vertexBuffer.copy(points.data(), sizeof(glm::vec4) * numPoints);
    }

}

void FluidSimPlayback::checkAppInputs() {
    camera->processInput();
}

void FluidSimPlayback::cleanup() {
    // TODO save pipeline cache
    VulkanBaseApp::cleanup();
}

void FluidSimPlayback::onPause() {
    VulkanBaseApp::onPause();
}

void FluidSimPlayback::initCamera() {
    OrbitingCameraSettings settings{};
    settings.offsetDistance = 5.0f;
    settings.rotationSpeed = 0.1f;
    settings.fieldOfView = 45.0f;
    settings.modelHeight = 0;
    settings.aspectRatio = static_cast<float>(swapChain.extent.width)/static_cast<float>(swapChain.extent.height);
    camera = std::make_unique<OrbitingCameraController>(device, swapChain.imageCount(), currentImageIndex, dynamic_cast<InputManager&>(*this), settings);
}


int main(){
    try{

        Settings settings;
        settings.depthTest = true;

        auto app = FluidSimPlayback{ settings };
        app.run();
    }catch(std::runtime_error& err){
        spdlog::error(err.what());
    }
}