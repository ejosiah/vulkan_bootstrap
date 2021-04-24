#include "ClothDemo.hpp"
#include "primitives.h"

ClothDemo::ClothDemo(const Settings &settings) : VulkanBaseApp("Cloth", settings) {

}

void ClothDemo::initApp() {
    checkInvariants();
    commandPool = device.createCommandPool(*device.queueFamilyIndex.graphics, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    commandBuffers = commandPool.allocate(swapChainImageCount);
    createCloth();
    createSphere();
    createFloor();
    initCamera();
    createPositionDescriptorSetLayout();
    createDescriptorPool();
    createPositionDescriptorSet();
    createPipelines();
    createComputePipeline();
}

void ClothDemo::checkInvariants() {
    auto pcLimit = device.getLimits().maxPushConstantsSize;
    assert(sizeof(constants) <= pcLimit);
}

void ClothDemo::createCloth() {
    auto xform = glm::translate(glm::mat4(1), {0, 60, 0}) *  glm::rotate(glm::mat4(1), -glm::half_pi<float>(), {1, 0, 0});
    auto plane = primitives::plane(cloth.gridSize.x - 1, cloth.gridSize.y - 1, cloth.size.x, cloth.size.y, xform, glm::vec4(1));

    cloth.vertices[0] = device.createCpuVisibleBuffer(plane.vertices.data(), sizeof(Vertex) * plane.vertices.size()
                                                    , VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    cloth.vertices[1] = device.createCpuVisibleBuffer(plane.vertices.data(), sizeof(Vertex) * plane.vertices.size()
                                                    , VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    ;
    cloth.vertexCount = plane.vertices.size();

    cloth.indices = device.createDeviceLocalBuffer(plane.indices.data(), sizeof(uint32_t) * plane.indices.size(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    cloth.indexCount = plane.indices.size();
    constants.inv_cloth_size = cloth.size/(cloth.gridSize - glm::vec2(1));
}

void ClothDemo::createSphere(){
    auto& xform = sphere.ubo.xform;
    xform = glm::translate(glm::mat4(1), {0, sphere.radius * 1.2, 0});
    xform = glm::scale(xform, glm::vec3(sphere.radius));
    auto s = primitives::sphere(25, 25, 1.0f, xform,  {1, 1, 0, 1});
    sphere.vertices = device.createDeviceLocalBuffer(s.vertices.data(), sizeof(Vertex) * s.vertices.size(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    sphere.indices = device.createDeviceLocalBuffer(s.indices.data(), sizeof(uint32_t) * s.indices.size(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    sphere.indexCount = s.indices.size();

    sphere.uboBuffer = device.createDeviceLocalBuffer(&sphere.ubo, sizeof(sphere.ubo), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
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
}

void ClothDemo::onSwapChainRecreation() {
    createPipelines();
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

    VkDeviceSize offset = 0;
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.wireframe);
    cameraController->push(commandBuffer, pipelineLayouts.wireframe);
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, floor.vertices, &offset);
    vkCmdBindIndexBuffer(commandBuffer, floor.indices, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(commandBuffer, floor.indexCount, 1, 0, 0, 0);

    vkCmdBindVertexBuffers(commandBuffer, 0, 1, sphere.vertices, &offset);
    vkCmdBindIndexBuffer(commandBuffer, sphere.indices, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(commandBuffer, sphere.indexCount, 1, 0, 0, 0);

    static std::array<VkDeviceSize, 2> offsets{0u, 0u};
    static std::array<VkBuffer, 2> buffers{cloth.vertices[output_index], cloth.vertexAttributes };
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers.data(), offsets.data());
    vkCmdBindIndexBuffer(commandBuffer, cloth.indices, 0, VK_INDEX_TYPE_UINT32);

    vkCmdDrawIndexed(commandBuffer, cloth.indexCount, 1, 0, 0, 0);


    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.point);
    cameraController->push(commandBuffer, pipelineLayouts.point);

    static glm::vec4 pointColor{1, 0, 0, 1};
    vkCmdPushConstants(commandBuffer, pipelineLayouts.point, VK_SHADER_STAGE_VERTEX_BIT, sizeof(Camera), sizeof(glm::vec4), &pointColor[0]);
    vkCmdDraw(commandBuffer, cloth.vertexCount, 1, 0, 0);

    static glm::vec4 normalColor{1, 1, 0, 1};
    static float normalLength = glm::length(constants.inv_cloth_size) * 0.5f;
    static std::array<char, sizeof(normalColor) + sizeof(normalLength)> normalConstants{};
    std::memcpy(normalConstants.data(), &normalColor[0], sizeof(normalColor));
    std::memcpy(normalConstants.data() + sizeof(normalColor), &normalLength, sizeof(normalLength));
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.normals);
    cameraController->push(commandBuffer, pipelineLayouts.normals, VK_SHADER_STAGE_GEOMETRY_BIT);
    vkCmdPushConstants(commandBuffer, pipelineLayouts.normals, VK_SHADER_STAGE_GEOMETRY_BIT, sizeof(Camera), normalConstants.size(), normalConstants.data());
    vkCmdDraw(commandBuffer, cloth.vertexCount, 1, 0, 0);

    renderUI(commandBuffer);

    vkCmdEndRenderPass(commandBuffer);
    vkEndCommandBuffer(commandBuffer);

    return cmdBuffers.data();
}

void ClothDemo::update(float time) {
    cameraController->update(time);
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
    cameraController->lookAt({0, settings.floorOffset, 20}, glm::vec3(0), {0, 1, 0});
}

void ClothDemo::createPipelines() {
    auto flatVertexModule = ShaderModule{ "../../data/shaders/flat.vert.spv", device};
    auto flatFragmentModule = ShaderModule{ "../../data/shaders/flat.frag.spv", device};

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


    auto pointVertexShaderModule = ShaderModule{ "../../data/shaders/point.vert.spv", device };
    auto pointFragShaderModule = ShaderModule{ "../../data/shaders/point.frag.spv", device };
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

    auto normalVertexShaderModule = ShaderModule{ "../../data/shaders/draw_normals.vert.spv", device};
    auto normalGeometryShaderModule = ShaderModule{"../../data/shaders/draw_normals.geom.spv", device};

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
}

void ClothDemo::createPositionDescriptorSetLayout() {
    std::array<VkDescriptorSetLayoutBinding, 1> bindings{};
    bindings[0].binding = 0;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[0].descriptorCount = 1;
    bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    positionSetLayout = device.createDescriptorSetLayout(bindings);

    bindings[0].binding = 0;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bindings[0].descriptorCount = 1;
    bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    collision.setLayout = device.createDescriptorSetLayout(bindings);
}

void ClothDemo::createDescriptorPool() {
    std::array<VkDescriptorPoolSize, 2> poolSizes{};
    poolSizes[0].descriptorCount = 2;
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

    poolSizes[1].descriptorCount = 1;
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

    uint32_t maxSet = 3;

    descriptorPool = device.createDescriptorPool(maxSet, poolSizes);
}

void ClothDemo::createPositionDescriptorSet() {
    assert(cloth.vertices[0].buffer != VK_NULL_HANDLE && cloth.vertices[1].buffer != VK_NULL_HANDLE);


    auto descriptorSets = descriptorPool.allocate({ positionSetLayout, positionSetLayout, collision.setLayout});
    positionDescriptorSets[0] = descriptorSets[0];
    positionDescriptorSets[1] = descriptorSets[1];
    collision.descriptorSet = descriptorSets[2];

    std::array<VkDescriptorBufferInfo, 3> bufferInfo{};
    bufferInfo[0].buffer = cloth.vertices[0];
    bufferInfo[0].offset = 0;
    bufferInfo[0].range = VK_WHOLE_SIZE;

    bufferInfo[1].buffer = cloth.vertices[1];
    bufferInfo[1].offset = 0;
    bufferInfo[1].range = VK_WHOLE_SIZE;

    bufferInfo[2].buffer = sphere.uboBuffer;
    bufferInfo[2].offset = 0;
    bufferInfo[2].range = VK_WHOLE_SIZE;

    auto writes = initializers::writeDescriptorSets<3>();
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

    vkUpdateDescriptorSets(device, COUNT(writes), writes.data(), 0, VK_NULL_HANDLE);
}

void ClothDemo::createComputePipeline() {
    auto computeModule = ShaderModule{ "../../data/shaders/cloth/cloth.comp.spv", device };

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

    commandPool.oneTimeCommand(device.queues.compute, [&](auto commandBuffer){
        static std::array<VkDescriptorSet, 3> descriptors{};
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelines.compute);
        vkCmdPushConstants(commandBuffer, pipelineLayouts.compute, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(constants), &constants);
        for(auto i = 0; i < numIterations; i++) {
            descriptors[0] = positionDescriptorSets[input_index];
            descriptors[1] = positionDescriptorSets[output_index];
            descriptors[2] = collision.descriptorSet;
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayouts.compute, 0, COUNT(descriptors), descriptors.data(), 0,
                                    nullptr);
            vkCmdDispatch(commandBuffer, 1, 1, 1);

            if(i - 1 < numIterations){
                computeToComputeBarrier(commandBuffer);
            }
            std::swap(input_index, output_index);

        }
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
    ImGui::SetWindowSize("Cloth Simulation", {350, 70});
    ImGui::Text("%d iteration(s), timeStep: %.3f ms", numIterations, frameTime * 1000);
    ImGui::Text("Application average %.3f ms/frame, (%d FPS)", 1000.0/framePerSecond, framePerSecond);
    ImGui::End();

    imGuiPlugin.draw(commandBuffer);
}

int main(){
    Settings settings;
    settings.vSync = true;
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