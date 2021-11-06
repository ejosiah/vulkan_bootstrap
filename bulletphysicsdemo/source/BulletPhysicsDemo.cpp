#include "BulletPhysicsDemo.hpp"
#include "BulletPhysicsPlugin.hpp"
#include "ImGuiPlugin.hpp"

BulletPhysicsDemo::BulletPhysicsDemo(const Settings& settings) : VulkanBaseApp("bullet physics demo", settings) {

}

void BulletPhysicsDemo::initApp() {
    initCamera();
    createRigidBodies();
    createDescriptorPool();
    createCommandPool();
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
    commandBuffers = commandPool.allocate(swapChainImageCount);
}

void BulletPhysicsDemo::createPipelineCache() {
    pipelineCache = device.createPipelineCache();
}


void BulletPhysicsDemo::createRenderPipeline() {
    auto vertModule = VulkanShaderModule{ "../../data/shaders/pass_through.vert.spv", device};
    auto fragModule = VulkanShaderModule{"../../data/shaders/pass_through.frag.spv", device};

    auto shaderStages = initializers::vertexShaderStages({
             { vertModule, VK_SHADER_STAGE_VERTEX_BIT },
             {fragModule, VK_SHADER_STAGE_FRAGMENT_BIT}
     });

    auto bindings = Vertex::bindingDisc();
    auto attribs = Vertex::attributeDisc();

    auto vertexInputState = initializers::vertexInputState(bindings, attribs);
    auto inputAssemblyState = initializers::inputAssemblyState();

    auto viewport = initializers::viewport(swapChain.extent);
    auto scissor = initializers::scissor(swapChain.extent);

    auto viewportState = initializers::viewportState(viewport, scissor);

    auto rasterizationState = initializers::rasterizationState();
    rasterizationState.cullMode = VK_CULL_MODE_NONE;
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
    auto colorBlendState = initializers::colorBlendState(colorBlendAttachment);

    render.layout = device.createPipelineLayout({});

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
    createInfo.layout = render.layout;
    createInfo.renderPass = renderPass;
    createInfo.subpass = 0;

    render.pipeline = device.createGraphicsPipeline(createInfo, pipelineCache);
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
    bullet.draw(commandBuffer);
    displayInfo(commandBuffer);
    vkCmdEndRenderPass(commandBuffer);

    vkEndCommandBuffer(commandBuffer);

    return &commandBuffer;
}

void BulletPhysicsDemo::update(float time) {
    auto text = fmt::format("{} - {} fps", title, framePerSecond);
    glfwSetWindowTitle(window, text.c_str());
    cameraController->update(time);
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
    bullet.set(const_cast<Camera*>(&cameraController->cam()));
}

void BulletPhysicsDemo::createRigidBodies() {
    RigidBody body;
    body.shape = new btBoxShape(btVector3(50.f, 50.f, 50.f));
    body.xform.basis = glm::mat3{1};
    body.xform.origin = glm::vec3(0, -50, 0);
    body.mass = 0.0;

    auto& bullet = plugin<BulletPhysicsPlugin>(BULLET_PHYSICS_PLUGIN);
    bullet.addRigidBody(body);

    auto boxShape = new btBoxShape(btVector3{.1, .1, .1});
    for(auto k = 0; k < 5; k++){
        for(auto i = 0; i < 5; i++){
            for(auto j = 0; j < 5; j++){
                RigidBody box{};
                box.shape = boxShape;
                box.xform.origin = {0.2f * float(i), 2.f + .2 * float(k), 0.2f * float(j)};
                box.mass = 1.f;
                bullet.addRigidBody(box);
            }
        }
    }

}

void BulletPhysicsDemo::displayInfo(VkCommandBuffer commandBuffer) {
    auto& imgui = plugin<ImGuiPlugin>(IM_GUI_PLUGIN);
    auto font = imgui.font("Arial", 15);
    ImGui::Begin("info", nullptr, IMGUI_NO_WINDOW);
    ImGui::SetWindowSize({ 500, float(height)});
    ImGui::PushFont(font);

    auto camPos = fmt::format("camera position: {}", cameraController->position());
    ImGui::TextColored({1, 1, 0, 1}, camPos.c_str());
    ImGui::PopFont();
    ImGui::End();
    imgui.draw(commandBuffer);
}


int main(){
    try{

        Settings settings;
        settings.depthTest = true;
        settings.enabledFeatures.wideLines = true;
        settings.enabledFeatures.fillModeNonSolid = true;

        auto app = BulletPhysicsDemo{ settings };

        BulletPhysicsPluginInfo info{};
        info.gravity = {0, -0.9807, 0};
        info.debugMode = btIDebugDraw::DebugDrawModes::DBG_DrawWireframe;
        info.timeStep = INV_120_HERTZ;
        info.maxSubSteps = 10;
        std::unique_ptr<Plugin> bullet = std::make_unique<BulletPhysicsPlugin>(info);

        std::vector<FontInfo> fonts {
#ifdef WIN32
                {"JetBrainsMono", R"(C:\Users\Josiah\Downloads\JetBrainsMono-2.225\fonts\ttf\JetBrainsMono-Regular.ttf)", 20},
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