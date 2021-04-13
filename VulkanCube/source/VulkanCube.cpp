#include "VulkanCube.h"

#include  <stb_image.h>

VulkanCube::VulkanCube(const Settings& settings): VulkanBaseApp("VulkanCube", settings, {}){}

void VulkanCube::initApp() {
    createCommandPool();

    mesh = primitives::cube();
//    mesh = primitives::sphere(50, 50, 0.5f);
    createVertexBuffer();
    createIndexBuffer();
    createTextureBuffers();

    createDescriptorSetLayout();
    createDescriptorPool();
    createDescriptorSet();

    OrbitingCameraSettings settings{};
    settings.offsetDistance = 2.0f;
    settings.rotationSpeed = 0.1f;
    settings.zNear = 0.1f;
    settings.zFar = 10.0f;
    settings.fieldOfView = 45.0f;
    settings.modelHeight = 0;
    settings.aspectRatio = static_cast<float>(swapChain.extent.width)/static_cast<float>(swapChain.extent.height);
    cameraController = std::unique_ptr<BaseCameraController>{new OrbitingCameraController{ device, swapChain.imageCount(), currentImageIndex, *this, settings}};
   // cameraController->lookAt({0, 0, 2}, glm::vec3(0), {0, 1, 0});
    pipelineCache = device.createPipelineCache();
    createGraphicsPipeline();
    createCommandBuffer();

}

void VulkanCube::onSwapChainDispose() {
    commandPool.free(commandBuffers);
    dispose(pipeline);
    dispose(layout);
    dispose(descriptorPool);
    dispose(descriptorSetLayout);
}

void VulkanCube::onSwapChainRecreation() {
    createDescriptorSetLayout();
    createDescriptorPool();
    createDescriptorSet();

    cameraController->onResize(swapChain.extent.width, swapChain.extent.height);
    createGraphicsPipeline();
    createCommandBuffer();
}

void VulkanCube::
createGraphicsPipeline() {
    assert(renderPass != VK_NULL_HANDLE);
    auto vertexShaderModule = ShaderModule{"../../data/shaders/triangle.vert.spv", device};
    auto fragmentShaderModule = ShaderModule{"../../data/shaders/triangle.frag.spv", device};

    std::vector<VkPipelineShaderStageCreateInfo> stages = initializers::vertexShaderStages(
            {
                    {vertexShaderModule,   VK_SHADER_STAGE_VERTEX_BIT},
                    {fragmentShaderModule, VK_SHADER_STAGE_FRAGMENT_BIT}
            }
    );

    std::vector<VkVertexInputBindingDescription> vertexBindings{Vertex::bindingDisc()};
    std::vector<VkVertexInputAttributeDescription> vertexAttributes = Vertex::attributeDisc();
    VkPipelineVertexInputStateCreateInfo vertexInputState = initializers::vertexInputState(vertexBindings, vertexAttributes);


    VkPipelineInputAssemblyStateCreateInfo InputAssemblyState = initializers::inputAssemblyState();

    VkViewport viewport = initializers::viewport(swapChain.extent);

    VkRect2D scissor{};
    scissor.extent = swapChain.extent;
    scissor.offset = {0, 0};

    VkPipelineViewportStateCreateInfo viewportState = initializers::viewportState(viewport, scissor);

    VkPipelineRasterizationStateCreateInfo rasterizationState = initializers::rasterizationState();
    rasterizationState.rasterizerDiscardEnable = VK_FALSE;
    rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizationState.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizationState.depthBiasEnable = VK_FALSE;
    rasterizationState.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo multisampleState = initializers::multisampleState();


    VkPipelineDepthStencilStateCreateInfo depthStencilState = initializers::depthStencilState();

    std::vector<VkPipelineColorBlendAttachmentState> blendAttachment(1);
    blendAttachment[0].blendEnable = VK_FALSE;
    blendAttachment[0].colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT
            | VK_COLOR_COMPONENT_G_BIT
            | VK_COLOR_COMPONENT_B_BIT
            | VK_COLOR_COMPONENT_A_BIT;
    VkPipelineColorBlendStateCreateInfo blendState = initializers::colorBlendState(blendAttachment);

    VkPipelineDynamicStateCreateInfo dynamicState = initializers::dynamicState();


    layout = device.createPipelineLayout({descriptorSetLayout }, { Camera::pushConstant() });

    VkGraphicsPipelineCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    createInfo.stageCount = static_cast<uint32_t>(stages.size());
    createInfo.pStages = stages.data();
    createInfo.pVertexInputState = &vertexInputState;
    createInfo.pInputAssemblyState = &InputAssemblyState;
    createInfo.pTessellationState = nullptr;
    createInfo.pViewportState = &viewportState;
    createInfo.pRasterizationState = &rasterizationState;
    createInfo.pMultisampleState  = &multisampleState;
    createInfo.pDepthStencilState = &depthStencilState;
    createInfo.pColorBlendState = &blendState;
    createInfo.pDynamicState = &dynamicState;
    createInfo.layout = layout;
    createInfo.renderPass = renderPass;
    createInfo.subpass = 0;
    createInfo.basePipelineIndex = -1;
    createInfo.basePipelineHandle = VK_NULL_HANDLE;

    pipeline = device.createGraphicsPipeline(createInfo, pipelineCache);
}

VkCommandBuffer* VulkanCube::buildCommandBuffers(uint32_t imageIndex, uint32_t& numCommandBuffers) {
    numCommandBuffers = 1;
    uint32_t i = imageIndex;
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0;
    vkBeginCommandBuffer(commandBuffers[i], &beginInfo);

    VkClearValue clearValues[2];
    clearValues[0].color = {0.0f, 0.0f, 0.0f, 1.0f};
    clearValues[1].depthStencil = {1.0, 0u};

    VkRenderPassBeginInfo renderPassBeginInfo{};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.renderPass = renderPass;
    renderPassBeginInfo.framebuffer = framebuffers[i];
    renderPassBeginInfo.renderArea.offset = {0, 0};
    renderPassBeginInfo.renderArea.extent = swapChain.extent;
    renderPassBeginInfo.clearValueCount = 2;
    renderPassBeginInfo.pClearValues = clearValues;
    vkCmdBeginRenderPass(commandBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE );

    std::vector<VkDescriptorSet> descriptorSets{ descriptorSet };
 //   std::vector<VkDescriptorSet> descriptorSets{ cameraController->descriptorSet(i), descriptorSet };

    vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, COUNT(descriptorSets), descriptorSets.data(), 0, nullptr);
    vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline );
    cameraController->push(commandBuffers[i], layout);
    VkDeviceSize offset = 0;
    vkCmdBindIndexBuffer(commandBuffers[i], indexBuffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, &vertexBuffer.buffer, &offset);
    vkCmdDrawIndexed(commandBuffers[i], static_cast<uint32_t>(mesh.indices.size()), 1u, 0u, 0u, 0u);
//        vkCmdDraw(commandBuffers[i], 3, 1, 0, 0);

//    vkCmdNextSubpass(commandBuffers[i], VK_SUBPASS_CONTENTS_INLINE);

    vkCmdEndRenderPass(commandBuffers[i]);

    vkEndCommandBuffer(commandBuffers[i]);
    return &commandBuffers[imageIndex];
}

void VulkanCube::update(float time) {
    cameraController->update(time);
//    font->clear();
//    font->write(fmt::format("Frames: {}\nFPS: {}", frameCount, framePerSecond), 20, 50);
}

void VulkanCube::checkAppInputs() {
    cameraController->processInput();
}

void VulkanCube::createCommandBuffer() {
    commandBuffers.resize(swapChain.imageCount() + 1);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = commandBuffers.size();

    REPORT_ERROR(vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()), "Failed to allocate command buffer");

    auto last = commandBuffers.begin();
    std::advance(last, commandBuffers.size() - 1);
    commandBuffers.erase(last);
}

void VulkanCube::createDescriptorPool() {


    VkDescriptorPoolSize  texturePoolSize{};
    texturePoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    texturePoolSize.descriptorCount = 1;

    std::vector<VkDescriptorPoolSize> poolSizes{
            texturePoolSize
    };

    descriptorPool = device.createDescriptorPool(swapChain.imageCount(), poolSizes);


}

void VulkanCube::createDescriptorSet() {
    const auto swapChainImageCount = 1;

    std::vector<VkDescriptorSetLayout> layouts(swapChainImageCount, descriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &descriptorSetLayout.handle;
    vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet);

    VkWriteDescriptorSet textureWrites{};
    textureWrites.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    textureWrites.dstSet = descriptorSet;
    textureWrites.dstBinding = 0;
    textureWrites.dstArrayElement = 0;
    textureWrites.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    textureWrites.descriptorCount = 1;

    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = texture.imageView;
    imageInfo.sampler = texture.sampler;
    textureWrites.pImageInfo = &imageInfo;

    std::vector<VkWriteDescriptorSet> writes{
            textureWrites,
    };

    vkUpdateDescriptorSets(device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
}

void VulkanCube::createTextureBuffers() {
    textures::fromFile(device, texture, "../../data/textures/vulkan.png");
}

void VulkanCube::createVertexBuffer() {

    VkDeviceSize size = sizeof(Vertex) * mesh.vertices.size();
    vertexBuffer = device.createDeviceLocalBuffer(mesh.vertices.data(), size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
}

void VulkanCube::createIndexBuffer() {
    VkDeviceSize size = sizeof(mesh.indices[0]) * mesh.indices.size();
    indexBuffer = device.createDeviceLocalBuffer(mesh.indices.data(), size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
}

void VulkanCube::createCommandPool() {
    commandPool = device.createCommandPool(*device.queueFamilyIndex.graphics, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
}

void VulkanCube::createDescriptorSetLayout() {
    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding = 0;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.pImmutableSamplers = nullptr;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    std::vector<VkDescriptorSetLayoutBinding> bindings {
            samplerLayoutBinding
    };

    VkDescriptorSetLayoutCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    createInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    createInfo.pBindings = bindings.data();

    descriptorSetLayout = device.createDescriptorSetLayout(bindings);
}

void VulkanCube::cleanup() {
 //   Fonts::cleanup();
    VulkanBaseApp::cleanup();
}

RenderPassInfo VulkanCube::buildRenderPass() {
    auto info =  VulkanBaseApp::buildRenderPass();

    return info;
}

int main() {
    try{
        Settings settings;
        settings.vSync = false;
        settings.depthTest = true;
        settings.relativeMouseMode = false;
        settings.width = 1080;
        settings.height = 720;
        VulkanCube app{settings};
        app.run();
    }catch(const std::runtime_error& err){
        spdlog::error(err.what());
    }
    return 0;
}