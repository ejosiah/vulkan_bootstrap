#include <BulletPhysicsDemo.hpp>
#include "BulletPhysicsDemo.hpp"
#include "BulletPhysicsPlugin.hpp"
#include "ImGuiPlugin.hpp"
#include "GraphicsPipelineBuilder.hpp"
#include "glm_bullet_interpreter.hpp"

BulletPhysicsDemo::BulletPhysicsDemo(const Settings& settings) : VulkanBaseApp("bullet physics demo", settings) {
    fileManager.addSearchPath(".");
    fileManager.addSearchPath("../../examples/bulletphysicsdemo");
    fileManager.addSearchPath("../../data/shaders");
    fileManager.addSearchPath("../../data");
    fileManager.addSearchPath("../../data/models");
    fileManager.addSearchPath("../../data/textures");
}

void BulletPhysicsDemo::initApp() {
    SkyBox::init(this);
    createCommandPool();
    createDescriptorPool();
    createDescriptorSetLayouts();
    accStructBuilder = rt::AccelerationStructureBuilder{&device};
    initCamera();
    createSkyBox();
    createCubes();
    createRigidBodies();
    createPipelineCache();
    createRenderPipeline();
    createComputePipeline();
}

void BulletPhysicsDemo::createDescriptorPool() {
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

void BulletPhysicsDemo::createCommandPool() {
    commandPool = device.createCommandPool(*device.queueFamilyIndex.graphics, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    commandBuffers = commandPool.allocateCommandBuffers(swapChainImageCount);
}

void BulletPhysicsDemo::createPipelineCache() {
    pipelineCache = device.createPipelineCache();
}


void BulletPhysicsDemo::createRenderPipeline() {
    auto builder = device.graphicsPipelineBuilder();
    render.pipeline =
        builder
            .shaderStage()
                .vertexShader(load("spv/cube.vert.spv"))
                .fragmentShader(load("spv/cube.frag.spv"))
            .vertexInputState()
                .addVertexBindingDescription(0, sizeof(VertexData), VK_VERTEX_INPUT_RATE_VERTEX)
                .addVertexBindingDescription(1, sizeof(VertexInstanceData), VK_VERTEX_INPUT_RATE_INSTANCE)
                .addVertexAttributeDescription(0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetOf(VertexData, position))
                .addVertexAttributeDescription(1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetOf(VertexData, normal))
                .addVertexAttributeDescription(2, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetOf(VertexInstanceData, color))
                .addVertexAttributeDescription(3, 1, VK_FORMAT_R32G32B32A32_SFLOAT, offsetOf(VertexInstanceData, xform))
                .addVertexAttributeDescription(4, 1, VK_FORMAT_R32G32B32A32_SFLOAT, offsetOf(VertexInstanceData, xform) + 16)
                .addVertexAttributeDescription(5, 1, VK_FORMAT_R32G32B32A32_SFLOAT, offsetOf(VertexInstanceData, xform) + 32)
                .addVertexAttributeDescription(6, 1, VK_FORMAT_R32G32B32A32_SFLOAT, offsetOf(VertexInstanceData, xform) + 48)
            .inputAssemblyState()
                .triangles()
            .viewportState()
                .viewport()
                    .origin(0, 0)
                    .dimension(swapChain.extent)
                    .minDepth(0)
                    .maxDepth(1)
                .scissor()
                    .offset(0, 0)
                    .extent(swapChain.extent)
                .add()
            .rasterizationState()
                .cullBackFace()
                .frontFaceCounterClockwise()
                .polygonModeFill()
            .multisampleState()
                .rasterizationSamples(settings.msaaSamples)
            .depthStencilState()
                .enableDepthTest()
                .enableDepthWrite()
                .compareOpLess()
                .minDepthBounds(0)
                .maxDepthBounds(1)
            .colorBlendState()
                .attachment()
                .add()
            .layout()
                .addDescriptorSetLayout(accStructDescriptorSetLayout)
                .addPushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Camera) + sizeof(glm::vec3))
            .renderPass(renderPass)
            .subpass(0)
            .name("render_cube")
            .allowDerivatives()
        .build(render.layout);

    floor.pipeline =
        builder
            .setDerivatives()
            .basePipeline(render.pipeline)
            .shaderStage()
                .clear()
                .vertexShader(load("/spv/floor.vert.spv"))
                .fragmentShader(load("spv/floor.frag.spv"))
            .vertexInputState()
                .clear()
                .addVertexBindingDescription(0, sizeof(FloorVertexData), VK_VERTEX_INPUT_RATE_VERTEX)
                .addVertexAttributeDescription(0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(FloorVertexData, position))
                .addVertexAttributeDescription(1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(FloorVertexData, normal))
                .addVertexAttributeDescription(2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(FloorVertexData, uv))
            .name("floor")
        .build(floor.layout);
}

void BulletPhysicsDemo::createComputePipeline() {
    auto module = VulkanShaderModule{ "../../data/shaders/pass_through.comp.spv", device};
    auto stage = initializers::shaderStage({ module, VK_SHADER_STAGE_COMPUTE_BIT});

    compute.layout = device.createPipelineLayout();

    auto computeCreateInfo = initializers::computePipelineCreateInfo();
    computeCreateInfo.stage = stage;
    computeCreateInfo.layout = compute.layout;

    compute.pipeline = device.createComputePipeline(computeCreateInfo, pipelineCache);
}


void BulletPhysicsDemo::onSwapChainDispose() {
    dispose(render.pipeline);
    dispose(compute.pipeline);
}

void BulletPhysicsDemo::onSwapChainRecreation() {
    createRenderPipeline();
    createComputePipeline();
}

VkCommandBuffer *BulletPhysicsDemo::buildCommandBuffers(uint32_t imageIndex, uint32_t &numCommandBuffers) {
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

    auto& bullet = plugin<BulletPhysicsPlugin>(BULLET_PHYSICS_PLUGIN);
    drawFloor(commandBuffer);
    drawCubes(commandBuffer);
    bullet.set(cameraController->cam());
//    bullet.draw(commandBuffer);
    drawSkyBox(commandBuffer);
    displayInfo(commandBuffer);

    vkCmdEndRenderPass(commandBuffer);

    vkEndCommandBuffer(commandBuffer);

    return &commandBuffer;
}

void BulletPhysicsDemo::update(float time) {

//    if(totalFrames) {
//        calculateCameraShade();
//    }

    auto text = fmt::format("{} - {} fps", title, framePerSecond);
    glfwSetWindowTitle(window, text.c_str());
    cameraController->update(time);

    auto& bullet = plugin<BulletPhysicsPlugin>(BULLET_PHYSICS_PLUGIN);
    cubes.instanceBuffer.map<VertexInstanceData>([&](VertexInstanceData* ptr){
        int next = 0;
       for(auto& id : cubes.ids){
           auto xform = bullet.getTransform(id);
           auto& instance = ptr[next++];
           instance.xform = glm::translate(glm::mat4(1), xform.origin) * glm::mat4(xform.basis);
           asInstances[id].xform = instance.xform;
       }
    });
    accStructBuilder.updateTlas(asInstances);
}

void BulletPhysicsDemo::checkAppInputs() {
    cameraController->processInput();
}

void BulletPhysicsDemo::cleanup() {
    // TODO save pipeline cache
    VulkanBaseApp::cleanup();
}

void BulletPhysicsDemo::onPause() {
    VulkanBaseApp::onPause();
}

void BulletPhysicsDemo::initCamera() {
    CameraSettings settings{};
    settings.aspectRatio = static_cast<float>(swapChain.extent.width)/static_cast<float>(swapChain.extent.height);
    settings.orbit.orbitMinZoom = CAMERA_ZOOM_MIN;
    settings.orbit.orbitMaxZoom = CAMERA_ZOOM_MAX;
    settings.acceleration = CAMERA_ACCELERATION;
    settings.velocity = CAMERA_VELOCITY;
    settings.orbit.offsetDistance = CAMERA_ZOOM_MIN + (CAMERA_ZOOM_MAX - CAMERA_ZOOM_MIN) * 0.3F;
    settings.rotationSpeed = 0.1f;
    settings.fieldOfView = CAMERA_FOVX;
    settings.zNear = CAMERA_ZNEAR;
    settings.zFar = CAMERA_ZFAR;
    settings.horizontalFov = true;
    settings.orbit.modelHeight = 0.5;
    cameraController = std::make_unique<CameraController>(device, swapChain.imageCount(), currentImageIndex, dynamic_cast<InputManager&>(*this), settings);
    cameraController->setMode(CameraMode::SPECTATOR);
    cameraController->lookAt({3, 7, 6}, {0, 0, 0}, {0, 1, 0});
    auto& bullet = plugin<BulletPhysicsPlugin>(BULLET_PHYSICS_PLUGIN);
}

void BulletPhysicsDemo::createRigidBodies() {
    auto& bullet = plugin<BulletPhysicsPlugin>(BULLET_PHYSICS_PLUGIN);

    RigidBody body;
    body.shape = new btBoxShape(btVector3(50.f, 50.f, 50.f));
    body.xform.basis = glm::mat3{1};
    body.xform.origin = glm::vec3(0, -50, 0);
    body.mass = 0.0;

    body = bullet.addRigidBody(body);
    std::vector<RigidBody> rigidBodies;
    rigidBodies.push_back(body);

    auto boxShape = new btBoxShape(btVector3{.1, .1, .1});
    cubes.instanceBuffer.map<VertexInstanceData>([&](auto instancePtr){
        for(auto i = 0; i < cubes.numBoxes; i++){
            auto instance = instancePtr[i];
            RigidBody box{};
            box.shape = boxShape;
            box.xform.origin = (instance.xform * glm::vec4{0, 0, 0, 1}).xyz();
            box.mass = 1.f;
            box = bullet.addRigidBody(box);
            cubes.ids.push_back(box.id);
            rigidBodies.push_back(box);
        }
    });

    createAccelerationStructure(rigidBodies);

//    std::vector<mesh::Mesh> meshes;
//    mesh::load(meshes, R"(C:\Users\Josiah\OneDrive\media\models\werewolf.obj)");
//    mesh::normalize(meshes, 1.0);
//
//    glm::vec3 vMin, vMax;
//    mesh::bounds(meshes, vMin, vMax);
//    mesh::transform(meshes, glm::translate(glm::mat4(1), {0, -vMin.y, 0}));
//    mesh::bounds(meshes, vMin, vMax);
//
//    std::vector<btVector3> vertices;
//    vertices.reserve(meshes.size());
//    for(auto& mesh : meshes){
//        for(auto& vertex : mesh.vertices){
//            auto pos = vertex.position.xyz();
//            vertices.push_back(to_btVector3(pos));
//        }
//    }
//    auto convexHull = new btConvexHullShape{reinterpret_cast<btScalar*>(vertices.data()), int(vertices.size())};
//
//    body.shape = convexHull;
//    body.xform.origin = (vMin + vMax) * 0.5f;
//    body.mass = 0.0;
//    bullet.addRigidBody(body);
}

void BulletPhysicsDemo::createAccelerationStructure(const std::vector<RigidBody>& rigidBodies) {
    imp::Box box{glm::vec3(-0.1), glm::vec3(0.1)};

    rt::ImplicitObject boxObj;
    boxObj.numObjects = 1;
    boxObj.hitGroupId = 0;
    boxObj.aabbBuffer =
            device.createCpuVisibleBuffer(&box, sizeof(box)
                    , VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR
                      | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);
    auto blasId = accStructBuilder.buildBlas({ &boxObj },
                                             VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_BUILD_BIT_KHR
                                             | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR ).front();

    for(auto i = 0u; i < rigidBodies.size(); i++){
        auto& body = rigidBodies[i];
        rt::Instance instance{ blasId, i};
        instance.xform = glm::translate(glm::mat4(1), body.xform.origin) * glm::mat4(body.xform.basis);
        accStructBuilder.add(instance);
    }
    asInstances = accStructBuilder.buildTlas(VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR);
    createDescriptorSets();
    updateAccelerationStructureDescriptorSet();
}

void BulletPhysicsDemo::displayInfo(VkCommandBuffer commandBuffer) {
    auto& imgui = plugin<ImGuiPlugin>(IM_GUI_PLUGIN);
    auto font = imgui.font("Arial", 15);
    ImGui::Begin("info", nullptr, IMGUI_NO_WINDOW);
    ImGui::SetWindowSize({ 500, float(height)});
    ImGui::PushFont(font);

    auto camPos = fmt::format("camera position: {}", cameraController->position());
    ImGui::TextColored({0, 0, 0, 1}, camPos.c_str());
    ImGui::PopFont();
    ImGui::End();
    imgui.draw(commandBuffer);
}

void BulletPhysicsDemo::createSkyBox() {
    SkyBox::create(skyBox, R"(C:\Users\Josiah Ebhomenye\OneDrive\media\textures\skybox\005)"
                   , {"right.jpg", "left.jpg", "top.jpg", "bottom.jpg", "front.jpg", "back.jpg"});
}

void BulletPhysicsDemo::createCubes() {

    auto cube = primitives::cube();
    std::vector<VertexData> vertices;
    std::vector<FloorVertexData> floorVertices;
    vertices.reserve(cube.vertices.size());
    for(auto& vertex : cube.vertices){
        glm::vec3 pos = (glm::scale(glm::mat4(1), glm::vec3(0.2)) * vertex.position).xyz();
        VertexData data{ pos, vertex.normal};
        vertices.push_back(data);

        FloorVertexData floorData{ vertex.position.xyz(), vertex.normal, vertex.uv};
        floorVertices.push_back(floorData);
    }
    cubes.vertexBuffer = device.createDeviceLocalBuffer(vertices.data(), BYTE_SIZE(vertices), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    cubes.indexBuffer = device.createDeviceLocalBuffer(cube.indices.data(), BYTE_SIZE(cube.indices), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);

    std::vector<VertexInstanceData> instances;
    auto rng = rngFunc(0.2, 1.0, 1 << 20);
    int n = 5;
    for(auto k = 0; k < n; k++){
        for(auto i = 0; i < n; i++){
            for(auto j = 0; j < n; j++){
                glm::vec3 origin = {0.2f * float(i), 2.f + .2 * float(k), 0.2f * float(j)};
               VertexInstanceData instance{};
               instance.xform = glm::translate(glm::mat4(1), origin);
               instance.color = {rng(), rng(), rng()};
                glm::vec3 t = {float(i)/float(n), float(k)/float(n), float(j)/float(n)};
               instance.color = trilerp(glm::vec3(0), {1, 0, 0}, {0, 1, 0}, {1, 1, 0},
                                        {0, 0, 1}, {1, 0, 1}, {0, 1, 1}, glm::vec3(1), t);
               instances.push_back(instance);
            }
        }
    }
    cubes.instanceBuffer = device.createCpuVisibleBuffer(instances.data(), BYTE_SIZE(instances), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);

    floor.vertexBuffer = device.createDeviceLocalBuffer(floorVertices.data(), BYTE_SIZE(floorVertices), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    floor.indexBuffer = device.createDeviceLocalBuffer(cube.indices.data(), BYTE_SIZE(cube.indices), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
}

void BulletPhysicsDemo::drawCubes(VkCommandBuffer commandBuffer) {
    static std::array<VkBuffer, 2> buffers;
    buffers[0] = cubes.vertexBuffer;
    buffers[1] = cubes.instanceBuffer;

    uint32_t instanceCount = cubes.instanceBuffer.size / sizeof(VertexInstanceData);
    uint32_t indexCount = cubes.indexBuffer.size / sizeof(uint32_t);
    static std::array<VkDeviceSize, 2> offsets{0, 0};

    vkCmdBindVertexBuffers(commandBuffer, 0, COUNT(buffers), buffers.data(), offsets.data());
    vkCmdBindIndexBuffer(commandBuffer, cubes.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, render.pipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, render.layout, 0, 1, &accStructDescriptorSet, 0, VK_NULL_HANDLE);
    cameraController->push(commandBuffer, render.layout, glm::mat4(1));
    vkCmdPushConstants(commandBuffer, render.layout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(Camera), sizeof(glm::vec3), &lightDir);
    vkCmdDrawIndexed(commandBuffer, indexCount, instanceCount, 0, 0, 0);
}

void BulletPhysicsDemo::drawFloor(VkCommandBuffer commandBuffer) {
    uint32_t indexCount = floor.indexBuffer.size / sizeof(uint32_t);
    VkDeviceSize offset = 0;

    static glm::mat4 model = glm::translate(glm::mat4(1), {0, -50, 0}) * glm::scale(glm::mat4(1), glm::vec3(100));

    vkCmdBindVertexBuffers(commandBuffer, 0, 1, floor.vertexBuffer, &offset);
    vkCmdBindIndexBuffer(commandBuffer, floor.indexBuffer, offset, VK_INDEX_TYPE_UINT32);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, floor.pipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, floor.layout, 0, 1, &accStructDescriptorSet, 0, VK_NULL_HANDLE);

    cameraController->push(commandBuffer, floor.layout, model);
    vkCmdPushConstants(commandBuffer, render.layout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(Camera), sizeof(glm::vec3), &lightDir);
    vkCmdDrawIndexed(commandBuffer, indexCount, 1, 0, 0, 0);
}

void BulletPhysicsDemo::drawSkyBox(VkCommandBuffer commandBuffer) {
    static glm::mat4 model{1};

    uint32_t indexCount = skyBox.cube.indices.size /sizeof(int32_t);
    uint32_t instanceCount = 1;
    int32_t vertexOffset = 0;
    uint32_t firstInstance = 0;
    VkDeviceSize offset = 0;

    vkCmdBindVertexBuffers(commandBuffer, 0, 1, skyBox.cube.vertices, &offset);
    vkCmdBindIndexBuffer(commandBuffer, skyBox.cube.indices, 0, VK_INDEX_TYPE_UINT32);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *skyBox.pipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *skyBox.layout, 0, 1, &skyBox.descriptorSet, 0, VK_NULL_HANDLE);
    cameraController->push(commandBuffer, *skyBox.layout, model);
    vkCmdDrawIndexed(commandBuffer, indexCount, instanceCount, firstInstance, vertexOffset, firstInstance);

}

void BulletPhysicsDemo::calculateCameraShade() {
    auto& bullet = plugin<BulletPhysicsPlugin>(BULLET_PHYSICS_PLUGIN);
    const auto& world = bullet.dynamicsWorld();

    auto numObjects = world.getNumCollisionObjects();
    auto objects = world.getCollisionObjectArray();
    for(auto i = 0; i < numObjects - 1; i++){
        auto objA = objects[i];
        for(auto j = i+1; j < numObjects; j++){
            auto objB = objects[j];
            if(objA->checkCollideWith(objB)){
                auto bodyA = btRigidBody::upcast(objA);
                auto bodyB = btRigidBody::upcast(objB);
                Transform xformA = bullet.getTransform(i);
                Transform xformB = bullet.getTransform(j);

                glm::vec3 n = glm::normalize(xformA.origin - xformB.origin);
                glm::vec3 vA = to_vec3(bodyA->getLinearVelocity());
                auto vB = to_vec3(bodyB->getLinearVelocity());
                glm::vec3 v =  vA - vB;
                bool moving = glm::all(glm::greaterThan(v, glm::vec3(0)));
                if(moving && glm::dot(n, v) > 0){
                    spdlog::info("object[{}] and object[{}] separating", i, j);
                }
            }
        }
    }
}

void BulletPhysicsDemo::createDescriptorSetLayouts() {
    accStructDescriptorSetLayout =
        device.descriptorSetLayoutBuilder()
            .binding(0)
                .descriptorType(VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR)
                .descriptorCount(1)
                .shaderStages(VK_SHADER_STAGE_FRAGMENT_BIT)
            .createLayout();
}

void BulletPhysicsDemo::createDescriptorSets(){
    accStructDescriptorSet = descriptorPool.allocate({ accStructDescriptorSetLayout }).front();
}

void BulletPhysicsDemo::updateAccelerationStructureDescriptorSet() {

    auto accWrites = VkWriteDescriptorSetAccelerationStructureKHR{};
    accWrites.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
    accWrites.accelerationStructureCount = 1;
    accWrites.pAccelerationStructures = accStructBuilder.accelerationStructure();

    auto writes = initializers::writeDescriptorSets<1>();
    writes[0].dstSet = accStructDescriptorSet;
    writes[0].dstBinding = 0;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
    writes[0].descriptorCount = 1;
    writes[0].pNext = &accWrites;

    device.updateDescriptorSets(writes);
}


int main(){
    try{

        Settings settings;
        settings.depthTest = true;
        settings.enabledFeatures.wideLines = true;
        settings.enabledFeatures.fillModeNonSolid = true;
        settings.msaaSamples = VK_SAMPLE_COUNT_8_BIT;

        auto app = BulletPhysicsDemo{ settings };

        BulletPhysicsPluginInfo info{};
        info.gravity = {0, -0.9807, 0};
        info.debugMode = btIDebugDraw::DebugDrawModes::DBG_DrawWireframe;
        info.timeStep = INV_120_HERTZ;
        info.maxSubSteps = 10;
        std::unique_ptr<Plugin> bullet = std::make_unique<BulletPhysicsPlugin>(info);

        std::vector<FontInfo> fonts {
#ifdef WIN32
                {"JetBrainsMono", R"(D:\Users\Josiah\Downloads\JetBrainsMono-2.225\fonts\ttf\JetBrainsMono-Regular.ttf)", 20},
                {"Arial", R"(C:\Windows\Fonts\arial.ttf)", 20},
                {"Arial", R"(C:\Windows\Fonts\arial.ttf)", 15}
#elif defined(__APPLE__)
                {"Arial", "/Library/Fonts/Arial Unicode.ttf", 20},
                {"Arial", "/Library/Fonts/Arial Unicode.ttf", 15}
#endif
        };
        std::unique_ptr<Plugin> imGui = std::make_unique<ImGuiPlugin>(fonts);

        app.addPlugin(bullet);
        app.addPlugin(imGui);
        app.run();
    }catch(std::runtime_error& err){
        spdlog::error(err.what());
    }
}