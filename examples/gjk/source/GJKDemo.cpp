#include "GJKDemo.hpp"
#include "GraphicsPipelineBuilder.hpp"
#include "DescriptorSetBuilder.hpp"
#include "gjk.hpp"
#include "ImGuiPlugin.hpp"

GJKDemo::GJKDemo(const Settings& settings) : VulkanBaseApp("GJK Collision detection", settings) {
    fileManager.addSearchPath(".");
    fileManager.addSearchPath("../../examples/gjk");
    fileManager.addSearchPath("../../examples/gjk/spv");
    fileManager.addSearchPath("../../examples/gjk/models");
    fileManager.addSearchPath("../../examples/gjk/textures");
    fileManager.addSearchPath("../../data/shaders");
    fileManager.addSearchPath("../../data/models");
    fileManager.addSearchPath("../../data/textures");
    fileManager.addSearchPath("../../data");

    move.forward = &mapToKey(Key::UP, "move_forward");
    move.back = &mapToKey(Key::DOWN, "move_back");
    move.left = &mapToKey(Key::LEFT, "move_left");
    move.right = &mapToKey(Key::RIGHT, "move_right");
    move.up = &mapToKey(Key::W, "move_up");
    move.down = &mapToKey(Key::S, "move_down");
}

void GJKDemo::initApp() {
    createShapes();
    initCSOView();
    collisionTest();
    textures::fromFile(device, vulkanImage, resource("vulkan.png"));
    vulkanImageTexId = plugin<ImGuiPlugin>(IM_GUI_PLUGIN).addTexture(vulkanImage);
    initCameras();
    createDescriptorPool();
    createDescriptorSetLayouts();
    createCommandPool();
    
    updateDescriptorSets();
    createPipelineCache();
    createRenderPipeline();
    createComputePipeline();
}

void GJKDemo::createDescriptorSetLayouts() {
    textureDescriptorSetLayout =
        device.descriptorSetLayoutBuilder()
            .binding(0)
                .descriptorType(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE)
                .descriptorCount(1)
                .shaderStages(VK_SHADER_STAGE_ALL)
                .immutableSamplers(vulkanImage.sampler)
            .createLayout();
    
    vulkanImageDescriptorSet = descriptorPool.allocate( {textureDescriptorSetLayout }).front();
}

void GJKDemo::updateDescriptorSets() {
    auto writes = initializers::writeDescriptorSets<1>();
    
    writes[0].dstSet = vulkanImageDescriptorSet;
    writes[0].dstBinding = 0;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    writes[0].descriptorCount = 1;
    VkDescriptorImageInfo imageInfo{ VK_NULL_HANDLE, vulkanImage.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    writes[0].pImageInfo = &imageInfo;

    device.updateDescriptorSets(writes);
}

void GJKDemo::initCSOView() {
    csoView = CSOView{512, 512, &device, dynamic_cast<InputManager*>(this), &fileManager};
    csoTexId = plugin<ImGuiPlugin>(IM_GUI_PLUGIN).addTexture(csoView.renderTarget.imageView);
}

void GJKDemo::createDescriptorPool() {
    constexpr uint32_t maxSets = 100;
    std::array<VkDescriptorPoolSize, 16> poolSizes{
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
                    { VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 100 * maxSets }
            }
    };
    descriptorPool = device.createDescriptorPool(maxSets, poolSizes, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT);

}

void GJKDemo::createCommandPool() {
    commandPool = device.createCommandPool(*device.queueFamilyIndex.graphics, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    commandBuffers = commandPool.allocateCommandBuffers(swapChainImageCount);
}

void GJKDemo::createPipelineCache() {
    pipelineCache = device.createPipelineCache();
}


void GJKDemo::createRenderPipeline() {
    //    @formatter:off
    auto builder = device.graphicsPipelineBuilder();
    render.pipeline =
        builder
            .shaderStage()
                .vertexShader("../../data/shaders/flat.vert.spv")
                .fragmentShader("../../data/shaders/flat.frag.spv")
            .vertexInputState()
                .addVertexBindingDescriptions(Vertex::bindingDisc())
                .addVertexAttributeDescriptions(Vertex::attributeDisc())
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
                    .polygonModeLine()
                .multisampleState()
                    .rasterizationSamples(settings.msaaSamples)
                .depthStencilState()
                    .enableDepthWrite()
                    .enableDepthTest()
                    .compareOpLess()
                    .minDepthBounds(0)
                    .maxDepthBounds(1)
                .colorBlendState()
                    .attachment()
                    .add()
                .layout()
                    .addPushConstantRange(Camera::pushConstant())
                .renderPass(renderPass)
                .subpass(0)
                .name("render")
                .pipelineCache(pipelineCache)
            .build(render.layout);
    //    @formatter:on
}

void GJKDemo::createComputePipeline() {
    auto module = VulkanShaderModule{ "../../data/shaders/pass_through.comp.spv", device};
    auto stage = initializers::shaderStage({ module, VK_SHADER_STAGE_COMPUTE_BIT});

    compute.layout = device.createPipelineLayout();

    auto computeCreateInfo = initializers::computePipelineCreateInfo();
    computeCreateInfo.stage = stage;
    computeCreateInfo.layout = compute.layout;

    compute.pipeline = device.createComputePipeline(computeCreateInfo, pipelineCache);
}


void GJKDemo::onSwapChainDispose() {
    dispose(render.pipeline);
    dispose(compute.pipeline);
}

void GJKDemo::onSwapChainRecreation() {
    cameraController->perspective(swapChain.aspectRatio());
    createRenderPipeline();
    createComputePipeline();
}

VkCommandBuffer *GJKDemo::buildCommandBuffers(uint32_t imageIndex, uint32_t &numCommandBuffers) {
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

    VkDeviceSize offset = 0;

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, render.pipeline);
    cameraController->push(commandBuffer, render.layout);
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, sphere.vertices, &offset);
    vkCmdBindIndexBuffer(commandBuffer, sphere.indices, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(commandBuffer, sphere.indices.sizeAs<uint32_t>(), 1, 0, 0, 0);


    cameraController->push(commandBuffer, render.layout, box.model());
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, box.vertices, &offset);
    vkCmdBindIndexBuffer(commandBuffer, box.indices, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(commandBuffer, box.indices.sizeAs<uint32_t>(), 1, 0, 0, 0);

    renderCSO(commandBuffer);

    vkCmdEndRenderPass(commandBuffer);

    vkEndCommandBuffer(commandBuffer);

    return &commandBuffer;
}

void GJKDemo::renderCSO(VkCommandBuffer commandBuffer) {
    auto flag = csoCam ? ImGuiWindowFlags_NoMove : ImGuiWindowFlags_None;
    ImGui::Begin("Configuration space", nullptr, flag);
    float w = 512;
    float h = 512;
    ImGui::SetWindowSize({0, 0});

    ImGui::Image(csoTexId, {w, h});
    ImGui::Checkbox("update", &csoCam);
    ImGui::End();
    plugin(IM_GUI_PLUGIN).draw(commandBuffer);
}

void GJKDemo::update(float time) {
    if(!ImGui::IsAnyItemActive()) {
        cameraController->update(time);
    }else if(csoCam){
        csoView.update(time);
    }

}

void GJKDemo::checkAppInputs() {
    if(!ImGui::IsAnyItemActive()) {
        cameraController->processInput();
    }else if(csoCam){
        csoView.checkAppInput();
    }
    glm::vec3 dir{0};
    if(move.forward->isPressed()){
        dir.z -= 1;
    }
    if(move.back->isPressed()){
        dir.z += 1;
    }
    if(move.right->isPressed()){
        dir.x += 1;
    }
    if(move.left->isPressed()){
        dir.x -= 1;
    }
    if(move.up->isPressed()){
        dir.y += 1;
    }
    if(move.down->isPressed()){
        dir.y -= 1;
    }
    dir *= 0.001;
    auto shouldMoveBox = glm::any(glm::lessThan(glm::vec3(0), dir) || glm::greaterThan(glm::vec3(0), dir));
    if(shouldMoveBox) {
        box.position += glm::mat3(glm::inverse(cameraController->cam().view)) * dir;
        collisionTest();
    }
}

void GJKDemo::cleanup() {
    // TODO save pipeline cache
    VulkanBaseApp::cleanup();
}

void GJKDemo::onPause() {
    VulkanBaseApp::onPause();
}

void GJKDemo::createShapes() {
    auto ss = primitives::sphere(10, 10, sphere.radius, glm::mat4{1}, glm::vec4{1}, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    sphere.vertices = device.createCpuVisibleBuffer(ss.vertices.data(), BYTE_SIZE(ss.vertices), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    sphere.indices = device.createCpuVisibleBuffer(ss.indices.data(), BYTE_SIZE(ss.indices), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    
    auto bs = primitives::cube(glm::vec4{1});
    box.vertices = device.createCpuVisibleBuffer(bs.vertices.data(), BYTE_SIZE(bs.vertices), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    box.indices = device.createCpuVisibleBuffer(bs.indices.data(), BYTE_SIZE(bs.indices), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    box.buildCorners();
}

void GJKDemo::initCameras() {
    OrbitingCameraSettings settings{};
    settings.offsetDistance = 10.0f;
    settings.orbitMinZoom = 0.1f;
    settings.orbitMaxZoom = 10.f;
    settings.rotationSpeed = 0.1f;
    settings.zNear = 0.1f;
    settings.zFar = 20.0f;
    settings.fieldOfView = 45.0f;
    settings.modelHeight = 0;
    settings.aspectRatio = static_cast<float>(swapChain.extent.width)/static_cast<float>(swapChain.extent.height);
    cameraController = std::make_unique<OrbitingCameraController>(dynamic_cast<InputManager&>(*this), settings);
}

void GJKDemo::newFrame() {
    cameraController->newFrame();
}

void GJKDemo::collisionTest() {
    glm::vec3 pointOnA, pointOnB;
    static std::vector<glm::vec3> simplex;
    static std::vector<uint16_t> indices;
    float bias = 0.001f;
    indices.clear();
    auto intersects = GJK::doesIntersect(sphere, box, bias, pointOnA, pointOnB, simplex, indices);
    spdlog::info("intersects {}", intersects);
    csoView.update(simplex, indices);
}


int main(){
    try{

        Settings settings;
        settings.depthTest = true;
        settings.enabledFeatures.fillModeNonSolid = true;

        auto app = GJKDemo{ settings };
        std::unique_ptr<Plugin> plugin = std::make_unique<ImGuiPlugin>();
        app.addPlugin(plugin);
        app.run();
    }catch(std::runtime_error& err){
        spdlog::error(err.what());
    }
}