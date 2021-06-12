#include "MarchingCubeDemo.hpp"
#include "MarchingCube.hpp"
#include "ImGuiPlugin.hpp"

MarchingCubeDemo::MarchingCubeDemo(Settings settings) : VulkanBaseApp("Marching Cubes", settings) {

}


void MarchingCubeDemo::initApp() {
    nextConfig = &mapToKey(Key::SPACE_BAR, "next_config", Action::detectInitialPressOnly());
    commandPool = device.createCommandPool(*device.queueFamilyIndex.graphics, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    commandBuffers = commandPool.allocate(swapChainImageCount);
    initCamera();
    initVertexBuffer();
    createCellBuffer();
    createPipeline();
}

void MarchingCubeDemo::onSwapChainDispose() {
    VulkanBaseApp::onSwapChainDispose();
}

void MarchingCubeDemo::onSwapChainRecreation() {
    VulkanBaseApp::onSwapChainRecreation();
}

VkCommandBuffer *MarchingCubeDemo::buildCommandBuffers(uint32_t imageIndex, uint32_t &numCommandBuffers) {
    
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

    VkDeviceSize  offset = 0;
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, cellBuffer, &offset);
    vkCmdBindIndexBuffer(commandBuffer, cellIndices, 0, VK_INDEX_TYPE_UINT32);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.lines);
    camera->push(commandBuffer, pipelineLayout.lines);
    vkCmdDrawIndexed(commandBuffer, cellIndices.size/sizeof(uint32_t), 1, 0, 0, 0);


    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.points);
    camera->push(commandBuffer, pipelineLayout.points);
    vkCmdPushConstants(commandBuffer, pipelineLayout.points, VK_SHADER_STAGE_VERTEX_BIT, sizeof(Camera), sizeof(uint32_t), &config);
    vkCmdDraw(commandBuffer, cellBuffer.size/sizeof(glm::vec3), 1, 0, 0);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.triangles);
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffer, &offset);
    camera->push(commandBuffer, pipelineLayout.triangles, VK_SHADER_STAGE_VERTEX_BIT);
    vkCmdDrawIndirect(commandBuffer, drawCommandBuffer, 0, 1, sizeof(VkDrawIndirectCommand));

    renderText(commandBuffer);

    vkCmdEndRenderPass(commandBuffer);

    vkEndCommandBuffer(commandBuffer);

    return &commandBuffer;
}

void MarchingCubeDemo::update(float time) {
    camera->update(time);
}

void MarchingCubeDemo::checkAppInputs() {
    if(nextConfig->isPressed()){
        config += 1;
        config %= 256;
        generateTriangles();
        spdlog::info("config: {}", config);
    }
    camera->processInput();
}

void MarchingCubeDemo::cleanup() {
    VulkanBaseApp::cleanup();
}

void MarchingCubeDemo::initVertexBuffer() {
    vertexBuffer = device.createBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, sizeof(mVertex) * 16, "vertices");
    drawCommandBuffer = device.createBuffer(VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, sizeof(VkDrawIndirectCommand), "draw_command_buffer");

    drawCommandBuffer.map<VkDrawIndirectCommand>([&](auto drawCmd){
        drawCmd->vertexCount = 0;
        drawCmd->instanceCount = 0;
        drawCmd->firstInstance = 0;
        drawCmd->firstVertex = 0;
    });
}

void MarchingCubeDemo::createCellBuffer() {
    vertices = std::vector<glm::vec3>{
            {-0.5, -0.5, -0.5},
            {0.5, -0.5, -0.5},
            {0.5, -0.5, 0.5},
            {-0.5, -0.5, 0.5},
            {-0.5, 0.5, -0.5},
            {0.5, 0.5, -0.5},
            {0.5, 0.5, 0.5},
            {-0.5, 0.5, 0.5},
    };

    indices = std::vector<uint32_t>{
        0, 1, 1, 2, 2, 3, 3, 0,
        4, 5, 5, 6, 6, 7, 7, 4,
        4, 0, 5, 1, 6, 2, 7, 3
    };

    cellBuffer = device.createDeviceLocalBuffer(vertices.data(), sizeof(glm::vec3) * vertices.size(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    cellIndices = device.createDeviceLocalBuffer(indices.data(), sizeof(uint32_t) * indices.size(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
}

void MarchingCubeDemo::createPipeline() {
    auto cellVertModule = ShaderModule{ "../../data/shaders/marching_cubes/cell.vert.spv", device};
    auto cellFragModule = ShaderModule{"../../data/shaders/marching_cubes/cell.frag.spv", device};

    auto shaderStages = initializers::vertexShaderStages({
        { cellVertModule, VK_SHADER_STAGE_VERTEX_BIT },
        {cellFragModule, VK_SHADER_STAGE_FRAGMENT_BIT}
    });

    std::array<VkVertexInputBindingDescription, 1> bindings{
            {0, sizeof(glm::vec3), VK_VERTEX_INPUT_RATE_VERTEX}
    };
    std::array<VkVertexInputAttributeDescription , 1> attribs {
            {0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0}
    };

    auto vertexInputState = initializers::vertexInputState(bindings, attribs);
    auto inputAssemblyState = initializers::inputAssemblyState(VK_PRIMITIVE_TOPOLOGY_LINE_LIST);

    auto viewport = initializers::viewport(swapChain.extent);
    auto scissor = initializers::scissor(swapChain.extent);

    auto viewportState = initializers::viewportState(viewport, scissor);

    auto rasterizationState = initializers::rasterizationState();
    rasterizationState.cullMode = VK_CULL_MODE_NONE;
    rasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizationState.lineWidth = 2.0;

    auto multisampleState = initializers::multisampleState();
    auto depthStencilState = initializers::depthStencilState();
    depthStencilState.depthTestEnable = VK_TRUE;
    depthStencilState.depthWriteEnable = VK_TRUE;
    depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencilState.minDepthBounds = 0.0;
    depthStencilState.maxDepthBounds = 1.0;

    auto colorBlendAttachment = initializers::colorBlendStateAttachmentStates();
    auto colorBlendState = initializers::colorBlendState(colorBlendAttachment);

    pipelineLayout.lines = device.createPipelineLayout({}, {Camera::pushConstant()});

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
    createInfo.layout = pipelineLayout.lines;
    createInfo.renderPass = renderPass;
    createInfo.subpass = 0;

    pipelines.lines = device.createGraphicsPipeline(createInfo);

    auto pointVertModule = ShaderModule{ "../../data/shaders/marching_cubes/point.vert.spv", device};
    auto pointfragtModule = ShaderModule{ "../../data/shaders/marching_cubes/point.frag.spv", device};

    auto pointStages = initializers::vertexShaderStages(
            {
                {pointVertModule, VK_SHADER_STAGE_VERTEX_BIT},
                {pointfragtModule, VK_SHADER_STAGE_FRAGMENT_BIT}
            });

    inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;

    pipelineLayout.points = device.createPipelineLayout({}, { {VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(uint32_t) + sizeof(Camera)}});

    createInfo.flags = VK_PIPELINE_CREATE_DERIVATIVE_BIT;
    createInfo.stageCount = COUNT(pointStages);
    createInfo.pStages = pointStages.data();
    createInfo.pInputAssemblyState = &inputAssemblyState;
    createInfo.layout = pipelineLayout.points;
    createInfo.basePipelineIndex = -1;
    createInfo.basePipelineHandle = pipelines.lines;

    pipelines.points = device.createGraphicsPipeline(createInfo);

    auto triVertModule = ShaderModule{ "../../data/shaders/marching_cubes/triangle.vert.spv", device};
    auto triFragModule = ShaderModule{"../../data/shaders/marching_cubes/triangle.frag.spv", device};

    auto triStages = initializers::vertexShaderStages({
                         { triVertModule, VK_SHADER_STAGE_VERTEX_BIT },
                         {triFragModule, VK_SHADER_STAGE_FRAGMENT_BIT}
                 });

    std::array<VkVertexInputBindingDescription, 1> triBindings{
            {0, sizeof(mVertex), VK_VERTEX_INPUT_RATE_VERTEX}
    };
    std::array<VkVertexInputAttributeDescription , 2> triAttribs {{
           {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetOf(mVertex, position)},
           {1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetOf(mVertex, normal)}
    }};

    vertexInputState = initializers::vertexInputState(triBindings, triAttribs);

    inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    pipelineLayout.triangles = device.createPipelineLayout({}, {Camera::pushConstant()});

    createInfo.stageCount = COUNT(triStages);
    createInfo.pStages = triStages.data();
    createInfo.pVertexInputState = &vertexInputState;
    createInfo.pInputAssemblyState = &inputAssemblyState;
    createInfo.layout = pipelineLayout.triangles;
    createInfo.basePipelineIndex = -1;
    createInfo.basePipelineHandle = pipelines.lines;

    pipelines.triangles = device.createGraphicsPipeline(createInfo);
}

void MarchingCubeDemo::initCamera() {
    OrbitingCameraSettings settings;
    settings.modelHeight = 1.0;
    settings.offsetDistance = 1.5;
    settings.aspectRatio = swapChain.aspectRatio();
    camera = std::make_unique<OrbitingCameraController>(device, swapChainImageCount, currentImageIndex, dynamic_cast<InputManager&>(*this), settings);
}

void MarchingCubeDemo::generateTriangles() {
    static constexpr auto edgeTable = marching_cube::edgeTable();
    static constexpr auto triTable = marching_cube::triTable();

    if(edgeTable[config] == 0) {
        drawCommandBuffer.map<VkDrawIndirectCommand>([&](auto drawCmd){
            drawCmd->vertexCount = 0;
            drawCmd->instanceCount = 0;
            drawCmd->firstInstance = 0;
            drawCmd->firstVertex = 0;
        });
        return;
    }

    constexpr auto numEdges = 12u;
    std::array<glm::vec3, numEdges> vertexList{};

    auto getVertex = [&](uint32_t edge){
        auto p0 = vertices[indices[edge * 2]];
        auto p1 = vertices[indices[edge * 2 + 1]];

        return (p0 + p1) * 0.5f;
    };

    for(auto edge = 0; edge < numEdges; edge++){
        if(edgeTable[config] &  (1 << edge)){
            vertexList[edge] = getVertex(edge);
        }
    }
    std::vector<mVertex> triVertices{};
    std::vector<glm::vec3> normals;
    for(auto i = config * 16; triTable[i] != -1; i+= 3){
        glm::vec3 p0 = vertexList[triTable[i]];
        glm::vec3 p1 = vertexList[triTable[i + 1]];
        glm::vec3 p2 = vertexList[triTable[i + 2]];

        glm::vec3 a = p1 - p0;
        glm::vec3 b = p2 - p0;
        auto normal = glm::normalize(glm::cross(a, b));
        triVertices.push_back({p0, normal});
        triVertices.push_back({p1, normal});
        triVertices.push_back({p2, normal});
    }

    vertexBuffer.map<mVertex>([&](auto ptr){
       for(int i = 0; i < triVertices.size(); i++){
           ptr[i] = triVertices[i];
       }
    });

    drawCommandBuffer.map<VkDrawIndirectCommand>([&](auto drawCmd){
       drawCmd->vertexCount = triVertices.size();
       drawCmd->instanceCount = 1;
       drawCmd->firstInstance = 0;
       drawCmd->firstVertex = 0;
    });
}

void MarchingCubeDemo::renderText(VkCommandBuffer commandBuffer) {
    auto& imgui = plugin<ImGuiPlugin>(IM_GUI_PLUGIN);
    ImGui::Begin("Menu", nullptr, IMGUI_NO_WINDOW);
    ImGui::SetWindowSize({ 500, float(height)});
    ImGui::TextColored({1, 1, 0, 1}, "configuration: %d", config);
    ImGui::End();
    imgui.draw(commandBuffer);
}


int main(){
    Settings settings;
    settings.enabledFeatures.wideLines = VK_TRUE;
    settings.depthTest = true;

    std::unique_ptr<Plugin> imGui = std::make_unique<ImGuiPlugin>();

    try{
        auto scene = MarchingCubeDemo{ settings };
        scene.addPlugin(imGui);
        scene.run();
    }catch (std::runtime_error& err){
        spdlog::error(err.what());
    }
    return 0;
}