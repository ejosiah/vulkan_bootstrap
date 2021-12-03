#include "ClothDemo.hpp"
#include "primitives.h"

ClothDemo::ClothDemo(const Settings &settings) : VulkanRayTraceBaseApp("Cloth", settings) {

}

void ClothDemo::initApp() {
    checkInvariants();
    initQueryPools();
    createDescriptorPool();
    commandPool = device.createCommandPool(*device.queueFamilyIndex.graphics, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    commandBuffers = commandPool.allocateCommandBuffers(swapChainImageCount);
    createCloth();
    createSphere();
    createFloor();
    loadModel();
    initCamera();
    createPositionDescriptorSetLayout();
    createPositionDescriptorSet();
    createPipelines();
    createComputePipeline();

    createRayTraceDescriptorSetLayout();
    createRayTraceDescriptorSet();
    createRayTracePipeline();
    createShaderBindingTables();
}

void ClothDemo::createCloth() {
    auto xform = glm::translate(glm::mat4(1), {0, 60, 0}) *  glm::rotate(glm::mat4(1), -glm::half_pi<float>(), {1, 0, 0});
    auto plane = primitives::plane(cloth.gridSize.x - 1, cloth.gridSize.y - 1, cloth.size.x, cloth.size.y, xform, glm::vec4(0.2, 0.2, 0.2, 1.0));

    cloth.vertices[0] = device.createCpuVisibleBuffer(plane.vertices.data(), sizeof(Vertex) * plane.vertices.size()
                                                    , VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    cloth.vertices[1] = device.createCpuVisibleBuffer(plane.vertices.data(), sizeof(Vertex) * plane.vertices.size()
                                                    , VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    cloth.vertexCount = plane.vertices.size();

    cloth.indices = device.createDeviceLocalBuffer(plane.indices.data(), sizeof(uint32_t) * plane.indices.size(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    cloth.indexCount = plane.indices.size();

    textures::fromFile(device, cloth.diffuseMap, "../../data/textures/cloth/Cloth_Diffuse_3K.jpg", true);

    constants.inv_cloth_size = cloth.size/(cloth.gridSize - glm::vec2(1));
}

void ClothDemo::createSphere(){
    auto& xform = sphere.ubo.xform;
    xform = glm::translate(glm::mat4(1), {0, sphere.radius * 2, 0});
    xform = glm::scale(xform, glm::vec3(sphere.radius));
    sphere.ubo.radius = sphere.radius;
    sphere.ubo.center = (xform * glm::vec4(0, 0, 0, 1)).xyz();

    auto s = primitives::sphere(25, 25, 1.0f, xform,  {1, 1, 0, 1});
    sphere.vertices = device.createDeviceLocalBuffer(s.vertices.data(), sizeof(Vertex) * s.vertices.size(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    sphere.indices = device.createDeviceLocalBuffer(s.indices.data(), sizeof(uint32_t) * s.indices.size(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    sphere.indexCount = s.indices.size();

    sphere.uboBuffer = device.createDeviceLocalBuffer(&sphere.ubo, sizeof(sphere.ubo), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
}

void ClothDemo::loadModel() {
    phong::VulkanDrawableInfo info{};
    info.vertexUsage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
    info.indexUsage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
//    phong::load(R"(C:\Users\Josiah\OneDrive\media\models\Nissan FairladyZ 2009\basemeshobj.obj)", device, descriptorPool, model, info, true, 30);
    phong::load(R"(C:\Users\Josiah\OneDrive\media\models\werewolf.obj)", device, descriptorPool, model, info, true, 40);
  //  phong::load(R"(C:\Users\Josiah\OneDrive\media\models\Lucy-statue\metallic-lucy-statue-stanford-scan.obj)",device, descriptorPool, model, info, true, 40);
 //   phong::load("../../data/models/bigship1.obj", device, descriptorPool, model, info, true, 40);
    modelInstance.object = rt::TriangleMesh{&model};
    modelInstance.xform = glm::translate(glm::mat4(1), {0, model.height() * 0.5f, 0});
    modelInstance.xformIT = glm::inverseTranspose(modelInstance.xform);
    modelData.xform = modelInstance.xform;
    modelData.xformIT = modelInstance.xformIT;
    modelData.numTriangles = model.numTriangles();

    modelBuffer = device.createDeviceLocalBuffer(&modelData, sizeof(ModelData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    createAccelerationStructure({ modelInstance });
}


void ClothDemo::createFloor() {
    auto xform = glm::rotate(glm::mat4{1}, -glm::half_pi<float>(), {1, 0, 0});
    auto plane = primitives::plane(50, 50, 250, 250, xform, {0, 0, 1, 0});

    floor.vertices = device.createDeviceLocalBuffer(plane.vertices.data(), sizeof(Vertex) * plane.vertices.size(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    floor.indices = device.createDeviceLocalBuffer(plane.indices.data(), sizeof(uint32_t) * plane.indices.size(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    floor.indexCount = plane.indices.size();
}

void ClothDemo::onSwapChainDispose() {
    dispose(pipelines.wireframe);
    dispose(pipelines.point);
    dispose(pipelines.normals);
    dispose(raytrace.pipeline);
    descriptorPool.free({raytrace.descriptorSet});
}

void ClothDemo::onSwapChainRecreation() {
    createPipelines();
    createRayTraceDescriptorSet();
    createRayTracePipeline();
    createShaderBindingTables();
    cameraController->onResize(width, height);
}

VkCommandBuffer *ClothDemo::buildCommandBuffers(uint32_t imageIndex, uint32_t &numCommandBuffers) {

    auto& commandBuffer = commandBuffers[imageIndex];

    static std::array<VkCommandBuffer, 1> cmdBuffers{};
    cmdBuffers[0] = commandBuffer;
   // cmdBuffers[1] = dispatchCompute();
    numCommandBuffers = cmdBuffers.size();

    auto beginInfo = initializers::commandBufferBeginInfo();
    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    std::array<VkClearValue, 2> clearValues{ {0.0f, 0.0f, 0.0f, 0.0f} };
    clearValues[1].depthStencil = {1.0f, 0u};

    auto renderPassInfo = initializers::renderPassBeginInfo();
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = framebuffers[imageIndex];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = swapChain.extent;
    renderPassInfo.clearValueCount = COUNT(clearValues);
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    if(shading == Shading::WIREFRAME) {
        drawWireframe(commandBuffer);
    }else if(shading == Shading::SHADED) {
        drawShaded(commandBuffer);
    }
    renderUI(commandBuffer);

    vkCmdEndRenderPass(commandBuffer);
    vkEndCommandBuffer(commandBuffer);

    return cmdBuffers.data();
}

void ClothDemo::drawWireframe(VkCommandBuffer commandBuffer) {
    static glm::mat4 identity{1};
    VkDeviceSize offset = 0;

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.wireframe);
    cameraController->push(commandBuffer, pipelineLayouts.wireframe, identity);
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, floor.vertices, &offset);
    vkCmdBindIndexBuffer(commandBuffer, floor.indices, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(commandBuffer, floor.indexCount, 1, 0, 0, 0);

//    vkCmdBindVertexBuffers(commandBuffer, 0, 1, sphere.vertices, &offset);
//    vkCmdBindIndexBuffer(commandBuffer, sphere.indices, 0, VK_INDEX_TYPE_UINT32);
//    vkCmdDrawIndexed(commandBuffer, sphere.indexCount, 1, 0, 0, 0);

    static std::array<VkDeviceSize, 2> offsets{0u, 0u};
    static std::array<VkBuffer, 2> buffers{cloth.vertices[input_index], cloth.vertexAttributes };
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers.data(), offsets.data());
    vkCmdBindIndexBuffer(commandBuffer, cloth.indices, 0, VK_INDEX_TYPE_UINT32);

    vkCmdDrawIndexed(commandBuffer, cloth.indexCount, 1, 0, 0, 0);

    if(showNormals) {
        static glm::vec4 normalColor{1, 1, 0, 1};
        static float normalLength = glm::length(constants.inv_cloth_size) * 0.5f;
        static std::array<char, sizeof(normalColor) + sizeof(normalLength)> normalConstants{};

        std::memcpy(normalConstants.data(), &normalColor[0], sizeof(normalColor));
        std::memcpy(normalConstants.data() + sizeof(normalColor), &normalLength, sizeof(normalLength));
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.normals);
        cameraController->push(commandBuffer, pipelineLayouts.normals, identity,VK_SHADER_STAGE_GEOMETRY_BIT);
        vkCmdPushConstants(commandBuffer, pipelineLayouts.normals, VK_SHADER_STAGE_GEOMETRY_BIT, sizeof(Camera),
                           normalConstants.size(), normalConstants.data());
        vkCmdDraw(commandBuffer, cloth.vertexCount, 1, 0, 0);
    }

    if(showPoints) {
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.point);
        cameraController->push(commandBuffer, pipelineLayouts.point, identity);


        static glm::vec4 pointColor{1, 0, 0, 1};
        vkCmdPushConstants(commandBuffer, pipelineLayouts.point, VK_SHADER_STAGE_VERTEX_BIT, sizeof(Camera),
                           sizeof(glm::vec4), &pointColor[0]);
        vkCmdDraw(commandBuffer, cloth.vertexCount, 1, 0, 0);
    }

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.spaceShip);
    cameraController->push(commandBuffer, pipelineLayouts.spaceShip, modelInstance.xform);
    modelInstance.object.drawable->draw(commandBuffer, pipelineLayouts.spaceShip);
}

void ClothDemo::drawShaded(VkCommandBuffer commandBuffer) {
    VkDeviceSize offset = 0;
    static glm::mat4 identity{1};
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.wireframe);
    cameraController->push(commandBuffer, pipelineLayouts.wireframe, identity);
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, floor.vertices, &offset);
    vkCmdBindIndexBuffer(commandBuffer, floor.indices, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(commandBuffer, floor.indexCount, 1, 0, 0, 0);

    static int useTexture;
    useTexture = 1;
    static std::array<char, sizeof(int) + sizeof(float)> lightParams{};
    std::memcpy(lightParams.data(), &useTexture, sizeof(int));
    std::memcpy(lightParams.data() + sizeof(int), &shine, sizeof(float));

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.shaded);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.shaded, 0, 1, &cloth.descriptorSet, 0, nullptr);
    cameraController->push(commandBuffer, pipelineLayouts.shaded, identity);
    vkCmdPushConstants(commandBuffer, pipelineLayouts.shaded, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(Camera), sizeof(int) + sizeof(float), lightParams.data());
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, cloth.vertices[input_index], &offset);
    vkCmdBindIndexBuffer(commandBuffer, cloth.indices, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(commandBuffer, cloth.indexCount, 1, 0, 0, 0);

//    useTexture = 0;
//    std::memcpy(lightParams.data(), &useTexture, sizeof(int));
//    std::memcpy(lightParams.data() + sizeof(int), &shine, sizeof(float));
//    vkCmdPushConstants(commandBuffer, pipelineLayouts.shaded, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(Camera), sizeof(int) + sizeof(float), lightParams.data());
//    vkCmdBindVertexBuffers(commandBuffer, 0, 1, sphere.vertices, &offset);
//    vkCmdBindIndexBuffer(commandBuffer, sphere.indices, 0, VK_INDEX_TYPE_UINT32);
//    vkCmdDrawIndexed(commandBuffer, sphere.indexCount, 1, 0, 0, 0);

      vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.spaceShip);
      cameraController->push(commandBuffer, pipelineLayouts.spaceShip, modelInstance.xform);
      modelInstance.object.drawable->draw(commandBuffer, pipelineLayouts.spaceShip);

//    if(showPoints) {
//        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.point);
//        cameraController->push(commandBuffer, pipelineLayouts.point);
//
//
//        static glm::vec4 pointColor{1, 0, 0, 1};
//        vkCmdPushConstants(commandBuffer, pipelineLayouts.point, VK_SHADER_STAGE_VERTEX_BIT, sizeof(Camera),
//                           sizeof(glm::vec4), &pointColor[0]);
//        vkCmdBindVertexBuffers(commandBuffer, 0, 1, cloth.vertices[input_index], &offset);
//        vkCmdDraw(commandBuffer, cloth.vertexCount, 1, 0, 0);
//    }
}

void ClothDemo::update(float time) {
    if(!ImGui::IsAnyItemActive()) {
        cameraController->update(time);
    }
    constants.timeStep = frameTime/numIterations;
    constants.elapsedTime = elapsedTime;
  //  constants.timeStep = 0.0005f/numIterations;
  //  constants.timeStep = time/numIterations;
  //  runPhysics(time);
    dispatchCompute();
}

void ClothDemo::checkAppInputs() {
    cameraController->processInput();
}

void ClothDemo::initCamera() {
    FirstPersonSpectatorCameraSettings settings;
//    OrbitingCameraSettings settings;
//    settings.orbitMinZoom = 10.f;
//    settings.orbitMaxZoom = 200.f;
//    settings.offsetDistance = 60.f;
//    settings.modelHeight = cloth.size.y;
    settings.floorOffset = cloth.size.x * 0.5;
    settings.velocity = glm::vec3{10};
    settings.acceleration = glm::vec3(20);
    settings.aspectRatio = float(swapChain.extent.width)/float(swapChain.extent.height);
    cameraController = std::make_unique<SpectatorCameraController>(device, swapChainImageCount, currentImageIndex, dynamic_cast<InputManager&>(*this), settings);
    cameraController->lookAt({-20, 37, 8}, {0.5, -0.6, 0.6}, {0, 1, 0});
}

void ClothDemo::createPipelines() {
    auto flatVertexModule = VulkanShaderModule{"../../data/shaders/flat.vert.spv", device};
    auto flatFragmentModule = VulkanShaderModule{"../../data/shaders/flat.frag.spv", device};

    auto wireFrameStages = initializers::vertexShaderStages({
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
    depthStencilState.depthTestEnable = VK_TRUE;
    depthStencilState.depthWriteEnable = VK_TRUE;
    depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencilState.minDepthBounds = 0.0f;
    depthStencilState.maxDepthBounds = 1.0f;

    auto colorBlendAttachments = initializers::colorBlendStateAttachmentStates();
    auto colorBlendState = initializers::colorBlendState(colorBlendAttachments);

    dispose(pipelineLayouts.wireframe);
    pipelineLayouts.wireframe = device.createPipelineLayout({}, {Camera::pushConstant()});

    auto createInfo = initializers::graphicsPipelineCreateInfo();
    createInfo.flags = VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT;
    createInfo.stageCount = COUNT(wireFrameStages);
    createInfo.pStages = wireFrameStages.data();
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


    auto pointVertexShaderModule = VulkanShaderModule{"../../data/shaders/point.vert.spv", device };
    auto pointFragShaderModule = VulkanShaderModule{"../../data/shaders/point.frag.spv", device };
    auto pointStages = initializers::vertexShaderStages({
        {pointVertexShaderModule, VK_SHADER_STAGE_VERTEX_BIT},
        {pointFragShaderModule, VK_SHADER_STAGE_FRAGMENT_BIT}
    });

    float pointSize = 5.0;
    VkSpecializationMapEntry entry{ 0, 0, sizeof(float)};
    VkSpecializationInfo specializationInfo{1, &entry, sizeof(float), &pointSize};
    pointStages[0].pSpecializationInfo = &specializationInfo;

    inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
    inputAssemblyState.primitiveRestartEnable = VK_FALSE;

    std::vector<VkPushConstantRange> pushConstants(1);
    pushConstants[0].offset = 0;
    pushConstants[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstants[0].size = sizeof(Camera) + sizeof(glm::vec4);

    dispose(pipelineLayouts.point);
    pipelineLayouts.point = device.createPipelineLayout({}, pushConstants);

    createInfo.flags = VK_PIPELINE_CREATE_DERIVATIVE_BIT;
    createInfo.stageCount = COUNT(pointStages);
    createInfo.pStages = pointStages.data();
    createInfo.pInputAssemblyState = &inputAssemblyState;
    createInfo.layout = pipelineLayouts.point;
    createInfo.basePipelineIndex = -1;
    createInfo.basePipelineHandle = pipelines.wireframe;

    pipelines.point = device.createGraphicsPipeline(createInfo);

    auto normalVertexShaderModule = VulkanShaderModule{"../../data/shaders/draw_normals.vert.spv", device};
    auto normalGeometryShaderModule = VulkanShaderModule{"../../data/shaders/draw_normals.geom.spv", device};

    auto drawNormalStages = initializers::vertexShaderStages({
        {normalVertexShaderModule, VK_SHADER_STAGE_VERTEX_BIT},
        {normalGeometryShaderModule, VK_SHADER_STAGE_GEOMETRY_BIT},
        {pointFragShaderModule, VK_SHADER_STAGE_FRAGMENT_BIT}
    });

    pushConstants[0].stageFlags = VK_SHADER_STAGE_GEOMETRY_BIT;
    pushConstants[0].size = sizeof(Camera) + sizeof(glm::vec4) + sizeof(float);

    dispose(pipelineLayouts.normals);
    pipelineLayouts.normals = device.createPipelineLayout({}, pushConstants);
    createInfo.stageCount = COUNT(drawNormalStages);
    createInfo.pStages = drawNormalStages.data();
    createInfo.layout = pipelineLayouts.normals;

    pipelines.normals = device.createGraphicsPipeline(createInfo);

    auto shadedVertexShaderModule = VulkanShaderModule{"../../data/shaders/phong/phong.vert.spv", device};
    auto shadedFragmentShaderModule = VulkanShaderModule{"../../data/shaders/phong/phong.frag.spv", device};
    auto shadedStages = initializers::vertexShaderStages({
        {shadedVertexShaderModule, VK_SHADER_STAGE_VERTEX_BIT},
        {shadedFragmentShaderModule, VK_SHADER_STAGE_FRAGMENT_BIT}
    });

    inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
    inputAssemblyState.primitiveRestartEnable = VK_TRUE;
    rasterState.polygonMode = VK_POLYGON_MODE_FILL;

    dispose(pipelineLayouts.shaded);
    pipelineLayouts.shaded = device.createPipelineLayout({ cloth.setLayout }
    , {Camera::pushConstant(), {VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(Camera), sizeof(int) + sizeof(float)}});

    createInfo.stageCount = COUNT(shadedStages);
    createInfo.pStages = shadedStages.data();
    createInfo.pInputAssemblyState = &inputAssemblyState;
    createInfo.pRasterizationState = &rasterState;
    createInfo.layout = pipelineLayouts.shaded;

    pipelines.shaded = device.createGraphicsPipeline(createInfo);

    VulkanShaderModule vertexShaderModule = VulkanShaderModule{"../../data/shaders/demo/spaceship.vert.spv", device};
    VulkanShaderModule fragmentShaderModule = VulkanShaderModule{"../../data/shaders/demo/spaceship.frag.spv", device};

    auto spaceShipStages = initializers::vertexShaderStages({
         {vertexShaderModule, VK_SHADER_STAGE_VERTEX_BIT},
         {fragmentShaderModule, VK_SHADER_STAGE_FRAGMENT_BIT}
    });

    inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssemblyState.primitiveRestartEnable = VK_FALSE;

    dispose(pipelineLayouts.spaceShip);
    pipelineLayouts.spaceShip = device.createPipelineLayout({ model.descriptorSetLayout }, {Camera::pushConstant()});

    createInfo.stageCount = COUNT(spaceShipStages);
    createInfo.pStages = spaceShipStages.data();
    createInfo.pInputAssemblyState = &inputAssemblyState;
    createInfo.layout = pipelineLayouts.spaceShip;

    pipelines.spaceShip = device.createGraphicsPipeline(createInfo);

    rasterState.polygonMode = VK_POLYGON_MODE_LINE;
    createInfo.pRasterizationState = &rasterState;
    pipelines.spaceShipWireframe = device.createGraphicsPipeline(createInfo);

}

void ClothDemo::createRayTraceDescriptorSetLayout() {
    std::vector<VkDescriptorSetLayoutBinding> binding(1);
    binding[0].binding = 0;
    binding[0].descriptorCount = 1;
    binding[0].descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
    binding[0].stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR;

    raytrace.descriptorSetLayout = device.createDescriptorSetLayout(binding);

}

void ClothDemo::createRayTraceDescriptorSet() {
    raytrace.descriptorSet = descriptorPool.allocate({ raytrace.descriptorSetLayout}).front();

    VkWriteDescriptorSetAccelerationStructureKHR asWrites{};
    asWrites.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
    asWrites.accelerationStructureCount = 1;
    asWrites.pAccelerationStructures = rtBuilder.accelerationStructure();

    auto writes = initializers::writeDescriptorSets<1>();
    writes[0].pNext = &asWrites;
    writes[0].dstSet = raytrace.descriptorSet;
    writes[0].dstBinding = 0;
    writes[0].descriptorCount = 1;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;

    vkUpdateDescriptorSets(device, COUNT(writes), writes.data(), 0, VK_NULL_HANDLE );
}

void ClothDemo::createRayTracePipeline() {
    auto rayGenShader = VulkanShaderModule{"../../data/shaders/cloth/raygen.rgen.spv", device};
    auto missGenShader = VulkanShaderModule{"../../data/shaders/cloth/miss.rmiss.spv", device};
    auto hitShader = VulkanShaderModule{"../../data/shaders/cloth/closesthit.rchit.spv", device};

    auto stages = initializers::vertexShaderStages(
            {
                { rayGenShader, VK_SHADER_STAGE_RAYGEN_BIT_KHR},
                { missGenShader, VK_SHADER_STAGE_MISS_BIT_KHR},
                {hitShader, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR}
            });

    shaderGroups.clear();
    VkRayTracingShaderGroupCreateInfoKHR shaderGroupInfo = initializers::rayTracingShaderGroupCreateInfo();
    shaderGroupInfo.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
    shaderGroupInfo.generalShader = shaderGroups.size();
    shaderGroupInfo.closestHitShader = VK_SHADER_UNUSED_KHR;
    shaderGroupInfo.anyHitShader = VK_SHADER_UNUSED_KHR;
    shaderGroupInfo.intersectionShader = VK_SHADER_UNUSED_KHR;
    shaderGroups.push_back(shaderGroupInfo);

    shaderGroupInfo.generalShader = shaderGroups.size();
    shaderGroups.push_back(shaderGroupInfo);

    shaderGroupInfo.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
    shaderGroupInfo.generalShader = VK_SHADER_UNUSED_KHR;
    shaderGroupInfo.closestHitShader = shaderGroups.size();
    shaderGroups.push_back(shaderGroupInfo);

    dispose(raytrace.layout);
    raytrace.layout = device.createPipelineLayout({raytrace.descriptorSetLayout, positionSetLayout, positionSetLayout});

    VkRayTracingPipelineCreateInfoKHR createInfo = initializers::rayTracingPipelineCreateInfo();
    createInfo.stageCount = COUNT(stages);
    createInfo.pStages = stages.data();
    createInfo.groupCount = COUNT(shaderGroups);
    createInfo.pGroups = shaderGroups.data();
    createInfo.maxPipelineRayRecursionDepth = 1;
    createInfo.layout = raytrace.layout;

    raytrace.pipeline = device.createRayTracingPipeline(createInfo);
}

void ClothDemo::createShaderBindingTables() {
    assert(raytrace.pipeline);
    const auto [handleSize, handleSizeAligned] = getShaderGroupHandleSizingInfo();
    const auto groupCount = COUNT(shaderGroups);
    const auto sbtSize = groupCount * handleSizeAligned;

    std::vector<uint8_t> shaderHandleStorage(sbtSize);

    ext.vkGetRayTracingShaderGroupHandlesKHR(device, raytrace.pipeline, 0, groupCount, sbtSize, shaderHandleStorage.data());

    const VkBufferUsageFlags usageFlags = VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

    void* rayGenPtr = shaderHandleStorage.data();
    void* missPtr = shaderHandleStorage.data() + handleSizeAligned;
    void* hitPtr = shaderHandleStorage.data() + handleSizeAligned * 2;
    createShaderBindingTable(shaderBindingTables.rayGen, rayGenPtr, usageFlags, VMA_MEMORY_USAGE_GPU_ONLY, 1);
    createShaderBindingTable(shaderBindingTables.miss, missPtr, usageFlags, VMA_MEMORY_USAGE_GPU_ONLY, 1);
    createShaderBindingTable(shaderBindingTables.hit, hitPtr, usageFlags, VMA_MEMORY_USAGE_GPU_ONLY, 1);
}

void ClothDemo::rayTraceToComputeBarrier(VkCommandBuffer commandBuffer) {
    VkBufferMemoryBarrier  barrier = initializers::bufferMemoryBarrier();
    barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    barrier.srcQueueFamilyIndex = *device.queueFamilyIndex.compute;
    barrier.dstQueueFamilyIndex = *device.queueFamilyIndex.compute;
    barrier.buffer = cloth.vertices[0];
    barrier.offset = 0;
    barrier.size = cloth.vertices[0].size;

    static std::array<VkBufferMemoryBarrier, 2> barriers{};
    barriers[0] = barrier;

    barrier.buffer = cloth.vertices[1];
    barriers[1] = barrier;


    vkCmdPipelineBarrier(commandBuffer,
                         VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                         0,
                         0,
                         nullptr,
                         COUNT(barriers),
                         barriers.data(),
                         0,
                         nullptr);
}

void ClothDemo::computeToRayTraceBarrier(VkCommandBuffer commandBuffer) {
    VkBufferMemoryBarrier  barrier = initializers::bufferMemoryBarrier();
    barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    barrier.srcQueueFamilyIndex = *device.queueFamilyIndex.compute;
    barrier.dstQueueFamilyIndex = *device.queueFamilyIndex.compute;
    barrier.buffer = cloth.vertices[0];
    barrier.offset = 0;
    barrier.size = cloth.vertices[0].size;

    static std::array<VkBufferMemoryBarrier, 2> barriers{};
    barriers[0] = barrier;

    barrier.buffer = cloth.vertices[1];
    barriers[1] = barrier;


    vkCmdPipelineBarrier(commandBuffer,
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                         VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
                         0,
                         0,
                         nullptr,
                         COUNT(barriers),
                         barriers.data(),
                         0,
                         nullptr);
}

void ClothDemo::createPositionDescriptorSetLayout() {
    std::vector<VkDescriptorSetLayoutBinding> bindings(1);
    bindings[0].binding = 0;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[0].descriptorCount = 1;
    bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR;

    positionSetLayout = device.createDescriptorSetLayout(bindings);

    bindings[0].binding = 0;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[0].descriptorCount = 1;
    bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    cloth.setLayout = device.createDescriptorSetLayout(bindings);

    bindings.resize(4);
    bindings[0].binding = 0;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bindings[0].descriptorCount = 1;
    bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    bindings[1].binding = 1;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[1].descriptorCount = 1;
    bindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    bindings[2].binding = 2;
    bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[2].descriptorCount = 1;
    bindings[2].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    bindings[3].binding = 3;
    bindings[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[3].descriptorCount = 1;
    bindings[3].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    collision.setLayout = device.createDescriptorSetLayout(bindings);

}

void ClothDemo::createDescriptorPool() {
    uint32_t maxSet = 100;
    std::vector<VkDescriptorPoolSize> poolSizes(5);
    poolSizes[0].descriptorCount = 100 * maxSet;
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

    poolSizes[1].descriptorCount = 100 * maxSet;
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

    poolSizes[2].descriptorCount = 100 * maxSet;
    poolSizes[2].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

    poolSizes[3].descriptorCount = 100 * maxSet;
    poolSizes[3].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

    poolSizes[4].descriptorCount = 1;
    poolSizes[4].type = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;

    descriptorPool = device.createDescriptorPool(maxSet, poolSizes, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT);
}

void ClothDemo::createPositionDescriptorSet() {
    assert(cloth.vertices[0].buffer != VK_NULL_HANDLE && cloth.vertices[1].buffer != VK_NULL_HANDLE);


    auto descriptorSets = descriptorPool.allocate({ positionSetLayout, positionSetLayout, collision.setLayout, cloth.setLayout});
    positionDescriptorSets[0] = descriptorSets[0];
    positionDescriptorSets[1] = descriptorSets[1];
    collision.descriptorSet = descriptorSets[2];
    cloth.descriptorSet = descriptorSets[3];

    std::array<VkDescriptorBufferInfo, 5> bufferInfo{};
    bufferInfo[0].buffer = cloth.vertices[0];
    bufferInfo[0].offset = 0;
    bufferInfo[0].range = VK_WHOLE_SIZE;

    bufferInfo[1].buffer = cloth.vertices[1];
    bufferInfo[1].offset = 0;
    bufferInfo[1].range = VK_WHOLE_SIZE;

    bufferInfo[2].buffer = modelBuffer;
    bufferInfo[2].offset = 0;
    bufferInfo[2].range = VK_WHOLE_SIZE;

    bufferInfo[3].buffer = model.vertexBuffer;
    bufferInfo[3].offset = 0;
    bufferInfo[3].range = VK_WHOLE_SIZE;

    bufferInfo[4].buffer = model.indexBuffer;
    bufferInfo[4].offset = 0;
    bufferInfo[4].range = VK_WHOLE_SIZE;


    auto writes = initializers::writeDescriptorSets<6>();
    writes[0].dstSet = positionDescriptorSets[0];
    writes[0].dstBinding = 0;
    writes[0].dstArrayElement = 0;
    writes[0].descriptorCount = 1;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[0].pBufferInfo = &bufferInfo[0];

    writes[1].dstSet = positionDescriptorSets[1];
    writes[1].dstBinding = 0;
    writes[1].dstArrayElement = 0;
    writes[1].descriptorCount = 1;
    writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[1].pBufferInfo = &bufferInfo[1];

    writes[2].dstSet = collision.descriptorSet;
    writes[2].dstBinding = 0;
    writes[2].dstArrayElement = 0;
    writes[2].descriptorCount = 1;
    writes[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writes[2].pBufferInfo = &bufferInfo[2];


    writes[3].dstSet = collision.descriptorSet;
    writes[3].dstBinding = 1;
    writes[3].dstArrayElement = 0;
    writes[3].descriptorCount = 1;
    writes[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[3].pBufferInfo = &bufferInfo[3];

    writes[4].dstSet = collision.descriptorSet;
    writes[4].dstBinding = 2;
    writes[4].dstArrayElement = 0;
    writes[4].descriptorCount = 1;
    writes[4].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[4].pBufferInfo = &bufferInfo[4];


    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageView = cloth.diffuseMap.imageView;
    imageInfo.sampler = cloth.diffuseMap.sampler;
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    writes[5].dstSet = cloth.descriptorSet;
    writes[5].dstBinding = 0;
    writes[5].dstArrayElement = 0;
    writes[5].descriptorCount = 1;
    writes[5].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[5].pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(device, COUNT(writes), writes.data(), 0, VK_NULL_HANDLE);
}

void ClothDemo::createComputePipeline() {
    auto computeModule = VulkanShaderModule{"../../data/shaders/cloth/cloth.comp.spv", device };

    auto stage = initializers::vertexShaderStages({{computeModule, VK_SHADER_STAGE_COMPUTE_BIT}}).front();

    VkPushConstantRange range{};
    range.offset = 0;
    range.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    range.size = sizeof(constants);

    pipelineLayouts.compute = device.createPipelineLayout({positionSetLayout, positionSetLayout, collision.setLayout}, { range });

    auto info = initializers::computePipelineCreateInfo();
    info.stage = stage;
    info.layout = pipelineLayouts.compute;

    pipelines.compute = device.createComputePipeline(info);
}

VkCommandBuffer ClothDemo::dispatchCompute() {

    if(!startSim){
        if(elapsedTime > 5.0f){ // wait 5 seconds
            numIterations = std::max(1.0f, 1/(frameTime * framePerSecond));
            frameTime *= numIterations;
            startSim = true;
        }
        return nullptr;
    }

    commandPool.oneTimeCommand( [&](auto commandBuffer){
        static std::array<VkDescriptorSet, 3> descriptors{};
        static std::array<VkDescriptorSet, 3> rt_descriptors{};
        vkCmdResetQueryPool(commandBuffer, queryPool, 0, 2);
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelines.compute);
        vkCmdPushConstants(commandBuffer, pipelineLayouts.compute, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(constants), &constants);
        vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, queryPool, 0u);
        for(auto i = 0; i < numIterations; i++) {
            descriptors[0] = positionDescriptorSets[input_index];
            descriptors[1] = positionDescriptorSets[output_index];
            descriptors[2] = collision.descriptorSet;
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayouts.compute, 0, COUNT(descriptors), descriptors.data(), 0, nullptr);
            vkCmdDispatch(commandBuffer, cloth.gridSize.x/10, cloth.gridSize.y/10, 1);

         //   computeToRayTraceBarrier(commandBuffer);

//            VkStridedDeviceAddressRegionKHR nullShaderSbtEntry{};
//            rt_descriptors[0] = raytrace.descriptorSet;
//            rt_descriptors[1] = positionDescriptorSets[input_index];
//            rt_descriptors[2] = positionDescriptorSets[output_index];
//            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, raytrace.layout, 0, COUNT(rt_descriptors), rt_descriptors.data(), 0, nullptr);
//            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, raytrace.pipeline);
//            vkCmdTraceRaysKHR(
//                    commandBuffer,
//                    shaderBindingTables.rayGen,
//                    shaderBindingTables.miss,
//                    shaderBindingTables.hit,
//                    &nullShaderSbtEntry,
//                    cloth.gridSize.x * 10,
//                    cloth.gridSize.y * 10,
//                    1);

            if(i - 1 < numIterations){
              //  rayTraceToComputeBarrier(commandBuffer);
                computeToComputeBarrier(commandBuffer);
            }
            std::swap(input_index, output_index);

        }
        vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, queryPool, 1u);
    });


    return nullptr;
}

void ClothDemo::computeToComputeBarrier(VkCommandBuffer commandBuffer) {
    VkBufferMemoryBarrier  barrier = initializers::bufferMemoryBarrier();
    barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    barrier.srcQueueFamilyIndex = *device.queueFamilyIndex.compute;
    barrier.dstQueueFamilyIndex = *device.queueFamilyIndex.compute;
    barrier.buffer = cloth.vertices[0];
    barrier.offset = 0;
    barrier.size = cloth.vertices[0].size;

    static std::array<VkBufferMemoryBarrier, 2> barriers{};
    barriers[0] = barrier;

    barrier.buffer = cloth.vertices[1];
    barriers[1] = barrier;


    vkCmdPipelineBarrier(commandBuffer,
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                         0,
                         0,
                         nullptr,
                         COUNT(barriers),
                         barriers.data(),
                         0,
                         nullptr);
}

void ClothDemo::runPhysics(float time) {
    using namespace glm;
    vec3 gravity = vec3(0, -9.81, 0);
    int width = cloth.gridSize.x;
    int height = cloth.gridSize.y;

    for(int _ = 0; _ < numIterations; _++) {
        for (int j = 0; j < height; j++) {
            for (int i = 0; i < width; i++) {
                int id = j * width + i;
                const auto pid = ivec2{i, j};
           //     spdlog::info("point: [{}, {}]", i, j);
                runPhysics0(time, i, j);
            }
        }
        std::swap(input_index, output_index);
   //     spdlog::info("\n\n");
    }
}

void ClothDemo::runPhysics0(float time, int i, int j) {
    using namespace glm;
    vec3 gravity = vec3(0, -9.81, 0);
    float mass = constants.mass;
    float kd = constants.kd;
    float numPoints = cloth.gridSize.x * cloth.gridSize.y;
    float ksStruct = constants.ksStruct;
    float kdStruct = constants.kdStruct;
    float ksShear = constants.ksShear;
    float kdShear = constants.kdShear;
    float ksBend = constants.ksBend;
    float kdBend = constants.kdBend;

    int width = cloth.gridSize.x;
    int height = cloth.gridSize.y;

    int id = j * width + i;

    static std::array<ivec2, 12> neighbourIndices = {
            ivec2(0, 1), ivec2(1, 0), ivec2(0, -1), ivec2(-1, 0),  // structural neigbhours
            ivec2(-1, 1), ivec2(1, 1), ivec2(-1, -1), ivec2(1, -1),  // shear neigbhours
            ivec2(0, 2), ivec2(0, -2), ivec2(-2, 0), ivec2(2, 0)    // bend neigbhours
    };

    cloth.vertices[input_index].map<glm::vec4>([&, i, j](glm::vec4 *positionIn) {
        cloth.vertices[output_index].map<glm::vec4>([&](glm::vec4 *positionOut) {

            auto neighbour = [&](int ii, vec3& pos, vec3& prev_pos, float& ks, float& kd, float& rest_length, ivec2 pid){
                ivec2 coord = neighbourIndices[ii];
                ivec2 index =  coord + ivec2(pid.x, pid.y);
                if(index.x < 0 || index.x >= width || index.y < 0 || index.y >= height){
                    return false;
                }
                uint nid = index.y * width + index.x;


                pos = positionIn[nid].xyz;
                prev_pos = positionOut[nid].xyz;

                rest_length = length(vec2(coord) * constants.inv_cloth_size);
                if(ii < 4){
                    ks = ksStruct;
                    kd = kdStruct;
                }else if(ii < 8){
                    ks = ksShear;
                    kd = kdShear;
                }else if(ii < 12){
                    ks = ksBend;
                    kd = kdBend;
                }

              //  spdlog::info("ii: {}, coord: {}, index: {}, nid: {}, length: {}", ii, vec2(coord), vec2(index), nid, rest_length);

                return true;
            };

            float dt = constants.timeStep;
            float inv_dt = 1 / dt;
            vec3 pos = positionIn[id].xyz;
            vec3 prev_pos = positionOut[id].xyz;
            vec3 velocity = (pos - prev_pos) * inv_dt;
            vec3 force = mass * gravity + kd * -velocity;
            //vec3 force = vec3(0);

            for(int ii = 0; ii < 12; ii++){
                vec3 nPos;
                vec3 nPrev_pos;
                float ks;
                float k;
                float l;

                if(!neighbour(ii, nPos, nPrev_pos, ks, k, l, {i, j})){
                    continue;
                }
                vec3 d = nPos - pos;
                if(d.x == 0 && d.y == 0 && d.z == 0){
                    spdlog::info("No diff: id: {}, [{}, {}]", id,  i, j);
                }

                vec3 d_norm = normalize(d);
                float dist = length(d);
                vec3 nVelocity = (nPos - nPrev_pos) * inv_dt;

                vec3 f = d_norm * (ks * (dist - l) + k * dot(nVelocity - velocity, d_norm));

                force += f;

                if(id == 5 && ii == 0) {
                    spdlog::info(
                            "id: {}\n\tni: {}\n\tpos: {}\n\tprev_pos: {}\n\tnPos: {}\n\tdiff: {}\n\tks: {}\n\tkd: {}\n\tspring force {}\n\tforce: {}",
                            id, ii, pos, prev_pos, nPos, d, ks, k, f, force);
                }
            }

            float inv_mass = 1.0f / mass;
            if (id == (numPoints - width) || id == (numPoints - 1)) {
                inv_mass = 0;
            }

            vec3 a = force * inv_mass;
            auto dp = a * dt * dt;
            vec3 p = 2.f * pos - prev_pos + a * dt * dt;

              if (p.y < 0) p.y = 0;

//            spdlog::info("p(t): {}, p(t - 1): {}", pos, prev_pos);
//            spdlog::info("current: {}, new: {}, dp: {}", pos.xyz(), p.xyz(), dp);
            positionIn[id].xyz = pos;
            positionOut[id].xyz = p;

            std::array<VmaAllocation, 2> allocations{cloth.vertices[0].allocation, cloth.vertices[0].allocation};
            std::array<VkDeviceSize, 2> offsets{0, 0};
            std::array<VkDeviceSize, 2> sizes{VK_WHOLE_SIZE, VK_WHOLE_SIZE};
            vmaFlushAllocations(device.allocator, 2, allocations.data(), offsets.data(), sizes.data());
        });
    });
}

void ClothDemo::renderUI(VkCommandBuffer commandBuffer) {
    auto& imGuiPlugin = plugin<ImGuiPlugin>(IM_GUI_PLUGIN);

    ImGui::Begin("Cloth Simulation");
    ImGui::SetWindowSize("Cloth Simulation", {350, 400});
    static int option = 1;

    ImGui::RadioButton("wireframe", &option, 0);
    ImGui::SameLine();
    ImGui::RadioButton("shaded", &option, 1);
    shading = static_cast<Shading>(option);

    bool showWireframeOptions = shading == Shading::WIREFRAME;
    if(ImGui::CollapsingHeader("wireframe options", &showWireframeOptions, ImGuiTreeNodeFlags_DefaultOpen)){
        ImGui::Checkbox("points", &showPoints);
        ImGui::Checkbox("normals", &showNormals);
    }

    if(ImGui::CollapsingHeader("Spring Constants", ImGuiTreeNodeFlags_DefaultOpen)){
        ImGui::SliderFloat("structural", &constants.ksStruct, 10.0f, 1000.0f);
        ImGui::SliderFloat("shear", &constants.ksShear, 0.1f, 200.0f);
        ImGui::SliderFloat("bend", &constants.ksBend, 0.1f, 200.0f);
    }
    if(ImGui::CollapsingHeader("Damping Constants", ImGuiTreeNodeFlags_DefaultOpen)){
        ImGui::SliderFloat("structural_d", &constants.kdStruct, 0.1f, 10.0f);
        ImGui::SliderFloat("shear_d", &constants.kdShear, 0.1f, 10.0f);
        ImGui::SliderFloat("bend_d", &constants.kdBend, 0.1f, 10.0f);
        ImGui::SliderFloat("velocity", &constants.kd, 0.1f, 10.0f);
    }

    ImGui::SliderFloat("shine", &shine, 1, 100);
    ImGui::Text("%d iteration(s), timeStep: %.3f ms", numIterations, frameTime * 1000);
    ImGui::Text("Application average %.3f ms/frame, (%d FPS)", 1000.0/framePerSecond, framePerSecond);
    ImGui::Text("compute: %.3f ms", computeDuration);
    ImGui::End();

    imGuiPlugin.draw(commandBuffer);
}

void ClothDemo::checkInvariants() {

}

void ClothDemo::newFrame() {
 //   timeQueryResults = timerQueryPool.queryResult<TimeQueryResults>(VK_QUERY_RESULT_64_BIT);
    vkGetQueryPoolResults(device, queryPool, 0, 2, sizeof(timeQueryResults),
                          &timeQueryResults, sizeof(uint64_t), 0);
    auto duration = (timeQueryResults.computeEnd - timeQueryResults.computeStart);
    computeDuration = duration * 0.000001f;
}

void ClothDemo::initQueryPools() {
    VkQueryPoolCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
    info.queryType = VK_QUERY_TYPE_TIMESTAMP;
    info.queryCount = 2;
    ASSERT(vkCreateQueryPool(device, &info, nullptr, &queryPool));
}

void ClothDemo::cleanup() {
    vkDestroyQueryPool(device, queryPool, nullptr);
}

int main(){
    Settings settings;
    settings.vSync = false;
    settings.depthTest = true;
    settings.enabledFeatures.fillModeNonSolid = true;
    settings.enabledFeatures.geometryShader = true;
    settings.queueFlags = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT;
    spdlog::set_level(spdlog::level::err);

    std::unique_ptr<Plugin> imGui = std::make_unique<ImGuiPlugin>();

    try{
        auto app = ClothDemo{ settings };
        app.addPlugin(imGui);
        app.run();
    }catch(const std::runtime_error& err){
        spdlog::error(err.what());
    }
}