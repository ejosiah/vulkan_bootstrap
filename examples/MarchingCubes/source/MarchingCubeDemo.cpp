#include "MarchingCubeDemo.hpp"
#include "MarchingCube.hpp"
#include "ImGuiPlugin.hpp"

MarchingCubeDemo::MarchingCubeDemo(Settings settings) : VulkanBaseApp("Marching Cubes", settings) {

}


void MarchingCubeDemo::initApp() {
    nextConfig = &mapToKey(Key::SPACE_BAR, "next_config", Action::detectInitialPressOnly());
    commandPool = device.createCommandPool(*device.queueFamilyIndex.graphics, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    commandBuffers = commandPool.allocateCommandBuffers(swapChainImageCount);
    initCamera();
    initSdf();
    initVertexBuffer();
    createCellBuffer();
    createDescriptorPool();
    createDescriptorSetLayout();
    createDescriptorSet();
    createPipeline();

    initMarchingCubeBuffers();
    createMarchingCubeDescriptorSetLayout();
    createMarchingCubeDescriptorSet();
    createMarchingCubePipeline();
    createSdf();
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
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout.triangles, 0, 1, &sdfDescriptorSet, 0, nullptr);
    camera->push(commandBuffer, pipelineLayout.triangles, VK_SHADER_STAGE_VERTEX_BIT);

//    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffer, &offset);
//    vkCmdDrawIndirect(commandBuffer, drawCommandBuffer, 0, 1, sizeof(VkDrawIndirectCommand));
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, marchingCube.vertexBuffer, &offset);
    vkCmdBindIndexBuffer(commandBuffer, marchingCube.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
//    vkCmdDraw(commandBuffer, marchingCube.numVertices, 1, 0, 0);
    vkCmdDrawIndexed(commandBuffer, marchingCube.indexBuffer.size/sizeof(uint32_t), 1, 0, 0, 0);

    renderText(commandBuffer);

    vkCmdEndRenderPass(commandBuffer);

    vkEndCommandBuffer(commandBuffer);

    return &commandBuffer;
}

void MarchingCubeDemo::update(float time) {
    camera->update(time);
    marchingCube.constants.time = elapsedTime;
//    createSdf();
}

void MarchingCubeDemo::checkAppInputs() {
    if(nextConfig->isPressed()){
        config += 1;
        config %= 256;
     //   generateTriangles();
        createSdf();
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
    auto cellVertModule = VulkanShaderModule{"../../data/shaders/marching_cubes/cell.vert.spv", device};
    auto cellFragModule = VulkanShaderModule{"../../data/shaders/marching_cubes/cell.frag.spv", device};

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

    auto pointVertModule = VulkanShaderModule{"../../data/shaders/marching_cubes/point.vert.spv", device};
    auto pointfragtModule = VulkanShaderModule{"../../data/shaders/marching_cubes/point.frag.spv", device};

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

    auto triVertModule = VulkanShaderModule{"../../data/shaders/marching_cubes/triangle.vert.spv", device};
    auto triFragModule = VulkanShaderModule{"../../data/shaders/marching_cubes/triangle.frag.spv", device};

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
    pipelineLayout.triangles = device.createPipelineLayout({sdfDescriptorSetLayout }, {Camera::pushConstant()});
//    rasterizationState.polygonMode = VK_POLYGON_MODE_LINE;
    createInfo.stageCount = COUNT(triStages);
    createInfo.pStages = triStages.data();
    createInfo.pVertexInputState = &vertexInputState;
    createInfo.pInputAssemblyState = &inputAssemblyState;
    createInfo.pRasterizationState = &rasterizationState;
    createInfo.layout = pipelineLayout.triangles;
    createInfo.basePipelineIndex = -1;
    createInfo.basePipelineHandle = pipelines.lines;

    pipelines.triangles = device.createGraphicsPipeline(createInfo);

    auto sdfComputeModule = VulkanShaderModule{"../../data/shaders/marching_cubes/sdf.comp.spv", device};
    auto stage = initializers::shaderStage({ sdfComputeModule, VK_SHADER_STAGE_COMPUTE_BIT});

    pipelineLayout.sdf = device.createPipelineLayout({computeDescriptorSetLayout});

    auto computeCreateInfo = initializers::computePipelineCreateInfo();
    computeCreateInfo.stage = stage;
    computeCreateInfo.layout = pipelineLayout.sdf;

    pipelines.sdf = device.createComputePipeline(computeCreateInfo);
}

void MarchingCubeDemo::initCamera() {
    OrbitingCameraSettings settings;
    settings.modelHeight = 1.0;
    settings.offsetDistance = 1.5;
    settings.orbitMinZoom = 0.001;
    settings.orbitMaxZoom = 10;
    settings.aspectRatio = swapChain.aspectRatio();
    camera = std::make_unique<OrbitingCameraController>(dynamic_cast<InputManager&>(*this), settings);
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
        triVertices.push_back({glm::vec4(p0, 0), glm::vec4(normal, 0)});
        triVertices.push_back({glm::vec4(p1, 0), glm::vec4(normal, 0)});
        triVertices.push_back({glm::vec4(p2, 0), glm::vec4(normal, 0)});
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
    ImGui::TextColored({1, 1, 0, 1}, "FPS: %u", framePerSecond);
    ImGui::End();
    imgui.draw(commandBuffer);
}

void MarchingCubeDemo::initSdf() {
    VkImageCreateInfo imageCreateInfo = initializers::imageCreateInfo(VK_IMAGE_TYPE_3D, VK_FORMAT_R32G32B32A32_SFLOAT
                                                                      , VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, sdfSize, sdfSize, sdfSize);
    sdf.image = device.createImage(imageCreateInfo);

    VkImageSubresourceRange subresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
    sdf.image.transitionLayout(device.graphicsCommandPool(), VK_IMAGE_LAYOUT_GENERAL, subresourceRange, 0,
                               VK_ACCESS_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
    sdf.imageView = sdf.image.createView(VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_VIEW_TYPE_3D, subresourceRange);

    normalMap.image = device.createImage(imageCreateInfo);
    normalMap.image.transitionLayout(device.graphicsCommandPool(), VK_IMAGE_LAYOUT_GENERAL, subresourceRange, 0,
                               VK_ACCESS_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

    normalMap.imageView = normalMap.image.createView(VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_VIEW_TYPE_3D, subresourceRange);


    VkSamplerCreateInfo samplerCreateInfo = initializers::samplerCreateInfo();
    samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerCreateInfo.magFilter = VK_FILTER_NEAREST;
    samplerCreateInfo.minFilter = VK_FILTER_NEAREST;
    samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;


    sdf.sampler = device.createSampler(samplerCreateInfo);
    normalMap.sampler = device.createSampler(samplerCreateInfo);
}

void MarchingCubeDemo::createDescriptorPool() {
    std::array<VkDescriptorPoolSize, 3> poolSizes{
            {
                    {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 100},
                    {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 100},
                    {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 100},
            }
    };

    descriptorPool = device.createDescriptorPool(3, poolSizes, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT);
}

void MarchingCubeDemo::createDescriptorSetLayout() {
    std::vector<VkDescriptorSetLayoutBinding> bindings(2);
    bindings[0].binding = 0;
    bindings[0].descriptorCount = 1;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;

    bindings[1].binding = 1;
    bindings[1].descriptorCount = 1;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
//
//    bindings[2].binding = 2;
//    bindings[2].descriptorCount = 1;
//    bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
//    bindings[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;

    sdfDescriptorSetLayout = device.createDescriptorSetLayout(bindings);

    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    bindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    computeDescriptorSetLayout = device.createDescriptorSetLayout(bindings);
}

void MarchingCubeDemo::createDescriptorSet() {
    auto sets = descriptorPool.allocate({sdfDescriptorSetLayout, computeDescriptorSetLayout});
    sdfDescriptorSet = sets.front();
    computeDescriptorSet = sets.back();
    
   auto writes = initializers::writeDescriptorSets<4>();
   
   VkDescriptorImageInfo imageInfo;
   imageInfo.imageView = sdf.imageView;
   imageInfo.sampler = sdf.sampler;
   imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

    VkDescriptorImageInfo normalInfo;
    normalInfo.imageView = normalMap.imageView;
    normalInfo.sampler = normalMap.sampler;
    normalInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
   
   writes[0].dstSet = sdfDescriptorSet;
   writes[0].dstBinding = 0;
   writes[0].descriptorCount = 1;
   writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
   writes[0].pImageInfo = &imageInfo;

    writes[1].dstSet = sdfDescriptorSet;
    writes[1].dstBinding = 1;
    writes[1].descriptorCount = 1;
    writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[1].pImageInfo = &normalInfo;

    writes[2].dstSet = computeDescriptorSet;
    writes[2].dstBinding = 0;
    writes[2].descriptorCount = 1;
    writes[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    writes[2].pImageInfo = &imageInfo;

    writes[3].dstSet = computeDescriptorSet;
    writes[3].dstBinding = 1;
    writes[3].descriptorCount = 1;
    writes[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    writes[3].pImageInfo = &normalInfo;

   device.updateDescriptorSets(writes);
}

void MarchingCubeDemo::createSdf() {
    device.graphicsCommandPool().oneTimeCommand([&](auto commandBuffer){
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelines.sdf);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout.sdf, 0, 1, &computeDescriptorSet, 0, VK_NULL_HANDLE);
        vkCmdDispatch(commandBuffer, sdfSize/8 , sdfSize/8 , sdfSize/8 );
    });

    auto numVertex = march(0);

    if(numVertex ==  0){
        spdlog::info("No primitives generated");
        return;
    }
    marchingCube.vertexBuffer = device.createBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, sizeof(mVertex) * numVertex);
    updateMarchingCubeVertexDescriptorSet();

    marchingCube.numVertices = march(1);
    spdlog::info("size: {}, {}, {}", numVertex, marchingCube.numVertices, marchingCube.vertexBuffer.size/sizeof(mVertex));

    generateIndex(marchingCube.vertexBuffer, marchingCube.vertexBuffer, marchingCube.indexBuffer);
}

void MarchingCubeDemo::createMarchingCubeDescriptorSetLayout() {
    std::array<VkDescriptorSetLayoutBinding, 4> bindings{};

    bindings[0].binding = 0;
    bindings[0].descriptorCount = 1;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    bindings[1].binding = 1;
    bindings[1].descriptorCount = 1;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    bindings[2].binding = 2;
    bindings[2].descriptorCount = 1;
    bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[2].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    bindings[3].binding = 3;
    bindings[3].descriptorCount = 1;
    bindings[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[3].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    marchingCube.descriptorSetLayout = device.createDescriptorSetLayout(bindings);
}

void MarchingCubeDemo::createMarchingCubeDescriptorSet() {
    marchingCube.descriptorSet = descriptorPool.allocate({ marchingCube.descriptorSetLayout }).front();

     auto writes = initializers::writeDescriptorSets<4>(marchingCube.descriptorSet);

    VkDescriptorBufferInfo counterInfo;
    counterInfo.buffer = marchingCube.atomicCounterBuffers;
    counterInfo.offset = 0;
    counterInfo.range = VK_WHOLE_SIZE;

    writes[0].dstBinding = 0;
    writes[0].descriptorCount = 1;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[0].pBufferInfo = &counterInfo;

    VkDescriptorBufferInfo edgeTableInfo;
    edgeTableInfo.buffer = marchingCube.edgeTableBuffer;
    edgeTableInfo.offset = 0;
    edgeTableInfo.range = VK_WHOLE_SIZE;

    writes[1].dstBinding = 1;
    writes[1].descriptorCount = 1;
    writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[1].pBufferInfo = &edgeTableInfo;

    VkDescriptorBufferInfo triTableInfo;
    triTableInfo.buffer = marchingCube.triTableBuffer;
    triTableInfo.offset = 0;
    triTableInfo.range = VK_WHOLE_SIZE;

    writes[2].dstBinding = 2;
    writes[2].descriptorCount = 1;
    writes[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[2].pBufferInfo = &triTableInfo;

    VkDescriptorBufferInfo vertexInfo;
    vertexInfo.buffer = marchingCube.vertexBuffer;
    vertexInfo.offset = 0;
    vertexInfo.range = VK_WHOLE_SIZE;

    writes[3].dstBinding = 3;
    writes[3].descriptorCount = 1;
    writes[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[3].pBufferInfo = &vertexInfo;

    device.updateDescriptorSets(writes);
}

void MarchingCubeDemo::initMarchingCubeBuffers() {
    constexpr auto edgeTable = marching_cube::edgeTable();
    marchingCube.edgeTableBuffer = device.createDeviceLocalBuffer(edgeTable.data(), sizeof(uint32_t) * edgeTable.size(), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);

    constexpr auto triTable = marching_cube::triTable();
    marchingCube.triTableBuffer = device.createDeviceLocalBuffer(triTable.data(), sizeof(int) * triTable.size(), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);

    marchingCube.atomicCounterBuffers = device.createBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, sizeof(uint32_t) * 2);
    marchingCube.vertexBuffer = device.createBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_ONLY, sizeof(glm::vec3));

}

void MarchingCubeDemo::createMarchingCubePipeline() {
    auto marchingCubeModule = VulkanShaderModule{"../../data/shaders/marching_cubes/march.comp.spv", device};
    auto stage = initializers::shaderStage({ marchingCubeModule, VK_SHADER_STAGE_COMPUTE_BIT});

    marchingCube.layout = device.createPipelineLayout({sdfDescriptorSetLayout, marchingCube.descriptorSetLayout}, { { VK_SHADER_STAGE_COMPUTE_BIT, sizeof(Camera), sizeof(decltype(marchingCube.constants)) } });

    auto computeCreateInfo = initializers::computePipelineCreateInfo();
    computeCreateInfo.stage = stage;
    computeCreateInfo.layout = marchingCube.layout;

    marchingCube.pipeline = device.createComputePipeline(computeCreateInfo);
}

uint32_t MarchingCubeDemo::march(int pass) {
    static std::array<VkDescriptorSet, 2> sets;
    sets[0] = sdfDescriptorSet;
    sets[1] = marchingCube.descriptorSet;

    marchingCube.atomicCounterBuffers.map<uint32_t>([&](auto ptr){ ptr[0] = ptr[1] = 0; });
    device.graphicsCommandPool().oneTimeCommand([&](auto commandBuffer){
        marchingCube.constants.pass = pass;
        marchingCube.constants.frameCount = frameCount;
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, marchingCube.pipeline);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, marchingCube.layout, 0, COUNT(sets), sets.data(), 0, VK_NULL_HANDLE);
        vkCmdPushConstants(commandBuffer, marchingCube.layout, VK_SHADER_STAGE_COMPUTE_BIT, sizeof(Camera), sizeof(marchingCube.constants), &marchingCube.constants);
        vkCmdDispatch(commandBuffer, numGrids, numGrids, numGrids);
    });
    uint32_t numVertex = 0;
    uint32_t vertexWrites = 0;
    marchingCube.atomicCounterBuffers.map<uint32_t>([&](auto ptr){ numVertex = *ptr; *ptr = 0; vertexWrites = ptr[1]; ptr[1] = 0; });
//    spdlog::info("num vertices: {}, vertexWrites: {}", numVertex, vertexWrites);
    return numVertex;
}

void MarchingCubeDemo::generateIndex(VulkanBuffer &source, VulkanBuffer &vBuffer, VulkanBuffer &iBuffer) {
    auto start = chrono::steady_clock::now();
    std::vector<uint32_t> indices;
    std::unordered_map<mVertex, uint32_t> vertexMap;
    std::vector<mVertex> vertices;
    auto prevNumVertices = source.size/sizeof(mVertex);


    int similar = 0;
    source.map<mVertex>([&](auto ptr){
       for(int i = 0; i < source.size/sizeof(mVertex); i++) {
           mVertex vertex = ptr[i];
           if(vertexMap.find(vertex) != end(vertexMap)){
               auto index = vertexMap[vertex];
               indices.push_back(index);
               similar++;
           }else{
               vertexMap[vertex] = vertices.size();
               indices.push_back(vertices.size());
               vertices.push_back(vertex);
           }
       }
    });
    auto end = chrono::steady_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(end - start);

    spdlog::info("previous num vertices: {}, new num vertices: {}, similar vertices {}, diff {}, num indices: {}, duration: {}"
                 , prevNumVertices, vertices.size(),  similar, prevNumVertices - vertices.size(), indices.size(), duration);

    vBuffer = device.createDeviceLocalBuffer(vertices.data(), sizeof(mVertex) * vertices.size(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    iBuffer = device.createDeviceLocalBuffer(indices.data(), sizeof(uint32_t) * indices.size(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
}

void MarchingCubeDemo::updateMarchingCubeVertexDescriptorSet() {
    auto writes = initializers::writeDescriptorSets<1>(marchingCube.descriptorSet);

    VkDescriptorBufferInfo vertexInfo;
    vertexInfo.buffer = marchingCube.vertexBuffer;
    vertexInfo.offset = 0;
    vertexInfo.range = VK_WHOLE_SIZE;

    writes[0].dstBinding = 3;
    writes[0].descriptorCount = 1;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[0].pBufferInfo = &vertexInfo;

    device.updateDescriptorSets(writes);
}

int main(){
    Settings settings;
    settings.enabledFeatures.wideLines = VK_TRUE;
    settings.enabledFeatures.fillModeNonSolid = VK_TRUE;
    settings.depthTest = true;
    settings.queueFlags |= VK_QUEUE_COMPUTE_BIT;

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