#include "CollisionDetection.hpp"
#include "GraphicsPipelineBuilder.hpp"

CollisionDetection::CollisionDetection(const Settings& settings) : VulkanBaseApp("Collision Detection", settings) {

}

void CollisionDetection::initApp() {
    objects.init(device);
    initCamera();
    createDescriptorPool();
    createCommandPool();
    createPipelineCache();
    createRenderPipeline();
    createComputePipeline();
    createGridBuffers();
    createGridDescriptorSetLayout();
    createGridDescriptorSet();
    createGridPipeline();
}

void CollisionDetection::createGridBuffers(){
    grid.mvp = device.createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, sizeof(Camera), "mvp");

    std::vector<glm::vec3> vertices;
    float n = std::sqrtf(static_cast<float>(grid.numCells));
    for(int i = 0; i < int(n) + 1; i++){
        float y = 2.0f * float(i)/n - 1;
        float x = 2.0f * float(i)/n - 1;
        float z = 2.0f * float(i)/n - 1;
//
//        vertices.emplace_back(-1, y, -1);
//        vertices.emplace_back( 1, y, -1);
//        vertices.emplace_back(-1, y, -1);
//        vertices.emplace_back(-1, y,  1);
//        vertices.emplace_back( 1, y, -1);
//        vertices.emplace_back( 1, y,  1);
//        vertices.emplace_back(-1, y,  1);
//        vertices.emplace_back( 1, y,  1);
//
//        vertices.emplace_back(x, -1, -1);
//        vertices.emplace_back(x,  1, -1);
//        vertices.emplace_back(x, -1, -1);
//        vertices.emplace_back(x, -1,  1);
//        vertices.emplace_back(x,  1, -1);
//        vertices.emplace_back(x,  1,  1);
//        vertices.emplace_back(x, -1,  1);
//        vertices.emplace_back(x,  1,  1);
//
//        vertices.emplace_back(-1, -1, z);
//        vertices.emplace_back( 1, -1, z);
//        vertices.emplace_back(-1, -1, z);
//        vertices.emplace_back(-1,  1, z);
//        vertices.emplace_back( 1, -1, z);
//        vertices.emplace_back( 1,  1, z);
//        vertices.emplace_back(-1,  1, z);
//        vertices.emplace_back( 1,  1, z);

        float h = grid.size * 0.5f;
        vertices.emplace_back( x, -h, -h);
        vertices.emplace_back( x, -h,  h);
        vertices.emplace_back(-h, -h,  z);
        vertices.emplace_back( h, -h,  z);

        vertices.emplace_back( x, -h, -h);
        vertices.emplace_back( x,  h, -h);
        vertices.emplace_back(-h,  y, -h);
        vertices.emplace_back( h,  y, -h);

        vertices.emplace_back( -h, -h,  z);
        vertices.emplace_back( -h,  h,  z);
        vertices.emplace_back( -h,  y, -h);
        vertices.emplace_back( -h,  y,  h);


    }

    grid.vertexBuffer = device.createDeviceLocalBuffer(vertices.data(), sizeof(vertices[0]) * vertices.size(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    grid.vertexCount = vertices.size();
}

void CollisionDetection::createDescriptorPool() {
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

void CollisionDetection::createGridDescriptorSetLayout(){
    std::vector<VkDescriptorSetLayoutBinding> bindings(1);
    bindings[0].binding = 0;
    bindings[0].descriptorCount = 1;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    grid.setLayout = device.createDescriptorSetLayout(bindings);
}

void CollisionDetection::createGridDescriptorSet(){
    grid.descriptorSet = descriptorPool.allocate({ grid.setLayout }).front();
    auto writes = initializers::writeDescriptorSets<1>();


    VkDescriptorBufferInfo mvpInfo{ grid.mvp, 0, VK_WHOLE_SIZE};
    writes[0].dstSet = grid.descriptorSet;
    writes[0].dstBinding = 0;
    writes[0].descriptorCount = 1;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writes[0].pBufferInfo = &mvpInfo;

    device.updateDescriptorSets(writes);
}

void CollisionDetection::createCommandPool() {
    commandPool = device.createCommandPool(*device.queueFamilyIndex.graphics, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    commandBuffers = commandPool.allocateCommandBuffers(swapChainImageCount);
}

void CollisionDetection::createPipelineCache() {
    pipelineCache = device.createPipelineCache();
}


void CollisionDetection::createRenderPipeline() {
    auto vertModule = VulkanShaderModule{ "../../data/shaders/flat.vert.spv", device};
    auto fragModule = VulkanShaderModule{"../../data/shaders/flat.frag.spv", device};

    auto shaderStages = initializers::vertexShaderStages({
             { vertModule, VK_SHADER_STAGE_VERTEX_BIT },
             {fragModule, VK_SHADER_STAGE_FRAGMENT_BIT}
     });

    auto bindings = Vertex::bindingDisc();
    auto attribs = Vertex::attributeDisc();

    auto vertexInputState = initializers::vertexInputState(bindings, attribs);
    auto inputAssemblyState = initializers::inputAssemblyState(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP, true);

    float width = swapChain.width() * 0.5f;
    float height = swapChain.height() * 0.5f;
    auto viewport = initializers::viewport(width, height);
    auto scissor = initializers::scissor(width, height);

    auto viewportState = initializers::viewportState(viewport, scissor);

    auto rasterizationState = initializers::rasterizationState();
    rasterizationState.cullMode = VK_CULL_MODE_BACK_BIT;
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
    colorBlendAttachment[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment[0].srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment[0].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment[0].colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment[0].blendEnable = VK_TRUE;
    auto colorBlendState = initializers::colorBlendState(colorBlendAttachment);

    objects.layout = device.createPipelineLayout({},{ Camera::pushConstant()});

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
    createInfo.layout = objects.layout;
    createInfo.renderPass = renderPass;
    createInfo.subpass = 0;

    objects.render.perspectivePipeline = device.createGraphicsPipeline(createInfo, pipelineCache);
    createInfo.flags = VK_PIPELINE_CREATE_DERIVATIVE_BIT;
    createInfo.basePipelineIndex = -1;
    createInfo.basePipelineHandle = objects.render.perspectivePipeline;

    // top
     viewport = initializers::viewport(width, height, width, 0.0f);
     scissor = initializers::scissor(width, height, width, 0.0f);
     viewportState = initializers::viewportState(viewport, scissor);
     createInfo.pViewportState = &viewportState;
     objects.render.topPipeline = device.createGraphicsPipeline(createInfo, pipelineCache);

    // right
     viewport = initializers::viewport(width, height, width, height);
    scissor = initializers::scissor(width, height, width, height);

     viewportState = initializers::viewportState(viewport, scissor);
     createInfo.pViewportState = &viewportState;
     objects.render.rightPipeline = device.createGraphicsPipeline(createInfo, pipelineCache);

    // left
     viewport = initializers::viewport(width, height, 0.0f, height);
     scissor = initializers::scissor(width, height, 0.0f, height);
     viewportState = initializers::viewportState(viewport, scissor);
     createInfo.pViewportState = &viewportState;
     objects.render.leftPipeline = device.createGraphicsPipeline(createInfo, pipelineCache);

    // crossHair perspective
    inputAssemblyState = initializers::inputAssemblyState(VK_PRIMITIVE_TOPOLOGY_LINE_LIST, VK_FALSE);
    rasterizationState.lineWidth = 5.0f;

    viewport = initializers::viewport(width, height);
    scissor = initializers::scissor(width, height);
    viewportState = initializers::viewportState(viewport, scissor);
    createInfo.pViewportState = &viewportState;

    createInfo.pInputAssemblyState = &inputAssemblyState;
    createInfo.pRasterizationState = &rasterizationState;

    objects.crossHair.perspectivePipeline = device.createGraphicsPipeline(createInfo, pipelineCache);

    // top
    viewport = initializers::viewport(width, height, width, 0.0f);
    scissor = initializers::scissor(width, height, width, 0.0f);
    viewportState = initializers::viewportState(viewport, scissor);
    createInfo.pViewportState = &viewportState;
    objects.crossHair.topPipeline = device.createGraphicsPipeline(createInfo, pipelineCache);

    // right
    viewport = initializers::viewport(width, height, width, height);
    scissor = initializers::scissor(width, height, width, height);
    viewportState = initializers::viewportState(viewport, scissor);
    createInfo.pViewportState = &viewportState;
    objects.crossHair.rightPipeline = device.createGraphicsPipeline(createInfo, pipelineCache);

    // left
    viewport = initializers::viewport(width, height, 0.0f, height);
    scissor = initializers::scissor(width, height, 0.0f, height);
    viewportState = initializers::viewportState(viewport, scissor);
    createInfo.pViewportState = &viewportState;
    objects.crossHair.leftPipeline = device.createGraphicsPipeline(createInfo, pipelineCache);
}

void CollisionDetection::createGridPipeline() {
    auto vertexModule = VulkanShaderModule{"../../data/shaders/grid.vert.spv", device};
    auto fragModule = VulkanShaderModule{"../../data/shaders/grid.frag.spv", device};

    auto shaderStages = initializers::vertexShaderStages({
         { vertexModule, VK_SHADER_STAGE_VERTEX_BIT },
         {fragModule, VK_SHADER_STAGE_FRAGMENT_BIT}}
         );

    glm::vec3 gridColor = {160, 190, 224};
    gridColor /= 255.0f;
    std::array<VkSpecializationMapEntry, 3> entries{};
    entries[0].constantID = 0;
    entries[0].offset = 0;
    entries[0].size = 4;

    entries[1].constantID = 1;
    entries[1].offset = 4;
    entries[1].size = 4;

    entries[2].constantID = 2;
    entries[2].offset = 8;
    entries[2].size = 4;

    VkSpecializationInfo specializationInfo{COUNT(entries), entries.data(), sizeof(gridColor), &gridColor};
    shaderStages[1].pSpecializationInfo = &specializationInfo;

    auto bindingDesc = std::vector<VkVertexInputBindingDescription>{{0, sizeof(glm::vec3),  VK_VERTEX_INPUT_RATE_VERTEX}};
    auto attribs = std::vector<VkVertexInputAttributeDescription>{ {0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0} };
    auto vertexInputState = initializers::vertexInputState(bindingDesc, attribs);

    auto inputAssemblyState = initializers::inputAssemblyState(VK_PRIMITIVE_TOPOLOGY_LINE_LIST);

    float width = swapChain.width() * 0.5f;
    float height = swapChain.height() * 0.5f;
    auto viewport = initializers::viewport(width, height);
    auto scissor = initializers::scissor(width, height);

    auto viewportState = initializers::viewportState(viewport, scissor);

    auto rasterState = initializers::rasterizationState();
    rasterState.polygonMode = VK_POLYGON_MODE_FILL;
    rasterState.cullMode = VK_CULL_MODE_NONE;
    rasterState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterState.lineWidth = 2.0f;

    auto multiSampleState = initializers::multisampleState();

    auto depthStencilState = initializers::depthStencilState();
    depthStencilState.depthTestEnable = VK_TRUE;
    depthStencilState.depthWriteEnable = VK_TRUE;
    depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencilState.minDepthBounds = 0.0;
    depthStencilState.maxDepthBounds = 1.0;

    auto colorBlendAttachment = initializers::colorBlendStateAttachmentStates();
    auto colorBlendState = initializers::colorBlendState(colorBlendAttachment);

    grid.layout = device.createPipelineLayout({}, { Camera::pushConstant() });

    auto createInfo = initializers::graphicsPipelineCreateInfo();
    createInfo.flags = VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT;
    createInfo.stageCount = COUNT(shaderStages);
    createInfo.pStages = shaderStages.data();
    createInfo.pVertexInputState = &vertexInputState;
    createInfo.pInputAssemblyState = &inputAssemblyState;
    createInfo.pViewportState = &viewportState;
    createInfo.pRasterizationState = &rasterState;
    createInfo.pMultisampleState = &multiSampleState;
    createInfo.pDepthStencilState = &depthStencilState;
    createInfo.pColorBlendState = &colorBlendState;
    createInfo.layout = grid.layout;
    createInfo.renderPass = renderPass;
    createInfo.subpass = 0;

    grid.perspectivePipeline = device.createGraphicsPipeline(createInfo);

    createInfo.flags = VK_PIPELINE_CREATE_DERIVATIVE_BIT;
    createInfo.basePipelineHandle = grid.perspectivePipeline;
    createInfo.basePipelineIndex = -1;

    // top
    viewport = initializers::viewport(width, height, width, 0.0f);
    scissor = initializers::scissor(width, height, width, 0.0f);
    viewportState = initializers::viewportState(viewport, scissor);
    createInfo.pViewportState = &viewportState;
    grid.topPipeline = device.createGraphicsPipeline(createInfo, pipelineCache);

    // right
    viewport = initializers::viewport(width, height, width, height);
    scissor = initializers::scissor(width, height, width, height);
    viewportState = initializers::viewportState(viewport, scissor);
    createInfo.pViewportState = &viewportState;
    grid.rightPipeline = device.createGraphicsPipeline(createInfo, pipelineCache);

    // left
    viewport = initializers::viewport(width, height, 0.0f, height);
    scissor = initializers::scissor(width, height, 0.0f, height);
    viewportState = initializers::viewportState(viewport, scissor);
    createInfo.pViewportState = &viewportState;
    grid.leftPipeline = device.createGraphicsPipeline(createInfo, pipelineCache);
}

void CollisionDetection::createComputePipeline() {
    auto module = VulkanShaderModule{ "../../data/shaders/pass_through.comp.spv", device};
    auto stage = initializers::shaderStage({ module, VK_SHADER_STAGE_COMPUTE_BIT});

    compute.layout = device.createPipelineLayout();

    auto computeCreateInfo = initializers::computePipelineCreateInfo();
    computeCreateInfo.stage = stage;
    computeCreateInfo.layout = compute.layout;

    compute.pipeline = device.createComputePipeline(computeCreateInfo, pipelineCache);
}


void CollisionDetection::onSwapChainDispose() {
    dispose(objects.render.perspectivePipeline);
    dispose(compute.pipeline);
}

void CollisionDetection::onSwapChainRecreation() {
    createRenderPipeline();
    createComputePipeline();
}

VkCommandBuffer *CollisionDetection::buildCommandBuffers(uint32_t imageIndex, uint32_t &numCommandBuffers) {
    numCommandBuffers = 1;
    auto& commandBuffer = commandBuffers[imageIndex];

    VkCommandBufferBeginInfo beginInfo = initializers::commandBufferBeginInfo();
    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    static std::array<VkClearValue, 2> clearValues;
    clearValues[0].color = {1, 1, 1, 1};
    clearValues[1].depthStencil = {1.0, 0u};

    VkRenderPassBeginInfo rPassInfo = initializers::renderPassBeginInfo();
    rPassInfo.clearValueCount = COUNT(clearValues);
    rPassInfo.pClearValues = clearValues.data();
    rPassInfo.framebuffer = framebuffers[imageIndex];
    rPassInfo.renderArea.offset = {0u, 0u};
    rPassInfo.renderArea.extent = swapChain.extent;
    rPassInfo.renderPass = renderPass;

    vkCmdUpdateBuffer(commandBuffer, grid.mvp, 0, sizeof(Camera), &camera->cam());
    vkCmdBeginRenderPass(commandBuffer, &rPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkDeviceSize offset = 0;
//    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, grid.perspectivePipeline);
//    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, grid.layout, 0, 1, &grid.descriptorSet, 0, 0);
//    // grid
//    vkCmdBindVertexBuffers(commandBuffer, 0, 1, grid.vertexBuffer, &offset);
//    vkCmdDraw(commandBuffer, grid.vertexCount, 1, 0, 0);
    drawGrid(commandBuffer, grid.perspectivePipeline, grid.layout, camera->cam());
    drawGrid(commandBuffer, grid.topPipeline, grid.layout, cameras.top);
    drawGrid(commandBuffer, grid.rightPipeline, grid.layout, cameras.right);
    drawGrid(commandBuffer, grid.leftPipeline, grid.layout, cameras.front);

    // objects
//    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, objects.crossHair.perspectivePipeline);
//    camera->push(commandBuffer, objects.layout);
//    for(auto& object : objects.objects){
//        vkCmdBindVertexBuffers(commandBuffer, 0, 1, object.crossHairBuffer, &offset);
//        vkCmdDraw(commandBuffer, object.crossHairBuffer.size/sizeof(Vertex), 1, 0, 0);
//    }
//
//    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, objects.render.perspectivePipeline);
//    camera->push(commandBuffer, objects.layout);
//    for(auto& object : objects.objects){
//        vkCmdBindVertexBuffers(commandBuffer, 0, 1, object.vertexBuffer, &offset);
//        vkCmdBindIndexBuffer(commandBuffer, object.indexBuffers, 0, VK_INDEX_TYPE_UINT32);
//        vkCmdDrawIndexed(commandBuffer, object.indexBuffers.size/sizeof(uint32_t), 1, 0, 0, 0);
//    }
    drawObject(commandBuffer, objects.render.perspectivePipeline, objects.crossHair.perspectivePipeline, objects.layout, camera->cam());
    drawObject(commandBuffer, objects.render.topPipeline, objects.crossHair.topPipeline, objects.layout, cameras.top);
    drawObject(commandBuffer, objects.render.rightPipeline, objects.crossHair.rightPipeline, objects.layout, cameras.right);
    drawObject(commandBuffer, objects.render.leftPipeline, objects.crossHair.leftPipeline, objects.layout, cameras.front);

    vkCmdEndRenderPass(commandBuffer);

    vkEndCommandBuffer(commandBuffer);

    return &commandBuffer;
}

void CollisionDetection::drawGrid(VkCommandBuffer commandBuffer, VkPipeline pipeline, VkPipelineLayout layout, const Camera& camera) {

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Camera), &camera);
    // grid
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, grid.vertexBuffer, &offset);
    vkCmdDraw(commandBuffer, grid.vertexCount, 1, 0, 0);
}

void CollisionDetection::drawObject(VkCommandBuffer commandBuffer, VkPipeline objectPipeline, VkPipeline crossHairPipeline, VkPipelineLayout layout,
                                    const Camera &camera) {

    VkDeviceSize offset = 0;
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, crossHairPipeline);
    vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Camera), &camera);
    for(auto& object : objects.objects){
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, object.crossHairBuffer, &offset);
        vkCmdDraw(commandBuffer, object.crossHairBuffer.size/sizeof(Vertex), 1, 0, 0);
    }

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, objectPipeline);
    vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Camera), &camera);
    for(auto& object : objects.objects){
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, object.vertexBuffer, &offset);
        vkCmdBindIndexBuffer(commandBuffer, object.indexBuffers, 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(commandBuffer, object.indexBuffers.size/sizeof(uint32_t), 1, 0, 0, 0);
    }

}

void CollisionDetection::update(float time) {
    camera->update(time);
}

void CollisionDetection::checkAppInputs() {
    camera->processInput();
}

void CollisionDetection::cleanup() {
    // TODO save pipeline cache
    VulkanBaseApp::cleanup();
}

void CollisionDetection::onPause() {
    VulkanBaseApp::onPause();
}

void CollisionDetection::initCamera() {
    OrbitingCameraSettings settings{};
    settings.offsetDistance = 5.0f;
    settings.rotationSpeed = 0.1f;
    settings.orbitMinZoom = -5.0f;
    settings.orbitMaxZoom = 100.0f;
    settings.fieldOfView = 45.0f;
    settings.modelHeight = 1.0f;
    settings.aspectRatio = swapChain.aspectRatio();
    camera = std::make_unique<OrbitingCameraController>(dynamic_cast<InputManager&>(*this), settings);

    cameras.top.proj = vkn::ortho(-1.5f, 1.5f, -1.5f, 1.5f, -1.5f, 1.5f);
    cameras.top.model = glm::rotate(glm::mat4(1), -glm::half_pi<float>(), {1, 0, 0});

    cameras.right.proj = vkn::ortho(-1.5f, 1.5f, -1.5f, 1.5f, -1.5f, 1.5f);
    cameras.right.model = glm::rotate(glm::mat4(1), glm::half_pi<float>(), {0, 1, 0});

    cameras.front.proj = vkn::ortho(-1.5f, 1.5f, -1.5f, 1.5f, -1.5f, 1.5f);

}


int main(){
    try{

        Settings settings;
        settings.width = 1280;
        settings.height = 1280;
        settings.depthTest = true;
        settings.enabledFeatures.wideLines = true;

        auto app = CollisionDetection{ settings };
        app.run();
    }catch(std::runtime_error& err){
        spdlog::error(err.what());
    }
}