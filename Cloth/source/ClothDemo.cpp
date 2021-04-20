#include "ClothDemo.hpp"
#include "primitives.h"

ClothDemo::ClothDemo(const Settings &settings) : VulkanBaseApp("Cloth", settings) {

}

void ClothDemo::initApp() {
    commandPool = device.createCommandPool(*device.queueFamilyIndex.graphics, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    commandBuffers = commandPool.allocate(swapChainImageCount);
    createCloth();
    initCamera();
    createPipelines();
}

void ClothDemo::createCloth() {
    auto xform = glm::mat4(1);
    auto plane = primitives::plane(10, 10, cloth.size, cloth.size, xform, glm::vec4(1));
    cloth.vertices = device.createDeviceLocalBuffer(plane.vertices.data(), sizeof(Vertex) * plane.vertices.size()
                                                    , VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    cloth.vertexCount = plane.vertices.size();

    cloth.indices = device.createDeviceLocalBuffer(plane.indices.data(), sizeof(uint32_t) * plane.indices.size(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    cloth.indexCount = plane.indices.size();
}

void ClothDemo::onSwapChainDispose() {
    dispose(pipelines.wireframe);
}

void ClothDemo::onSwapChainRecreation() {
    createPipelines();
}

VkCommandBuffer *ClothDemo::buildCommandBuffers(uint32_t imageIndex, uint32_t &numCommandBuffers) {
    numCommandBuffers = 1;
    auto& commandBuffer = commandBuffers[imageIndex];

    auto beginInfo = initializers::commandBufferBeginInfo();
    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    std::array<VkClearValue, 1> clearValues{ {0.0f, 0.0f, 0.0f, 0.0f} };

    auto renderPassInfo = initializers::renderPassBeginInfo();
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = framebuffers[imageIndex];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = swapChain.extent;
    renderPassInfo.clearValueCount = COUNT(clearValues);
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, cloth.vertices, &offset);
    vkCmdBindIndexBuffer(commandBuffer, cloth.indices, 0, VK_INDEX_TYPE_UINT32);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.wireframe);
    cameraController->push(commandBuffer, pipelineLayouts.wireframe);


    vkCmdDrawIndexed(commandBuffer, cloth.indexCount, 1, 0, 0, 0);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.point);
    cameraController->push(commandBuffer, pipelineLayouts.point);

    glm::vec4 pointColor{1, 0, 0, 1};
    vkCmdPushConstants(commandBuffer, pipelineLayouts.point, VK_SHADER_STAGE_VERTEX_BIT, sizeof(Camera), sizeof(glm::vec4), &pointColor[0]);
    vkCmdDrawIndexed(commandBuffer, cloth.indexCount, 1, 0, 0, 0);

    vkCmdEndRenderPass(commandBuffer);
    vkEndCommandBuffer(commandBuffer);

    return &commandBuffer;
}

void ClothDemo::update(float time) {
    cameraController->update(time);
}

void ClothDemo::checkAppInputs() {
    cameraController->processInput();
}

void ClothDemo::initCamera() {
    OrbitingCameraSettings settings;
    settings.orbitMinZoom = 0.5f;
    settings.orbitMaxZoom = 10.0f;
    settings.offsetDistance = 1.0;
    settings.modelHeight = cloth.size;
    settings.aspectRatio = float(swapChain.extent.width)/float(swapChain.extent.height);
    cameraController = std::make_unique<OrbitingCameraController>(device, swapChainImageCount, currentImageIndex, dynamic_cast<InputManager&>(*this), settings);
}

void ClothDemo::createPipelines() {
    auto flatVertexModule = ShaderModule{ "../../data/shaders/flat.vert.spv", device};
    auto flatFragmentModule = ShaderModule{ "../../data/shaders/flat.frag.spv", device};

    auto stages = initializers::vertexShaderStages({
        {flatVertexModule, VK_SHADER_STAGE_VERTEX_BIT},
        { flatFragmentModule, VK_SHADER_STAGE_FRAGMENT_BIT}
    });

    auto vertexBindings = Vertex::bindingDisc();
    auto vertexAttributes = Vertex::attributeDisc();
    auto vertexInputState = initializers::vertexInputState( vertexBindings, vertexAttributes);

    auto inputAssemblyState = initializers::inputAssemblyState();
    inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
    inputAssemblyState.primitiveRestartEnable = VK_TRUE;

    auto viewport = initializers::viewport(swapChain.extent);
    auto scissors = initializers::scissor(swapChain.extent);
    auto viewportState = initializers::viewportState(viewport, scissors);

    auto rasterState = initializers::rasterizationState();
    rasterState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterState.cullMode = VK_CULL_MODE_NONE;
    rasterState.polygonMode = VK_POLYGON_MODE_LINE;
    rasterState.lineWidth = 1.0f;

    auto multisampleState = initializers::multisampleState();

    auto depthStencilState = initializers::depthStencilState();

    auto colorBlendAttachments = initializers::colorBlendStateAttachmentStates();
    auto colorBlendState = initializers::colorBlendState(colorBlendAttachments);

    dispose(pipelineLayouts.wireframe);
    pipelineLayouts.wireframe = device.createPipelineLayout({}, {Camera::pushConstant()});

    auto createInfo = initializers::graphicsPipelineCreateInfo();
    createInfo.flags = VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT;
    createInfo.stageCount = COUNT(stages);
    createInfo.pStages = stages.data();
    createInfo.pVertexInputState = &vertexInputState;
    createInfo.pInputAssemblyState = &inputAssemblyState;
    createInfo.pViewportState = &viewportState;
    createInfo.pRasterizationState = &rasterState;
    createInfo.pMultisampleState = &multisampleState;
    createInfo.pDepthStencilState = &depthStencilState;
    createInfo.pColorBlendState = &colorBlendState;
    createInfo.layout = pipelineLayouts.wireframe;
    createInfo.renderPass = renderPass;
    createInfo.subpass = 0;

    pipelines.wireframe = device.createGraphicsPipeline(createInfo);


    auto pointVertexShaderModule = ShaderModule{ "../../data/shaders/point.vert.spv", device };
    auto pointFragShaderModule = ShaderModule{ "../../data/shaders/point.frag.spv", device };
    stages = initializers::vertexShaderStages({
        {pointVertexShaderModule, VK_SHADER_STAGE_VERTEX_BIT},
        {pointFragShaderModule, VK_SHADER_STAGE_FRAGMENT_BIT}
    });

    inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
    inputAssemblyState.primitiveRestartEnable = VK_FALSE;

    std::vector<VkPushConstantRange> constants(1);
    constants[0].offset = 0;
    constants[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    constants[0].size = sizeof(Camera) + sizeof(glm::vec4);

    pipelineLayouts.point = device.createPipelineLayout({}, constants);

    createInfo.flags = VK_PIPELINE_CREATE_DERIVATIVE_BIT;
    createInfo.stageCount = COUNT(stages);
    createInfo.pStages = stages.data();
    createInfo.pInputAssemblyState = &inputAssemblyState;
    createInfo.layout = pipelineLayouts.point;
    createInfo.basePipelineIndex = -1;
    createInfo.basePipelineHandle = pipelines.wireframe;

    pipelines.point = device.createGraphicsPipeline(createInfo);
}

int main(){
    Settings settings;
    settings.enabledFeatures.fillModeNonSolid = true;
    try{
        auto app = ClothDemo{ settings };
        app.run();
    }catch(const std::runtime_error& err){
        spdlog::error(err.what());
    }
}