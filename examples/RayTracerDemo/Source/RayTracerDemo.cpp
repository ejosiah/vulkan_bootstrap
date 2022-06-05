#define DEVICE_ADDRESS_BIT
#include <ImGuiPlugin.hpp>
#include "RayTracerDemo.hpp"
#include "spdlog/sinks/basic_file_sink.h"

RayTracerDemo::RayTracerDemo(const Settings& settings): VulkanRayTraceBaseApp("Ray trace Demo", settings) {

}

void RayTracerDemo::initApp() {
    createCommandPool();
    createDescriptorPool();
    createModel();
    loadSpaceShip();
    initCamera();
    createInverseCam();
 //   createBottomLevelAccelerationStructure();
    createDescriptorSetLayouts();
    createDescriptorSets();
    createGraphicsPipeline();
    createRayTracePipeline();
    createShaderbindingTables();
    loadTexture();
}

void RayTracerDemo::createInverseCam() {
    inverseCamProj = device.createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, sizeof(glm::mat4) * 2);
}

void RayTracerDemo::onSwapChainDispose() {
    dispose(graphics.pipeline);
    dispose(raytrace.pipeline);
    descriptorPool.free({raytrace.descriptorSet});
}

void RayTracerDemo::onSwapChainRecreation() {
    canvas.recreate();
    createDescriptorSets();
    createGraphicsPipeline();
    createRayTracePipeline();
    camera->onResize(swapChain.extent.width, swapChain.extent.height);
    createShaderbindingTables();

}

VkCommandBuffer *RayTracerDemo::buildCommandBuffers(uint32_t imageIndex, uint32_t &numCommandBuffers) {
    numCommandBuffers = 1;
    auto& commandBuffer = commandBuffers[imageIndex];
    VkCommandBufferBeginInfo beginInfo = initializers::commandBufferBeginInfo();
    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    std::array<VkClearValue, 2> clearValues{{0.0f, 0.0f, 0.2f, 1.0f}};
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
        assert(graphics.pipeline);
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics.pipeline);
        camera->push(commandBuffer, graphics.layout);
        static VkDeviceSize offset = 0;
//        vkCmdBindVertexBuffers(commandBuffer, 0, 1, triangle.vertices, &offset);
//        vkCmdBindIndexBuffer(commandBuffer, triangle.indices, 0, VK_INDEX_TYPE_UINT16);
//        vkCmdDrawIndexed(commandBuffer, 3, 1, 0, 0, 0);
          drawables["spaceShip"].draw(commandBuffer, graphics.layout);

          camera->push(commandBuffer, graphics.layout, glm::mat4(1));
          drawables["plane"].draw(commandBuffer, graphics.layout);
    }else{
        canvas.draw(commandBuffer);
    }

    renderUI(commandBuffer);

    vkCmdEndRenderPass(commandBuffer);

    if(useRayTracing) {
        rayTrace(commandBuffer);
    }

    vkEndCommandBuffer(commandBuffer);

    return &commandBuffer;
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
    commandBuffers = commandPool.allocateCommandBuffers(swapChainImageCount);
}

void RayTracerDemo::createDescriptorPool() {
    auto maxSets = 1000u;
    std::vector<VkDescriptorPoolSize> poolSizes = {
            { VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1 * maxSets },
            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1  * maxSets },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1  * maxSets},
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1  * maxSets},
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 4  * maxSets}
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
    descriptorPool = device.createDescriptorPool(maxSets, poolSizes, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT);
}

void RayTracerDemo::createDescriptorSetLayouts(){
    std::vector<VkDescriptorSetLayoutBinding> bindings(3);
    bindings[0].binding = 0;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
    bindings[0].descriptorCount = 1;
    bindings[0].stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

    bindings[1].binding = 1;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    bindings[1].descriptorCount = 1;
    bindings[1].stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR;

    bindings[2].binding = 2;
    bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bindings[2].descriptorCount = 1;
    bindings[2].stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR;

    raytrace.descriptorSetLayout = device.createDescriptorSetLayout(bindings);

    // Instance bindings
    bindings.clear();
    bindings.resize(3);

    bindings[0].binding = 0;    // materials
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[0].descriptorCount = 2;
    bindings[0].stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

    bindings[1].binding = 1;    // material ids
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[1].descriptorCount = 2;
    bindings[1].stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

    bindings[2].binding = 2;  // scene objects
    bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[2].descriptorCount = 1;
    bindings[2].stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

    raytrace.instanceDescriptorSetLayout = device.createDescriptorSetLayout(bindings);

    bindings.clear();
    bindings.resize(3);
    bindings[0].binding = 0;    // vertex buffer binding
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[0].descriptorCount = 2;
    bindings[0].stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

    bindings[1].binding = 1;    // index buffer bindings
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[1].descriptorCount = 2;
    bindings[1].stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

    // vertex offset buffer
    bindings[2].binding = 2;
    bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[2].descriptorCount = 2;
    bindings[2].stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

    raytrace.vertexDescriptorSetLayout = device.createDescriptorSetLayout(bindings);

}

void RayTracerDemo::createDescriptorSets() {
    auto sets = descriptorPool.allocate({ raytrace.descriptorSetLayout, raytrace.instanceDescriptorSetLayout, raytrace.vertexDescriptorSetLayout });
    raytrace.descriptorSet = sets[0];
    raytrace.instanceDescriptorSet = sets[1];
    raytrace.vertexDescriptorSet = sets[2];

    VkDebugUtilsObjectNameInfoEXT nameInfo{};
    nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
    nameInfo.pObjectName = "raytrace_base_descriptorSet";
    nameInfo.objectType = VK_OBJECT_TYPE_DESCRIPTOR_SET;
    nameInfo.objectHandle = (uint64_t)raytrace.descriptorSet;
    vkSetDebugUtilsObjectNameEXT(device, &nameInfo);

    nameInfo.pObjectName = "raytrace_instance_descriptorSet";
    nameInfo.objectHandle = (uint64_t)raytrace.instanceDescriptorSet;
    vkSetDebugUtilsObjectNameEXT(device, &nameInfo);

    nameInfo.pObjectName = "raytrace_vertex_descriptorSet";
    nameInfo.objectHandle = (uint64_t)raytrace.vertexDescriptorSet;
    vkSetDebugUtilsObjectNameEXT(device, &nameInfo);

    VkWriteDescriptorSetAccelerationStructureKHR accWrites{};
    accWrites.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
    accWrites.accelerationStructureCount = 1;
 //   accWrites.pAccelerationStructures = &topLevelAs.handle;
    accWrites.pAccelerationStructures = rtBuilder.accelerationStructure();

    auto writes = initializers::writeDescriptorSets<9>();
    writes[0].pNext = &accWrites;
    writes[0].dstSet = raytrace.descriptorSet;
    writes[0].dstBinding = 0;
    writes[0].descriptorCount = 1;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;

    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageView = canvas.imageView;
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
//    std::vector<VkDescriptorBufferInfo> materialBufferInfos;
//    materialBufferInfos.reserve(spaceShip.meshes.size());
//
//    for(auto& mesh : spaceShip.meshes){
//        VkDescriptorBufferInfo info{};
//        info.buffer = mesh.material.materialBuffer;
//        info.offset = 0;
//        info.range = VK_WHOLE_SIZE;
//        materialBufferInfos.push_back(info);
//    }


    std::array<VkDescriptorBufferInfo, 2> materialBufferInfo{};
    materialBufferInfo[0].buffer = drawables["spaceShip"].materialBuffer;
    materialBufferInfo[0].offset = 0;
    materialBufferInfo[0].range = VK_WHOLE_SIZE;

    materialBufferInfo[1].buffer = drawables["plane"].materialBuffer;
    materialBufferInfo[1].offset = 0;
    materialBufferInfo[1].range = VK_WHOLE_SIZE;

    writes[3].dstSet = raytrace.instanceDescriptorSet;
    writes[3].dstBinding = 0;
    writes[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[3].descriptorCount = 2;
    writes[3].pBufferInfo = materialBufferInfo.data();

    // instance descriptorSet
    std::array<VkDescriptorBufferInfo, 2> matIdBufferInfo{};
    matIdBufferInfo[0].buffer = drawables["spaceShip"].materialIdBuffer;
    matIdBufferInfo[0].offset = 0;
    matIdBufferInfo[0].range = VK_WHOLE_SIZE;

    matIdBufferInfo[1].buffer = drawables["plane"].materialIdBuffer;
    matIdBufferInfo[1].offset = 0;
    matIdBufferInfo[1].range = VK_WHOLE_SIZE;

    writes[4].dstSet = raytrace.instanceDescriptorSet;
    writes[4].dstBinding = 1;
    writes[4].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[4].descriptorCount = 2;
    writes[4].pBufferInfo = matIdBufferInfo.data();

    VkDescriptorBufferInfo sceneBufferInfo{};
    sceneBufferInfo.buffer = sceneObjectBuffer;
    sceneBufferInfo.offset = 0;
    sceneBufferInfo.range = VK_WHOLE_SIZE;

    writes[5].dstSet= raytrace.instanceDescriptorSet;
    writes[5].dstBinding = 2;
    writes[5].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[5].descriptorCount = 1;
    writes[5].pBufferInfo = &sceneBufferInfo;

    // vertex descriptorSet
    std::array<VkDescriptorBufferInfo, 2> vertexBufferInfo{};
    vertexBufferInfo[0].buffer = drawables["spaceShip"].vertexBuffer;
    vertexBufferInfo[0].offset = 0;
    vertexBufferInfo[0].range = VK_WHOLE_SIZE;

    vertexBufferInfo[1].buffer = drawables["plane"].vertexBuffer;
    vertexBufferInfo[1].offset = 0;
    vertexBufferInfo[1].range = VK_WHOLE_SIZE;

    writes[6].dstSet = raytrace.vertexDescriptorSet;
    writes[6].dstBinding = 0;
    writes[6].descriptorCount = 2;
    writes[6].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[6].pBufferInfo = vertexBufferInfo.data();

    std::array<VkDescriptorBufferInfo, 2> indexBufferInfo{};
    indexBufferInfo[0].buffer = drawables["spaceShip"].indexBuffer;
    indexBufferInfo[0].offset = 0;
    indexBufferInfo[0].range = VK_WHOLE_SIZE;

    indexBufferInfo[1].buffer = drawables["plane"].indexBuffer;
    indexBufferInfo[1].offset = 0;
    indexBufferInfo[1].range = VK_WHOLE_SIZE;

    writes[7].dstSet = raytrace.vertexDescriptorSet;
    writes[7].dstBinding = 1;
    writes[7].descriptorCount = 2;
    writes[7].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[7].pBufferInfo = indexBufferInfo.data();

    std::array<VkDescriptorBufferInfo, 2> offsetBufferInfo{};
    offsetBufferInfo[0].buffer = drawables["spaceShip"].offsetBuffer;
    offsetBufferInfo[0].offset = 0;
    offsetBufferInfo[0].range = VK_WHOLE_SIZE;

    offsetBufferInfo[1].buffer = drawables["plane"].offsetBuffer;
    offsetBufferInfo[1].offset = 0;
    offsetBufferInfo[1].range = VK_WHOLE_SIZE;

    writes[8].dstSet = raytrace.vertexDescriptorSet;
    writes[8].dstBinding = 2;
    writes[8].descriptorCount = 2;
    writes[8].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[8].pBufferInfo = offsetBufferInfo.data();

    vkUpdateDescriptorSets(device, COUNT(writes), writes.data(), 0, nullptr);
}

void RayTracerDemo::initCamera() {
    OrbitingCameraSettings cameraSettings;
//    FirstPersonSpectatorCameraSettings cameraSettings;
    cameraSettings.orbitMinZoom = 0.1;
    cameraSettings.orbitMaxZoom = 512.0f;
    cameraSettings.offsetDistance = 1.0f;
    cameraSettings.modelHeight = drawables["spaceShip"].height();
    cameraSettings.fieldOfView = 60.0f;
    cameraSettings.aspectRatio = float(swapChain.extent.width)/float(swapChain.extent.height);

    camera = std::make_unique<OrbitingCameraController>(dynamic_cast<InputManager&>(*this), cameraSettings);
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
//    auto vertexShaderModule = VulkanShaderModule{"../../data/shaders/barycenter.vert.spv", device};
//    auto fragmentShaderModule = VulkanShaderModule{"../../data/shaders/barycenter.frag.spv", device};

    VulkanShaderModule vertexShaderModule = VulkanShaderModule{"../../data/shaders/demo/spaceship.vert.spv", device};
    VulkanShaderModule fragmentShaderModule = VulkanShaderModule{"../../data/shaders/demo/spaceship.frag.spv", device};

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
    graphics.layout = device.createPipelineLayout({ drawables["spaceShip"].descriptorSetLayout }, {Camera::pushConstant()});

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

void RayTracerDemo::createBottomLevelAccelerationStructure() {
//    auto res = rtBuilder.buildAs({spaceShipInstance, planeInstance});
//    sceneObjects = std::move(std::get<0>(res));
//    asInstances = std::move(std::get<1>(res));
//    VkDeviceSize size = sizeof(rt::ObjectInstance);
//    auto stagingBuffer = device.createBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY, size * sceneObjects.size());
//
//    std::vector<rt::ObjectInstance> sceneDesc;
//    sceneDesc.reserve(sceneObjects.size());
//    for(auto& instanceGroup : sceneObjects){
//        for(auto& instance : instanceGroup.objectInstances){
//            sceneDesc.push_back(instance);
//        }
//    }
//    sceneObjectBuffer = device.createDeviceLocalBuffer(sceneDesc.data(), size * sceneDesc.size(), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
}

void RayTracerDemo::cleanup() {

}

void RayTracerDemo::createShaderbindingTables() {
    assert(raytrace.pipeline);
    const auto [handleSize, handleSizeAligned] = getShaderGroupHandleSizingInfo();
    const auto groupCount = COUNT(shaderGroups);
    uint32_t sbtSize = groupCount * handleSizeAligned;

    std::vector<uint8_t> shaderHandleStorage(sbtSize);

    ext.vkGetRayTracingShaderGroupHandlesKHR(device, raytrace.pipeline, 0, groupCount, sbtSize, shaderHandleStorage.data());

    const VkBufferUsageFlags usageFlags = VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

    void* rayGenPtr = shaderHandleStorage.data();
    void* missPtr = shaderHandleStorage.data() + handleSizeAligned;
    void* hitPtr = shaderHandleStorage.data() + handleSizeAligned * 3;
    createShaderBindingTable(bindingTables.rayGenShader, rayGenPtr, usageFlags, VMA_MEMORY_USAGE_GPU_ONLY, 1);
    createShaderBindingTable(bindingTables.missShader, missPtr, usageFlags, VMA_MEMORY_USAGE_GPU_ONLY, 2);
    createShaderBindingTable(bindingTables.hitShader, hitPtr, usageFlags, VMA_MEMORY_USAGE_GPU_ONLY, 1);

}

void RayTracerDemo::createRayTracePipeline() {
    auto rayGenShaderModule = VulkanShaderModule{"../../data/shaders/raytrace_basic/raygen.rgen.spv", device };
    auto missShaderModule = VulkanShaderModule{"../../data/shaders/raytrace_basic/miss.rmiss.spv", device };
    auto shadowMissShaderModule = VulkanShaderModule{"../../data/shaders/raytrace_basic/shadow.rmiss.spv", device };
    auto closestHitModule = VulkanShaderModule{"../../data/shaders/raytrace_basic/closesthit.rchit.spv", device };

    auto stages = initializers::vertexShaderStages({
        {rayGenShaderModule, VK_SHADER_STAGE_RAYGEN_BIT_KHR},
        {missShaderModule, VK_SHADER_STAGE_MISS_BIT_KHR},
        {shadowMissShaderModule, VK_SHADER_STAGE_MISS_BIT_KHR},
        {closestHitModule, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR}
    });
    shaderGroups.clear();
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

    // shadow group
    shaderGroup.generalShader = shaderGroups.size();
    shaderGroups.push_back(shaderGroup);

    // closest hit group
    shaderGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
    shaderGroup.generalShader = VK_SHADER_UNUSED_KHR;
    shaderGroup.closestHitShader = shaderGroups.size();
    shaderGroups.push_back(shaderGroup);

    dispose(raytrace.layout);
    raytrace.layout = device.createPipelineLayout(
            { raytrace.descriptorSetLayout, raytrace.instanceDescriptorSetLayout, raytrace.vertexDescriptorSetLayout });
    VkRayTracingPipelineCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
    createInfo.stageCount = COUNT(stages);
    createInfo.pStages = stages.data();
    createInfo.groupCount = COUNT(shaderGroups);
    createInfo.pGroups = shaderGroups.data();
    createInfo.maxPipelineRayRecursionDepth = 2;
    createInfo.layout = raytrace.layout;

    VkPipeline pipeline = VK_NULL_HANDLE;
    ext.vkCreateRayTracingPipelinesKHR(device, VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &createInfo, nullptr, &pipeline);
    raytrace.pipeline = VulkanPipeline{device, pipeline};
}

void RayTracerDemo::rayTrace(VkCommandBuffer commandBuffer) {


    VkStridedDeviceAddressRegionKHR callableShaderSbtEntry{};
    CanvasToRayTraceBarrier(commandBuffer);

    std::vector<VkDescriptorSet> sets{ raytrace.descriptorSet, raytrace.instanceDescriptorSet, raytrace.vertexDescriptorSet };
    assert(raytrace.pipeline);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, raytrace.pipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, raytrace.layout, 0, COUNT(sets), sets.data(), 0, VK_NULL_HANDLE);

    vkCmdTraceRaysKHR(commandBuffer, bindingTables.rayGenShader, bindingTables.missShader, bindingTables.hitShader,
                      &callableShaderSbtEntry, swapChain.extent.width, swapChain.extent.height, 1);

    rayTraceToCanvasBarrier(commandBuffer);
}

void RayTracerDemo::loadTexture() {
    textures::fromFile(device, texture, "../../data/textures/portrait.jpg", false, VK_FORMAT_R8G8B8A8_SRGB);

}

void RayTracerDemo::loadSpaceShip() {
    phong::VulkanDrawableInfo info{};
    info.vertexUsage |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    info.indexUsage |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    info.materialUsage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    info.materialIdUsage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    info.generateMaterialId = true;

    std::vector<rt::MeshObjectInstance> instances;
    VulkanDrawable spaceShip;
    phong::load("../../data/models/bigship1.obj", device, descriptorPool, spaceShip, info, true, 1);
 //   spaceShip = VulkanDrawable::flatten(device, descriptorPool, spaceShip.descriptorSetLayout, spaceShip, 0, 0, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    drawables.insert(std::make_pair("spaceShip", std::move(spaceShip)));
  //  phong::load(R"(C:\Users\Josiah Ebhomenye\OneDrive\media\models\ChineseDragon.obj)", device, descriptorPool, spaceShip, info);
   // phong::load(R"(C:\Users\Josiah Ebhomenye\OneDrive\media\models\Lucy-statue\metallic-lucy-statue-stanford-scan.obj)", device, descriptorPool, spaceShip, info, true, 1);
   // phong::load(R"(C:\Users\Josiah Ebhomenye\OneDrive\media\models\werewolf.obj)", device, descriptorPool, spaceShip, info);
    rt::MeshObjectInstance spaceShipInstance{};
    spaceShipInstance.object =  rt::TriangleMesh{ &drawables["spaceShip"] };
    spaceShipInstance.xform = glm::translate(glm::mat4{1}, {0, drawables["spaceShip"].height() * 0.5f, 0});
    spaceShipInstance.xformIT = glm::inverseTranspose(spaceShipInstance.xform);
    instances.push_back(spaceShipInstance);

    spaceShipInstance.xform = glm::translate(glm::mat4(1), {-2, drawables["spaceShip"].height() * 0.5f, 0});
    instances.push_back(spaceShipInstance);

    spaceShipInstance.xform = glm::translate(glm::mat4(1), {2, drawables["spaceShip"].height() * 0.5f, 0});
    instances.push_back(spaceShipInstance);

    spaceShipInstance.xform = glm::translate(glm::mat4(1), {0, drawables["spaceShip"].height() * 0.5f, 2});
    instances.push_back(spaceShipInstance);

    spaceShipInstance.xform = glm::translate(glm::mat4(1), {0, drawables["spaceShip"].height() * 0.5f, -2});
    instances.push_back(spaceShipInstance);


    VulkanDrawable plane_l;
    phong::load("../../data/models/plane.gltf", device, descriptorPool, plane_l,  info);
    drawables.insert(std::make_pair("plane", std::move(plane_l)));
//    phong::load(R"(C:\Users\Josiah Ebhomenye\OneDrive\media\models\Lucy-statue\metallic-lucy-statue-stanford-scan.obj)", device, descriptorPool, plane,  info, true, 1);
 //   phong::load("../../data/models/bigship1.obj", device, descriptorPool, plane,  info, true, 1);
    rt::MeshObjectInstance planeInstance{};
    planeInstance.object = rt::TriangleMesh{ &drawables["plane"] };
    instances.push_back(planeInstance);
    createAccelerationStructure(instances);
}

void RayTracerDemo::CanvasToRayTraceBarrier(VkCommandBuffer commandBuffer) const {
    VkImageMemoryBarrier barrier = initializers::ImageMemoryBarrier();
    barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
    barrier.image = canvas.image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;

    VkMemoryBarrier mBarrier{};
    mBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    mBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    mBarrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;

    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                         VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
                         0,
                         0,
                         VK_NULL_HANDLE,
                         0,
                         VK_NULL_HANDLE,
                         1,
                         &barrier);
}

void RayTracerDemo::rayTraceToCanvasBarrier(VkCommandBuffer commandBuffer) const {
    VkImageMemoryBarrier barrier = initializers::ImageMemoryBarrier();
    barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
    barrier.image = canvas.image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;


    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
                         VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                         0,

                         0,
                         VK_NULL_HANDLE,
                         0,
                         VK_NULL_HANDLE,
                         1,
                         &barrier);
}


int main(){

    Settings settings;
    settings.depthTest = true;
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