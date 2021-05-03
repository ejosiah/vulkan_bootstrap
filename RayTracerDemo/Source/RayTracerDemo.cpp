#define DEVICE_ADDRESS_BIT
#include <ImGuiPlugin.hpp>
#include "RayTracerDemo.hpp"
#include "spdlog/sinks/basic_file_sink.h"

RayTracerDemo::RayTracerDemo(const Settings& settings): VulkanBaseApp("Ray trace Demo", settings) {
    // Enable features required for ray tracing using feature chaining via pNext
    enabledBufferDeviceAddressFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
    enabledBufferDeviceAddressFeatures.bufferDeviceAddress = VK_TRUE;

    enabledRayTracingPipelineFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
    enabledRayTracingPipelineFeatures.rayTracingPipeline = VK_TRUE;
    enabledRayTracingPipelineFeatures.pNext = &enabledBufferDeviceAddressFeatures;

    enabledAccelerationStructureFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
    enabledAccelerationStructureFeatures.accelerationStructure = VK_TRUE;
    enabledAccelerationStructureFeatures.pNext = &enabledRayTracingPipelineFeatures;

    enabledDescriptorIndexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
    enabledDescriptorIndexingFeatures.pNext = &enabledAccelerationStructureFeatures;
    enabledDescriptorIndexingFeatures.runtimeDescriptorArray = VK_TRUE;

    deviceCreateNextChain = &enabledDescriptorIndexingFeatures;
}

void RayTracerDemo::initApp() {
    rtBuilder = rt::AccelerationStructureBuilder{&device};
    debugBuffer = device.createBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, sizeof(glm::vec4) * swapChain.extent.width * swapChain.extent.height);
    loadRayTracingPropertiesAndFeatures();
    createCommandPool();
    createDescriptorPool();
    initCamera();
    createInverseCam();
    createModel();
    loadSpaceShip();
    createBottomLevelAccelerationStructure();
    createTopLevelAccelerationStructure();
    createStorageImage();
    createDescriptorSetLayouts();
    createDescriptorSets();
    createGraphicsPipeline();
    createRayTracePipeline();
    createShaderBindingTable();
    loadTexture();
    createVertexBuffer();
    createCanvasDescriptorSetLayout();
    createCanvasDescriptorSet();
    createCanvasPipeline();
}

void RayTracerDemo::createInverseCam() {
    inverseCamProj = device.createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, sizeof(glm::mat4) * 2);
}

void RayTracerDemo::loadRayTracingPropertiesAndFeatures() {
    rayTracingPipelineProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
    VkPhysicalDeviceProperties2 properties{};
    properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    properties.pNext = &rayTracingPipelineProperties;

    vkGetPhysicalDeviceProperties2(device, &properties);
}

void RayTracerDemo::onSwapChainDispose() {
    dispose(graphics.pipeline);
    dispose(raytrace.pipeline);
}

void RayTracerDemo::onSwapChainRecreation() {
    createGraphicsPipeline();
    camera->onResize(swapChain.extent.width, swapChain.extent.height);
}

VkCommandBuffer *RayTracerDemo::buildCommandBuffers(uint32_t imageIndex, uint32_t &numCommandBuffers) {
    numCommandBuffers = 1;
    auto& commandBuffer = commandBuffers[imageIndex];
    VkCommandBufferBeginInfo beginInfo = initializers::commandBufferBeginInfo();
    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    std::array<VkClearValue, 2> clearValues{{0.0f, 0.0f, 0.0f, 1.0f}};
    clearValues[1].depthStencil = {1.0f, 0};

    VkRenderPassBeginInfo rPassBeginInfo = initializers::renderPassBeginInfo();
    rPassBeginInfo.renderPass = renderPass;
    rPassBeginInfo.framebuffer = framebuffers[imageIndex];
    rPassBeginInfo.renderArea.offset = {0, 0};
    rPassBeginInfo.renderArea.extent = swapChain.extent;
    rPassBeginInfo.clearValueCount = COUNT(clearValues);
    rPassBeginInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(commandBuffer, &rPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    if(!useRayTracing) {
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics.pipeline);
        camera->push(commandBuffer, graphics.layout);
        static VkDeviceSize offset = 0;
//        vkCmdBindVertexBuffers(commandBuffer, 0, 1, triangle.vertices, &offset);
//        vkCmdBindIndexBuffer(commandBuffer, triangle.indices, 0, VK_INDEX_TYPE_UINT16);
//        vkCmdDrawIndexed(commandBuffer, 3, 1, 0, 0, 0);
          spaceShip.draw(commandBuffer, graphics.layout);
    }else{
        drawCanvas(commandBuffer);
    }

    renderUI(commandBuffer);

    vkCmdEndRenderPass(commandBuffer);

  //  rayTrace(commandBuffer);

    vkEndCommandBuffer(commandBuffer);

    return &commandBuffer;
}

void RayTracerDemo::drawCanvas(VkCommandBuffer commandBuffer) {

    commandPool.oneTimeCommand([&](auto& cmdBuffer){
       rayTrace(cmdBuffer);
    });

    static std::vector<glm::vec4> output;
    if(debugOn){
        debugOn = false;
        output.clear();
        std::set<int> ids;
        std::set<int> cids;
        debugBuffer.use<glm::vec4>([&](glm::vec4 debugInfo){
            if(debugInfo.w != 1) return;
            output.push_back(debugInfo);
            ids.insert(int(debugInfo.x));
            cids.insert(int(debugInfo.y));
            spdlog::info("debuginfo: {}", debugInfo);
        });
        spdlog::info("ids: {}", ids);
        spdlog::info("cids: {}", cids);

    }

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, canvas.pipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, canvas.pipelineLayout, 0, 1, &canvas.descriptorSet, 0, VK_NULL_HANDLE);
    std::array<VkDeviceSize, 1> offsets = {0u};
    std::array<VkBuffer, 2> buffers{ vertexBuffer, vertexColorBuffer};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers.data() , offsets.data());
    vkCmdDraw(commandBuffer, 4, 1, 0, 0);
}

void RayTracerDemo::renderUI(VkCommandBuffer commandBuffer) {
    auto& imguiPlugin = plugin<ImGuiPlugin>(IM_GUI_PLUGIN);
    ImGui::Begin("Settings");
    ImGui::Checkbox("Ray trace mode", &useRayTracing);
    ImGui::Text("FPS: %d", framePerSecond);
    debugOn = ImGui::Button("debug");
    ImGui::End();

    imguiPlugin.draw(commandBuffer);
}

void RayTracerDemo::update(float time) {
    camera->update(time);
    auto cam = camera->cam();
    inverseCamProj.map<glm::mat4>([&](auto ptr){
        auto view = glm::inverse(cam.view);
        auto proj = glm::inverse(cam.proj);
        *ptr = view;
        *(ptr+1) = proj;
    });
}

void RayTracerDemo::checkAppInputs() {
    camera->processInput();
}

void RayTracerDemo::createCommandPool() {
    commandPool = device.createCommandPool(*device.queueFamilyIndex.graphics, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    commandBuffers = commandPool.allocate(swapChainImageCount);
}

void RayTracerDemo::createDescriptorPool() {
    auto maxSets = 1000u;
    std::vector<VkDescriptorPoolSize> poolSizes = {
            { VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1 },
            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 },
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 4}
    };
    auto phongPoolSizes = phong::getPoolSizes(maxSets);
    for(auto& poolSize : phongPoolSizes){
        auto itr = std::find_if(begin(poolSizes), end(poolSizes), [&](auto& p){
            return p.type == poolSize.type;
        });
        if(itr != end(poolSizes)){
            itr->descriptorCount += poolSize.descriptorCount;
        }else{
            poolSizes.push_back(poolSize);
        }
    }
    descriptorPool = device.createDescriptorPool(maxSets, poolSizes);
}

void RayTracerDemo::createDescriptorSetLayouts(){
    std::array<VkDescriptorSetLayoutBinding, 6> bindings{};
    bindings[0].binding = 0;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
    bindings[0].descriptorCount = 1;
    bindings[0].stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR;

    bindings[1].binding = 1;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    bindings[1].descriptorCount = 1;
    bindings[1].stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR;

    bindings[2].binding = 2;
    bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bindings[2].descriptorCount = 1;
    bindings[2].stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR;

    bindings[3].binding = 3;
    bindings[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[3].descriptorCount = 5;
    bindings[3].stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

    bindings[4].binding = 4;
    bindings[4].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[4].descriptorCount = 1;
    bindings[4].stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

    bindings[5].binding = 5;
    bindings[5].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[5].descriptorCount = 1;
    bindings[5].stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR | VK_SHADER_STAGE_RAYGEN_BIT_KHR;

    raytrace.descriptorSetLayout = device.createDescriptorSetLayout(bindings);
}

void RayTracerDemo::createDescriptorSets() {
    raytrace.descriptorSet = descriptorPool.allocate({ raytrace.descriptorSetLayout }).front();

    VkWriteDescriptorSetAccelerationStructureKHR accWrites{};
    accWrites.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
    accWrites.accelerationStructureCount = 1;
 //   accWrites.pAccelerationStructures = &topLevelAs.handle;
    accWrites.pAccelerationStructures = rtBuilder.accelerationStructure();

    auto writes = initializers::writeDescriptorSets<6>();
    writes[0].pNext = &accWrites;
    writes[0].dstSet = raytrace.descriptorSet;
    writes[0].dstBinding = 0;
    writes[0].descriptorCount = 1;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;

    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageView = storageImage.imageview;
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

    writes[1].dstSet = raytrace.descriptorSet;
    writes[1].dstBinding = 1;
    writes[1].descriptorCount = 1;
    writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    writes[1].pImageInfo = &imageInfo;

    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = inverseCamProj.buffer;
    bufferInfo.offset = 0;
    bufferInfo.range = VK_WHOLE_SIZE;

    writes[2].dstSet = raytrace.descriptorSet;
    writes[2].dstBinding = 2;
    writes[2].descriptorCount = 1;
    writes[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writes[2].pBufferInfo = &bufferInfo;

    // material writes
    std::vector<VkDescriptorBufferInfo> materialBufferInfos;
    materialBufferInfos.reserve(spaceShip.meshes.size());
    for(auto& mesh : spaceShip.meshes){
        VkDescriptorBufferInfo info{};
        info.buffer = mesh.material.materialBuffer;
        info.offset = 0;
        info.range = VK_WHOLE_SIZE;
        materialBufferInfos.push_back(info);
    }

    writes[3].dstSet = raytrace.descriptorSet;
    writes[3].dstBinding = 3;
    writes[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[3].descriptorCount = COUNT(materialBufferInfos);
    writes[3].pBufferInfo = materialBufferInfos.data();

    VkDescriptorBufferInfo matIdBufferInfo{};
    matIdBufferInfo.buffer = spaceShip.materialIdBuffer;
    matIdBufferInfo.offset = 0;
    matIdBufferInfo.range = VK_WHOLE_SIZE;

    writes[4].dstSet = raytrace.descriptorSet;
    writes[4].dstBinding = 4;
    writes[4].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[4].descriptorCount = 1;
    writes[4].pBufferInfo = &matIdBufferInfo;

    VkDescriptorBufferInfo debugInfo{};
    debugInfo.buffer = debugBuffer;
    debugInfo.offset = 0;
    debugInfo.range = VK_WHOLE_SIZE;


    writes[5].dstSet = raytrace.descriptorSet;
    writes[5].dstBinding = 5;
    writes[5].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[5].descriptorCount = 1;
    writes[5].pBufferInfo = &debugInfo;

    vkUpdateDescriptorSets(device, COUNT(writes), writes.data(), 0, nullptr);
}

void RayTracerDemo::initCamera() {
    OrbitingCameraSettings cameraSettings;
//    FirstPersonSpectatorCameraSettings cameraSettings;
    cameraSettings.orbitMinZoom = 0.1;
    cameraSettings.orbitMaxZoom = 512.0f;
    cameraSettings.offsetDistance = 1.0f;
    cameraSettings.modelHeight = 0.0f;
    cameraSettings.fieldOfView = 60.0f;
    cameraSettings.aspectRatio = float(swapChain.extent.width)/float(swapChain.extent.height);

    camera = std::make_unique<OrbitingCameraController>
            (device, swapChainImageCount, currentImageIndex, dynamic_cast<InputManager&>(*this), cameraSettings);
}

void RayTracerDemo::createModel() {
    std::vector<glm::vec3> vertices{
            {-1, -1, 0},
            {1, -1, 0},
            {0, 1, 0}
    };

    std::array<uint16_t, 3> indices{0, 1, 2};
    triangle.vertices = device.createDeviceLocalBuffer(vertices.data(), sizeof(glm::vec3) * 3,
                                                       VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                                                       VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
                                                       VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR);
    triangle.indices = device.createDeviceLocalBuffer(indices.data(), sizeof(uint16_t) * 3,
                                                      VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                                                             VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
                                                             VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR);
}

void RayTracerDemo::createGraphicsPipeline() {
//    auto vertexShaderModule = ShaderModule{"../../data/shaders/barycenter.vert.spv", device};
//    auto fragmentShaderModule = ShaderModule{"../../data/shaders/barycenter.frag.spv", device};

    ShaderModule vertexShaderModule = ShaderModule{ "../../data/shaders/demo/spaceship.vert.spv", device};
    ShaderModule fragmentShaderModule = ShaderModule{ "../../data/shaders/demo/spaceship.frag.spv", device};

    auto stages = initializers::vertexShaderStages({
        { vertexShaderModule, VK_SHADER_STAGE_VERTEX_BIT},
        {fragmentShaderModule, VK_SHADER_STAGE_FRAGMENT_BIT}
    });

//    std::array<VkVertexInputBindingDescription, 1> bindings{
//            { 0, sizeof(glm::vec3), VK_VERTEX_INPUT_RATE_VERTEX}
//    };
//    std::array<VkVertexInputAttributeDescription, 1> attributes{
//            {0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0}
//    };

    auto bindings = Vertex::bindingDisc();
    auto attributes = Vertex::attributeDisc();

    auto vertexInputState = initializers::vertexInputState(bindings, attributes);

    auto inputAssemblyState = initializers::inputAssemblyState();
    inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssemblyState.primitiveRestartEnable = VK_FALSE;

    auto viewport = initializers::viewport(swapChain.extent);
    auto scissors = initializers::scissor(swapChain.extent);

    auto viewportState = initializers::viewportState(viewport, scissors);

    auto rasterState = initializers::rasterizationState();
    rasterState.polygonMode = VK_POLYGON_MODE_FILL;
    rasterState.cullMode = VK_CULL_MODE_NONE;
    rasterState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterState.lineWidth = 1.0;

    auto multiSampleState = initializers::multisampleState();

    auto colorAttachmentState = initializers::colorBlendStateAttachmentStates();
    auto colorBlendState = initializers::colorBlendState(colorAttachmentState);

    auto depthStencilState = initializers::depthStencilState();
    depthStencilState.depthTestEnable = VK_TRUE;
    depthStencilState.depthWriteEnable = VK_TRUE;
    depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencilState.minDepthBounds = 0;
    depthStencilState.maxDepthBounds = 1;

    dispose(graphics.layout);
    graphics.layout = device.createPipelineLayout({ spaceShip.descriptorSetLayout }, {Camera::pushConstant()});

    auto createInfo = initializers::graphicsPipelineCreateInfo();
    createInfo.stageCount = COUNT(stages);
    createInfo.pStages = stages.data();
    createInfo.pVertexInputState = &vertexInputState;
    createInfo.pInputAssemblyState = &inputAssemblyState;
    createInfo.pViewportState = &viewportState;
    createInfo.pMultisampleState = &multiSampleState;
    createInfo.pColorBlendState = &colorBlendState;
    createInfo.pDepthStencilState = &depthStencilState;
    createInfo.pRasterizationState = &rasterState;
    createInfo.layout = graphics.layout;
    createInfo.renderPass = renderPass;
    createInfo.subpass = 0;

    graphics.pipeline = device.createGraphicsPipeline(createInfo);
}

RayTracingScratchBuffer RayTracerDemo::createScratchBuffer(VkDeviceSize size) {
    RayTracingScratchBuffer scratchBuffer{};
//
//
//    VkBufferCreateInfo bufferInfo = initializers::bufferCreateInfo();
//    bufferInfo.size = size;
//    bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
//    vkCreateBuffer(device, &bufferInfo, nullptr, &scratchBuffer.buffer);
//
//    VkMemoryRequirements  memoryRequirements{};
//    vkGetBufferMemoryRequirements(device, scratchBuffer.buffer, &memoryRequirements);
//
//    VkMemoryAllocateFlagsInfo  allocateFlagsInfo{};
//    allocateFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
//    allocateFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;
//
//    VkMemoryAllocateInfo  allocInfo{};
//    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
//    allocInfo.pNext = &allocateFlagsInfo;
//    allocInfo.allocationSize = memoryRequirements.size;
//    allocInfo.memoryTypeIndex = device.getMemoryTypeIndex(memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
//
//    vkAllocateMemory(device, &allocInfo, nullptr, &scratchBuffer.memory);
//    vkBindBufferMemory(device, scratchBuffer.buffer, scratchBuffer.memory, 0);

    scratchBuffer.buffer = device.createBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VMA_MEMORY_USAGE_GPU_ONLY, size, "");

    VkBufferDeviceAddressInfo bufferDeviceAddressInfo{};
    bufferDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    bufferDeviceAddressInfo.buffer = scratchBuffer.buffer;
    scratchBuffer.deviceAddress = vkGetBufferDeviceAddress(device, &bufferDeviceAddressInfo);

    return scratchBuffer;

}

void RayTracerDemo::createBottomLevelAccelerationStructure() {
    glm::mat4 xform{1};
    xform = glm::transpose(xform);
    VulkanBuffer xformBuffer = device.createDeviceLocalBuffer(&xform, sizeof(VkTransformMatrixKHR), VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR);

    VkDeviceOrHostAddressConstKHR vertexBufferDeviceAddress{};
    VkDeviceOrHostAddressConstKHR indexBufferDeviceAddress{};
    VkDeviceOrHostAddressConstKHR xformBufferDeviceAddress{};

    vertexBufferDeviceAddress.deviceAddress = device.getAddress(triangle.vertices);
    indexBufferDeviceAddress.deviceAddress = device.getAddress(triangle.indices);
    xformBufferDeviceAddress.deviceAddress = device.getAddress(xformBuffer);

    // build
    VkAccelerationStructureGeometryKHR accelerationStructureGeometry{};
    accelerationStructureGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    accelerationStructureGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
    accelerationStructureGeometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
    accelerationStructureGeometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
    accelerationStructureGeometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
    accelerationStructureGeometry.geometry.triangles.vertexData = vertexBufferDeviceAddress;
    accelerationStructureGeometry.geometry.triangles.maxVertex = 3;
    accelerationStructureGeometry.geometry.triangles.vertexStride = sizeof(glm::vec3);
    accelerationStructureGeometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT16;
    accelerationStructureGeometry.geometry.triangles.indexData = indexBufferDeviceAddress;
    accelerationStructureGeometry.geometry.triangles.transformData.deviceAddress = 0;
    accelerationStructureGeometry.geometry.triangles.transformData.hostAddress = nullptr;
  //  accelerationStructureGeometry.geometry.triangles.transformData = xformBufferDeviceAddress;

    // get size info
    VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo{};
    accelerationStructureBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    accelerationStructureBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    accelerationStructureBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    accelerationStructureBuildGeometryInfo.geometryCount = 1;
    accelerationStructureBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;

    const uint32_t numTriangles = 1;
    VkAccelerationStructureBuildSizesInfoKHR buildSizeInfo{};
    buildSizeInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
    ext.vkGetAccelerationStructureBuildSizesKHR(device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
                                            &accelerationStructureBuildGeometryInfo, &numTriangles, &buildSizeInfo);

    bottomLevelAS.buffer = device.createBuffer(VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
            , VMA_MEMORY_USAGE_GPU_ONLY, buildSizeInfo.accelerationStructureSize);

    VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo{};
    accelerationStructureCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
    accelerationStructureCreateInfo.buffer = bottomLevelAS.buffer;
    accelerationStructureCreateInfo.size = buildSizeInfo.accelerationStructureSize;
    accelerationStructureCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    ext.vkCreateAccelerationStructureKHR(device, &accelerationStructureCreateInfo, nullptr, &bottomLevelAS.handle);

    auto scratchBuffer = createScratchBuffer(buildSizeInfo.buildScratchSize);

    accelerationStructureBuildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    accelerationStructureBuildGeometryInfo.dstAccelerationStructure = bottomLevelAS.handle;
    accelerationStructureBuildGeometryInfo.scratchData.deviceAddress = scratchBuffer.deviceAddress;

    VkAccelerationStructureBuildRangeInfoKHR  accelerationStructureBuildRangeInfo{};
    accelerationStructureBuildRangeInfo.primitiveCount = numTriangles;
    accelerationStructureBuildRangeInfo.primitiveOffset = 0;
    accelerationStructureBuildRangeInfo.firstVertex = 0;
    accelerationStructureBuildRangeInfo.transformOffset = 0;
    std::vector<VkAccelerationStructureBuildRangeInfoKHR*> accelerationBuildStructureRangeInfos = { &accelerationStructureBuildRangeInfo };

    commandPool.oneTimeCommand( [&](auto commandBuffer){
       ext.vkCmdBuildAccelerationStructuresKHR(commandBuffer, 1, &accelerationStructureBuildGeometryInfo, accelerationBuildStructureRangeInfos.data());
    });

    VkAccelerationStructureDeviceAddressInfoKHR accelerationStructureDeviceAddressInfo{};
    accelerationStructureDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
    accelerationStructureDeviceAddressInfo.accelerationStructure = bottomLevelAS.handle;
    bottomLevelAS.deviceAddress = ext.vkGetAccelerationStructureDeviceAddressKHR(device, &accelerationStructureDeviceAddressInfo);

    rtBuilder.buildAs({spaceShipInstance});

}

void RayTracerDemo::createTopLevelAccelerationStructure() {
    VkTransformMatrixKHR transformMatrix = {
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f };

    VkAccelerationStructureInstanceKHR instance{};
    instance.transform = transformMatrix;
    instance.instanceCustomIndex = 0;
    instance.mask = 0xFF;
    instance.instanceShaderBindingTableRecordOffset = 0;
    instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_CULL_DISABLE_BIT_NV;
    instance.accelerationStructureReference = bottomLevelAS.deviceAddress;

    VulkanBuffer instanceBuffer = device.createDeviceLocalBuffer(&instance, sizeof(VkAccelerationStructureInstanceKHR),
            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR);

    VkDeviceOrHostAddressConstKHR instanceDataDeviceAddress{};
    instanceDataDeviceAddress.deviceAddress = device.getAddress(instanceBuffer);

    VkAccelerationStructureGeometryKHR geometry{};
    geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    geometry.geometryType =  VK_GEOMETRY_TYPE_INSTANCES_KHR;
    geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
    geometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
    geometry.geometry.instances.arrayOfPointers = VK_FALSE;
    geometry.geometry.instances.data = instanceDataDeviceAddress;

    VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo{};
    accelerationStructureBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    accelerationStructureBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    accelerationStructureBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_BUILD_BIT_KHR;
    accelerationStructureBuildGeometryInfo.geometryCount = 1;
    accelerationStructureBuildGeometryInfo.pGeometries = &geometry;

    uint32_t primitive_count = 1;
    VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo{};
    accelerationStructureBuildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
    ext.vkGetAccelerationStructureBuildSizesKHR(
            device,
            VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
            &accelerationStructureBuildGeometryInfo,
            &primitive_count,
            &accelerationStructureBuildSizesInfo);

    topLevelAs.buffer = device.createBuffer(
            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
            VMA_MEMORY_USAGE_GPU_ONLY,
            accelerationStructureBuildSizesInfo.accelerationStructureSize
            );

    VkAccelerationStructureCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
    createInfo.buffer = topLevelAs.buffer;
    createInfo.size = accelerationStructureBuildSizesInfo.accelerationStructureSize;
    createInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    ext.vkCreateAccelerationStructureKHR(device, &createInfo, nullptr, &topLevelAs.handle);

    auto scratchBuffer = createScratchBuffer(accelerationStructureBuildSizesInfo.buildScratchSize);

    accelerationStructureBuildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    accelerationStructureBuildGeometryInfo.dstAccelerationStructure = topLevelAs.handle;
    accelerationStructureBuildGeometryInfo.scratchData.deviceAddress = scratchBuffer.deviceAddress;

    VkAccelerationStructureBuildRangeInfoKHR  accelerationStructureBuildRangeInfo{};
    accelerationStructureBuildRangeInfo.primitiveCount = 1;
    accelerationStructureBuildRangeInfo.primitiveOffset = 0;
    accelerationStructureBuildRangeInfo.firstVertex = 0;
    accelerationStructureBuildRangeInfo.transformOffset = 0;
    std::vector<VkAccelerationStructureBuildRangeInfoKHR*> accelerationBuildStructureRangeInfos = { &accelerationStructureBuildRangeInfo };

    commandPool.oneTimeCommand( [&](auto commandBuffer){
        ext.vkCmdBuildAccelerationStructuresKHR(commandBuffer, 1, &accelerationStructureBuildGeometryInfo, accelerationBuildStructureRangeInfos.data());
    });

    VkAccelerationStructureDeviceAddressInfoKHR accelerationStructureDeviceAddressInfo{};
    accelerationStructureDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
    accelerationStructureDeviceAddressInfo.accelerationStructure = topLevelAs.handle;
    topLevelAs.deviceAddress = ext.vkGetAccelerationStructureDeviceAddressKHR(device, &accelerationStructureDeviceAddressInfo);
}

void RayTracerDemo::cleanup() {
    ext.vkDestroyAccelerationStructureKHR(device, bottomLevelAS.handle, nullptr);
    ext.vkDestroyAccelerationStructureKHR(device, topLevelAs.handle, nullptr);
}

void RayTracerDemo::createShaderBindingTable() {
    assert(raytrace.pipeline);
    const uint32_t handleSize = rayTracingPipelineProperties.shaderGroupHandleSize;
    const uint32_t handleSizeAligned = alignedSize(handleSize, rayTracingPipelineProperties.shaderGroupHandleAlignment);
    const auto groupCount = COUNT(shaderGroups);
    uint32_t sbtSize = groupCount * handleSizeAligned;

    std::vector<uint8_t> shaderHandleStorage(sbtSize);

    ext.vkGetRayTracingShaderGroupHandlesKHR(device, raytrace.pipeline, 0, groupCount, sbtSize, shaderHandleStorage.data());

    const VkBufferUsageFlags usageFlags = VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    bindingTable.rayGenShader = device.createDeviceLocalBuffer(shaderHandleStorage.data(), handleSize, usageFlags);
    bindingTable.missShader = device.createDeviceLocalBuffer(shaderHandleStorage.data() + handleSizeAligned, handleSize, usageFlags);
    bindingTable.hitShader = device.createDeviceLocalBuffer(shaderHandleStorage.data() + handleSizeAligned * 2, handleSize, usageFlags);
}

void RayTracerDemo::createStorageImage() {
    auto image = initializers::imageCreateInfo(
            VK_IMAGE_TYPE_2D,
            VK_FORMAT_R32G32B32A32_SFLOAT,
              VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            swapChain.extent.width,
            swapChain.extent.height
            );
    storageImage.image = device.createImage(image);

    VkImageSubresourceRange subresourceRange{};
    subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceRange.baseMipLevel = 0;
    subresourceRange.levelCount = 1;
    subresourceRange.baseArrayLayer = 0;
    subresourceRange.layerCount = 1;
    storageImage.imageview = storageImage.image.createView(VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_VIEW_TYPE_2D, subresourceRange);
    commandPool.oneTimeCommand([&](auto commandBuffer) {
        auto barrier = initializers::ImageMemoryBarrier();
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = storageImage.image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0,
                             0, nullptr, 0, nullptr, 1, &barrier);

    });
}

void RayTracerDemo::createRayTracePipeline() {
    auto rayGenShaderModule = ShaderModule{ "../../data/shaders/raytrace_basic/raygen.rgen.spv", device };
    auto missShaderModule = ShaderModule{ "../../data/shaders/raytrace_basic/miss.rmiss.spv", device };
    auto closestHitModule = ShaderModule{ "../../data/shaders/raytrace_basic/closesthit.rchit.spv", device };

    auto stages = initializers::vertexShaderStages({
        {rayGenShaderModule, VK_SHADER_STAGE_RAYGEN_BIT_KHR},
        {missShaderModule, VK_SHADER_STAGE_MISS_BIT_KHR},
        {closestHitModule, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR}
    });

    // Ray gen group
    VkRayTracingShaderGroupCreateInfoKHR shaderGroup{};
    shaderGroup.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
    shaderGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
    shaderGroup.generalShader = shaderGroups.size();
    shaderGroup.closestHitShader = VK_SHADER_UNUSED_KHR;
    shaderGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
    shaderGroup.intersectionShader = VK_SHADER_UNUSED_KHR;
    shaderGroups.push_back(shaderGroup);

    // miss group
    shaderGroup.generalShader = shaderGroups.size();
    shaderGroups.push_back(shaderGroup);

    // closest hit group
    shaderGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
    shaderGroup.generalShader = VK_SHADER_UNUSED_KHR;
    shaderGroup.closestHitShader = shaderGroups.size();
    shaderGroups.push_back(shaderGroup);

    raytrace.layout = device.createPipelineLayout({ raytrace.descriptorSetLayout });
    VkRayTracingPipelineCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
    createInfo.stageCount = COUNT(stages);
    createInfo.pStages = stages.data();
    createInfo.groupCount = COUNT(shaderGroups);
    createInfo.pGroups = shaderGroups.data();
    createInfo.maxPipelineRayRecursionDepth = 1;
    createInfo.layout = raytrace.layout;

    VkPipeline pipeline = VK_NULL_HANDLE;
    ext.vkCreateRayTracingPipelinesKHR(device, VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &createInfo, nullptr, &pipeline);
    raytrace.pipeline = VulkanPipeline{device, pipeline};
}

void RayTracerDemo::rayTrace(VkCommandBuffer commandBuffer) {
    /*
     *  Setup buffer regions pointing to the shaders inour shader binding table;
     */

    const auto handleSizeAligned = alignedSize(rayTracingPipelineProperties.shaderGroupHandleSize, rayTracingPipelineProperties.shaderGroupHandleAlignment);
    VkStridedDeviceAddressRegionKHR  rayGenShaderSbtEntry{};
    rayGenShaderSbtEntry.deviceAddress = device.getAddress(bindingTable.rayGenShader);
    rayGenShaderSbtEntry.stride = handleSizeAligned;
    rayGenShaderSbtEntry.size = handleSizeAligned;

    VkStridedDeviceAddressRegionKHR missShaderSbtEntry{};
    missShaderSbtEntry.deviceAddress = device.getAddress(bindingTable.missShader);
    missShaderSbtEntry.stride = handleSizeAligned;
    missShaderSbtEntry.size = handleSizeAligned;

    VkStridedDeviceAddressRegionKHR hitShaderSbtEntry{};
    hitShaderSbtEntry.deviceAddress = device.getAddress(bindingTable.hitShader);
    missShaderSbtEntry.size = handleSizeAligned;

    VkStridedDeviceAddressRegionKHR callableShaderSbtEntry{};

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, raytrace.pipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, raytrace.layout, 0, 1, &raytrace.descriptorSet, 0, VK_NULL_HANDLE);

    vkCmdTraceRaysKHR(commandBuffer, &rayGenShaderSbtEntry, &missShaderSbtEntry, &hitShaderSbtEntry,
                      &callableShaderSbtEntry, swapChain.extent.width, swapChain.extent.height, 1);
}

void RayTracerDemo::loadTexture() {
    textures::fromFile(device, texture, "../../data/textures/portrait.jpg", false, VK_FORMAT_R8G8B8A8_SRGB);

}

void RayTracerDemo::createCanvasDescriptorSet() {
    canvas.descriptorSet = descriptorPool.allocate({ canvas.descriptorSetLayout }).front();

 //   std::array<VkDescriptorImageInfo, 1> imageInfo{};
//    imageInfo[0].imageView = texture.imageView;
//    imageInfo[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
//    imageInfo[0].sampler = texture.sampler;

    std::array<VkDescriptorImageInfo, 1> imageInfo{};
    imageInfo[0].imageView = storageImage.imageview;
    imageInfo[0].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    imageInfo[0].sampler = texture.sampler;

    auto writes = initializers::writeDescriptorSets<1>();
    writes[0].dstSet = canvas.descriptorSet;
    writes[0].dstBinding = 0;
    writes[0].dstArrayElement = 0;
    writes[0].descriptorCount = 1;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[0].pImageInfo = imageInfo.data();

//    VkDescriptorImageInfo computeImageInfo{};
//    computeImageInfo.imageView = compute.texture.imageView;
//    computeImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
//
//    writes[1].dstSet = compute.descriptorSet;
//    writes[1].dstBinding = 0;
//    writes[1].dstArrayElement = 0;
//    writes[1].descriptorCount = 1;
//    writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
//    writes[1].pImageInfo = &computeImageInfo;

    vkUpdateDescriptorSets(device, COUNT(writes), writes.data(), 0, nullptr);
}

void RayTracerDemo::createCanvasDescriptorSetLayout() {
    std::array<VkDescriptorSetLayoutBinding, 1> binding{};
    binding[0].binding = 0;
    binding[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    binding[0].descriptorCount = 1;
    binding[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    canvas.descriptorSetLayout = device.createDescriptorSetLayout(binding);
}

void RayTracerDemo::createCanvasPipeline() {
    auto vertexShaderModule = ShaderModule{ "../../data/shaders/quad.vert.spv", device };
    auto fragmentShaderModule = ShaderModule{ "../../data/shaders/quad.frag.spv", device };

    auto stages = initializers::vertexShaderStages({
                                                             { vertexShaderModule, VK_SHADER_STAGE_VERTEX_BIT}
                                                           , {fragmentShaderModule, VK_SHADER_STAGE_FRAGMENT_BIT}
                                                   });

    auto bindings = ClipSpace::bindingDescription();
    //   bindings.push_back({1, sizeof(glm::vec4), VK_VERTEX_INPUT_RATE_VERTEX});

    auto attributes = ClipSpace::attributeDescriptions();
    //   attributes.push_back({2, 1, VK_FORMAT_R32G32B32A32_SFLOAT, 0});

    auto vertexInputState = initializers::vertexInputState(bindings, attributes);

    auto inputAssemblyState = initializers::inputAssemblyState(ClipSpace::Quad::topology);

    auto viewport = initializers::viewport(swapChain.extent);
    auto scissor = initializers::scissor(swapChain.extent);

    auto viewportState = initializers::viewportState( viewport, scissor);

    auto rasterState = initializers::rasterizationState();
    rasterState.polygonMode = VK_POLYGON_MODE_FILL;
    rasterState.cullMode = VK_CULL_MODE_NONE;
    rasterState.frontFace = ClipSpace::Quad::frontFace;

    auto multisampleState = initializers::multisampleState();
    multisampleState.rasterizationSamples = settings.msaaSamples;

    auto depthStencilState = initializers::depthStencilState();

    auto colorBlendAttachment = std::vector<VkPipelineColorBlendAttachmentState>(1);
    colorBlendAttachment[0].colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    auto colorBlendState = initializers::colorBlendState(colorBlendAttachment);

    canvas.pipelineLayout = device.createPipelineLayout({ canvas.descriptorSetLayout});


    VkGraphicsPipelineCreateInfo createInfo = initializers::graphicsPipelineCreateInfo();
    createInfo.stageCount = COUNT(stages);
    createInfo.pStages = stages.data();
    createInfo.pVertexInputState = &vertexInputState;
    createInfo.pInputAssemblyState = &inputAssemblyState;
    createInfo.pViewportState = &viewportState;
    createInfo.pRasterizationState = &rasterState;
    createInfo.pMultisampleState = &multisampleState;
    createInfo.pDepthStencilState = &depthStencilState;
    createInfo.pColorBlendState = &colorBlendState;
    createInfo.layout = canvas.pipelineLayout;
    createInfo.renderPass = renderPass;
    createInfo.subpass = 0;

    canvas.pipeline = device.createGraphicsPipeline(createInfo);
}

void RayTracerDemo::createVertexBuffer() {
    VkDeviceSize size = sizeof(glm::vec2) * ClipSpace::Quad::positions.size();
    auto data = ClipSpace::Quad::positions;
    vertexBuffer = device.createDeviceLocalBuffer(data.data(), size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);


    std::vector<glm::vec4> colors{
            {1, 0, 0, 1},
            {0, 1, 0, 1},
            {0, 0, 1, 1},
            {1, 1, 0, 1}
    };
    size = sizeof(glm::vec4) * colors.size();
    vertexColorBuffer = device.createDeviceLocalBuffer(colors.data(), size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
}

void RayTracerDemo::loadSpaceShip() {
    phong::VulkanDrawableInfo info{};
    info.vertexUsage |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
    info.indexUsage |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
    info.materialUsage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
    info.materialIdUsage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
    info.generateMaterialId = true;
    phong::load("../../data/models/bigship1.obj", device, descriptorPool, spaceShip, info);
    spaceShipInstance.drawable = &spaceShip;
}


int main(){

    Settings settings;
    settings.depthTest = true;
    settings.deviceExtensions = {
            VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
            VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
            VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
            VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
            VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
            VK_KHR_SPIRV_1_4_EXTENSION_NAME,
            VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME
    };
    std::unique_ptr<Plugin> plugin = std::make_unique<ImGuiPlugin>();
//    auto logger = spdlog::basic_logger_mt("logger", "log.txt");
//    spdlog::set_default_logger(logger);
    try{
        auto app = RayTracerDemo{settings};
        app.addPlugin(plugin);
        app.run();
    }catch(std::runtime_error& err){
        spdlog::error(err.what());
    }
}