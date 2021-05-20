
#include "PrimitivesApp.h"

PrimitivesApp::PrimitivesApp(const Settings &settings):VulkanBaseApp("primitives", settings) {

}

void PrimitivesApp::initApp() {
    nextPrimitive = &mapToMouse(static_cast<int>(MouseEvent::Button::LEFT), "Next primitive", Action::detectInitialPressOnly());
    commandPool = device.createCommandPool(*device.queueFamilyIndex.graphics, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    commandBuffers = commandPool.allocate(swapChainImageCount);
    createDescriptorPool();
    createPrimitives();
    initCamera();
    createDescriptorSetLayout();
    createDescriptorSet();
    createPipeline();
}


void PrimitivesApp::onSwapChainDispose() {
    VulkanBaseApp::onSwapChainDispose();
}

void PrimitivesApp::onSwapChainRecreation() {
    VulkanBaseApp::onSwapChainRecreation();
}

VkCommandBuffer *PrimitivesApp::buildCommandBuffers(uint32_t imageIndex, uint32_t &numCommandBuffers) {
    numCommandBuffers = 1;
    auto& commandBuffer = commandBuffers[imageIndex];

    VkCommandBufferBeginInfo beginInfo = initializers::commandBufferBeginInfo();

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {0.0f, 0.0f, 0.0f, 1.0f};
    clearValues[1].depthStencil = {1.0f, 0u};

    VkRenderPassBeginInfo renderPassInfo = initializers::renderPassBeginInfo();
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = framebuffers[imageIndex];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = swapChain.extent;
    renderPassInfo.clearValueCount = COUNT(clearValues);
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    auto& object = objects[currentPrimitiveIndex];
    if(object.topology == VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP){
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.triangleStripPipeline);
    }else{
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.trianglePipeline);
    }
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &descriptorSet, 0, VK_NULL_HANDLE);
    cameraController->push(commandBuffer, layout);
    vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(Camera), sizeof(lightParams), &lightParams);
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, object.vertices, &offset);
    vkCmdBindIndexBuffer(commandBuffer, object.indices, offset, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(commandBuffer, object.indexCount, 1, 0, 0, 0);

    vkCmdEndRenderPass(commandBuffer);

    vkEndCommandBuffer(commandBuffer);


    return &commandBuffer;
}

void PrimitivesApp::update(float time) {
    cameraController->update(time);
}

void PrimitivesApp::checkAppInputs() {
    if(nextPrimitive->isPressed()){
        currentPrimitiveIndex++;
        currentPrimitiveIndex %= objects.size();
    }
    cameraController->processInput();
}

struct Mesh{
    std::vector<Vertex> vertices;
};

void PrimitivesApp::createPrimitives() {
    glm::vec4 v{0};
    auto cube = primitives::cube();
    auto sphere = primitives::sphere(50, 50, 1.0, glm::mat4{1}, randomColor(), VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    auto hemisphere = primitives::hemisphere(50, 50, 0.5);
    auto cone = primitives::cone(50, 50, 0.5, 1.0);
    auto cylinder = primitives::cylinder(50, 50, 0.5, 1.0);
    auto torus = primitives::torus(100, 100, 1, 0.3);
    auto plane = primitives::plane(5, 5, 2, 2 );

    Object object;
    object.vertices = device.createDeviceLocalBuffer(cube.vertices.data(), sizeof(Vertex) * cube.vertices.size(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    object.indices = device.createDeviceLocalBuffer(cube.indices.data(), sizeof(uint32_t) * cube.indices.size(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    object.indexCount = cube.indices.size();
    object.topology = cube.topology;
    objects.push_back(std::move(object));

    object = Object{};
    object.vertices = device.createDeviceLocalBuffer(sphere.vertices.data(), sizeof(Vertex) * sphere.vertices.size(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    object.indices = device.createDeviceLocalBuffer(sphere.indices.data(), sizeof(uint32_t) * sphere.indices.size(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    object.indexCount = sphere.indices.size();
    object.topology = sphere.topology;
    objects.push_back(std::move(object));

    object = Object{};
    object.vertices = device.createDeviceLocalBuffer(hemisphere.vertices.data(), sizeof(Vertex) * hemisphere.vertices.size(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    object.indices = device.createDeviceLocalBuffer(hemisphere.indices.data(), sizeof(uint32_t) * hemisphere.indices.size(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    object.indexCount = hemisphere.indices.size();
    object.topology = hemisphere.topology;
    objects.push_back(std::move(object));

    object = Object{};
    object.vertices = device.createDeviceLocalBuffer(cone.vertices.data(), sizeof(Vertex) * cone.vertices.size(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    object.indices = device.createDeviceLocalBuffer(cone.indices.data(), sizeof(uint32_t) * cone.indices.size(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    object.indexCount = cone.indices.size();
    object.topology = cone.topology;
    objects.push_back(std::move(object));

    object = Object{};
    object.vertices = device.createDeviceLocalBuffer(cylinder.vertices.data(), sizeof(Vertex) * cylinder.vertices.size(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    object.indices = device.createDeviceLocalBuffer(cylinder.indices.data(), sizeof(uint32_t) * cylinder.indices.size(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    object.indexCount = cylinder.indices.size();
    object.topology = cylinder.topology;
    objects.push_back(std::move(object));

    object = Object{};
    object.vertices = device.createDeviceLocalBuffer(torus.vertices.data(), sizeof(Vertex) * torus.vertices.size(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    object.indices = device.createDeviceLocalBuffer(torus.indices.data(), sizeof(uint32_t) * torus.indices.size(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    object.indexCount = torus.indices.size();
    object.topology = torus.topology;
    objects.push_back(std::move(object));

    object = Object{};
    object.vertices = device.createDeviceLocalBuffer(plane.vertices.data(), sizeof(Vertex) * plane.vertices.size(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    object.indices = device.createDeviceLocalBuffer(plane.indices.data(), sizeof(uint32_t) * plane.indices.size(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    object.indexCount = plane.indices.size();
    object.topology = plane.topology;
    objects.push_back(std::move(object));

}

void PrimitivesApp::createPipeline() {
    auto vertexShaderModule = ShaderModule{"../../data/shaders/phong/phong.vert.spv", device};
    auto fragmentShaderModule = ShaderModule{"../../data/shaders/phong/phong.frag.spv", device};

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
    InputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

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
    depthStencilState.depthTestEnable = VK_TRUE;
    depthStencilState.depthWriteEnable = VK_TRUE;
    depthStencilState.minDepthBounds = 0.0f;
    depthStencilState.maxDepthBounds = 1.0f;
    depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS;

    auto colorBlendAttachment = initializers::colorBlendStateAttachmentStates();
    VkPipelineColorBlendStateCreateInfo blendState = initializers::colorBlendState(colorBlendAttachment);

    VkPipelineDynamicStateCreateInfo dynamicState = initializers::dynamicState();


    layout = device.createPipelineLayout({ setLayout }, { Camera::pushConstant(), {VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(Camera), sizeof(lightParams)} });

    VkGraphicsPipelineCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    createInfo.flags = VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT;
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

    pipelines.trianglePipeline = device.createGraphicsPipeline(createInfo);

    createInfo.flags = VK_PIPELINE_CREATE_DERIVATIVE_BIT;
    InputAssemblyState = initializers::inputAssemblyState();
    InputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
    InputAssemblyState.primitiveRestartEnable = VK_TRUE;
    createInfo.pInputAssemblyState = &InputAssemblyState;
    createInfo.basePipelineIndex = -1;
    createInfo.basePipelineHandle = pipelines.trianglePipeline.handle;

    pipelines.triangleStripPipeline = device.createGraphicsPipeline(createInfo);
}

void PrimitivesApp::initCamera() {
    OrbitingCameraSettings settings{};
    settings.offsetDistance = 5.0f;
    settings.rotationSpeed = 0.1f;
    settings.fieldOfView = 45.0f;
    settings.modelHeight = 0;
    settings.aspectRatio = static_cast<float>(swapChain.extent.width)/static_cast<float>(swapChain.extent.height);
    cameraController = std::make_unique<OrbitingCameraController>(device, swapChain.imageCount(), currentImageIndex, dynamic_cast<InputManager&>(*this), settings);
}

void PrimitivesApp::createDescriptorPool() {
    std::vector<VkDescriptorPoolSize> poolSizes(1);
    poolSizes[0].descriptorCount = 1;
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

    descriptorPool = device.createDescriptorPool(1, poolSizes, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT);
}

void PrimitivesApp::createDescriptorSetLayout() {
    std::vector<VkDescriptorSetLayoutBinding> binding(1);
    binding[0].binding = 0;
    binding[0].descriptorCount = 1;
    binding[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    binding[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    setLayout = device.createDescriptorSetLayout(binding);
}

void PrimitivesApp::createDescriptorSet() {
    descriptorSet = descriptorPool.allocate({setLayout}).front();
}


int main(){
    try{
        Settings settings{};
        settings.relativeMouseMode = true;
        settings.depthTest = true;
        settings.enabledFeatures.fillModeNonSolid = VK_TRUE;
        auto app = PrimitivesApp(settings);
        app.run();
    } catch (std::runtime_error& err) {
        spdlog::error(err.what());
    }
    return 0;
}