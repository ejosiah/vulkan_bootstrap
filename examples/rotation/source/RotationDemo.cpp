#include "RotationDemo.hpp"
#include "GraphicsPipelineBuilder.hpp"
#include "DescriptorSetBuilder.hpp"

RotationDemo::RotationDemo(const Settings& settings) : VulkanBaseApp("Rotation", settings) {
    fileManager.addSearchPath(".");
    fileManager.addSearchPath("../../examples/rotation");
    fileManager.addSearchPath("../../examples/rotation/spv");
    fileManager.addSearchPath("../../examples/rotation/models");
    fileManager.addSearchPath("../../examples/rotation/textures");
    fileManager.addSearchPath("../../data/shaders");
    fileManager.addSearchPath("../../data/shaders/phong");
    fileManager.addSearchPath("../../data/models");
    fileManager.addSearchPath("../../data/textures");
    fileManager.addSearchPath("../../data");
}

void RotationDemo::initApp() {
    createDescriptorPool();
    createCommandPool();
    createPipelineCache();
    createRenderPipeline();
    createComputePipeline();
    createCamera();
    loadModel();
    createRotationAxis();
    updateModelParentTransform();
}

void RotationDemo::createDescriptorPool() {
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

void RotationDemo::createCommandPool() {
    commandPool = device.createCommandPool(*device.queueFamilyIndex.graphics, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    commandBuffers = commandPool.allocateCommandBuffers(swapChainImageCount);
}

void RotationDemo::createPipelineCache() {
    pipelineCache = device.createPipelineCache();
}


void RotationDemo::createRenderPipeline() {
    auto builder = device.graphicsPipelineBuilder();
    render.pipeline =
        builder
            .shaderStage()
                .vertexShader(load("torus.vert.spv"))
                .fragmentShader(load("torus.frag.spv"))
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
                    .polygonModeFill()
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
}

void RotationDemo::createComputePipeline() {
    auto module = VulkanShaderModule{ "../../data/shaders/pass_through.comp.spv", device};
    auto stage = initializers::shaderStage({ module, VK_SHADER_STAGE_COMPUTE_BIT});

    compute.layout = device.createPipelineLayout();

    auto computeCreateInfo = initializers::computePipelineCreateInfo();
    computeCreateInfo.stage = stage;
    computeCreateInfo.layout = compute.layout;

    compute.pipeline = device.createComputePipeline(computeCreateInfo, pipelineCache);
}


void RotationDemo::onSwapChainDispose() {
    dispose(render.pipeline);
    dispose(compute.pipeline);
}

void RotationDemo::onSwapChainRecreation() {
    createRenderPipeline();
    createComputePipeline();
    cameraController->perspective(swapChain.aspectRatio());
    auto view = m_registry.view<component::Pipelines>();
    component::Pipeline pipeline{ render.pipeline, render.layout };
    for(auto entity : view){
        auto& pipelines = view.get<component::Pipelines>(entity);
        pipelines.clear();
        pipelines.add(pipeline);
    }
}

VkCommandBuffer *RotationDemo::buildCommandBuffers(uint32_t imageIndex, uint32_t &numCommandBuffers) {
    numCommandBuffers = 1;
    auto& commandBuffer = commandBuffers[imageIndex];

    VkCommandBufferBeginInfo beginInfo = initializers::commandBufferBeginInfo();
    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    static std::array<VkClearValue, 2> clearValues;
    clearValues[0].color = {0.4, 0.4, 0.4, 1};
    clearValues[1].depthStencil = {1.0, 0u};

    VkRenderPassBeginInfo rPassInfo = initializers::renderPassBeginInfo();
    rPassInfo.clearValueCount = COUNT(clearValues);
    rPassInfo.pClearValues = clearValues.data();
    rPassInfo.framebuffer = framebuffers[imageIndex];
    rPassInfo.renderArea.offset = {0u, 0u};
    rPassInfo.renderArea.extent = swapChain.extent;
    rPassInfo.renderPass = renderPass;

    vkCmdBeginRenderPass(commandBuffer, &rPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    renderEntities(commandBuffer, m_registry);
    renderUI(commandBuffer);

    vkCmdEndRenderPass(commandBuffer);

    vkEndCommandBuffer(commandBuffer);

    return &commandBuffer;
}

void RotationDemo::update(float time) {

    if(moveCamera) {
        cameraController->update(time);
    }

    if(orderUpdated){
        auto view = m_registry.view<RotationAxis>();
        std::vector<entt::entity> entities;
        for(auto entity : view){
            entities.push_back(entity);
        }
        m_registry.destroy(begin(entities), end(entities));
        createRotationAxis();
        updateModelParentTransform();
        axisRotation = glm::vec3(0);
    }

    if(glm::any(glm::notEqual(axisRotation, glm::vec3(0)))){
        auto xView = m_registry.view<XAxis, component::Transform>();
        for(auto entity : xView){
            xView.get<component::Transform>(entity).value = glm::rotate(glm::mat4(1), glm::radians(axisRotation.x), {1, 0, 0});
        }
        auto yView = m_registry.view<YAxis, component::Transform>();
        for(auto entity : yView){
            yView.get<component::Transform>(entity).value = glm::rotate(glm::mat4(1), glm::radians(axisRotation.y), {0, 1, 0});
        }
        auto zView = m_registry.view<ZAxis, component::Transform>();
        for(auto entity : zView){
            zView.get<component::Transform>(entity).value = glm::rotate(glm::mat4(1), glm::radians(axisRotation.z), {0, 0, 1});
        }

        auto view = m_registry.view<RotationAxis, component::Transform>();
        for(auto entity : view){
            auto& transform = view.get<component::Transform>(entity);
            if(transform.parent){
                transform.value = transform.parent->value * transform.value;
            }
        }

        auto& transform = spaceShip.get<component::Transform>();
        transform.value = transform.parent->value;
    }
}

void RotationDemo::checkAppInputs() {
    if(moveCamera) {
        cameraController->processInput();
    }
}

void RotationDemo::cleanup() {
    // TODO save pipeline cache
    VulkanBaseApp::cleanup();
}

void RotationDemo::onPause() {
    VulkanBaseApp::onPause();
}

void RotationDemo::createCamera() {
    OrbitingCameraSettings settings{};
    settings.offsetDistance = 3.5f;
    settings.rotationSpeed = 0.1f;
    settings.zNear = 0.1f;
    settings.zFar = 10.0f;
    settings.fieldOfView = 45.0f;
    settings.modelHeight = 0;
    settings.aspectRatio = static_cast<float>(swapChain.extent.width)/static_cast<float>(swapChain.extent.height);
    cameraController = std::make_unique<OrbitingCameraController>(device, swapChain.imageCount(), currentImageIndex, dynamic_cast<InputManager&>(*this), settings);
    auto cameraEntity = Entity{m_registry };
    cameraEntity.add<component::Camera>().camera = const_cast<Camera*>(&cameraController->cam());
}

void RotationDemo::createRotationAxis() {
    if(rotationOrder == XYZ) {
        auto zAxis = createAxis("z_axis", 1, 0.05, {0, 0, 1}, {1, 0, 0, 0});
        zAxis.add<ZAxis>();

        auto orient = glm::angleAxis(-glm::half_pi<float>(), glm::vec3{1, 0, 0});
        auto yAxis = createAxis("y_axis", 1.10, 0.05, {0, 1, 0}, orient);
        yAxis.add<YAxis>();

        orient = glm::angleAxis(-glm::half_pi<float>(), glm::vec3{0, 1, 0});
        auto xAxis = createAxis("x_axis", 1.20, 0.05, {1, 0, 0}, orient);
        xAxis.add<XAxis>();

        zAxis.get<component::Transform>().parent = &yAxis.get<component::Transform>();
        yAxis.get<component::Transform>().parent = &xAxis.get<component::Transform>();
    }else if(rotationOrder == XZY){
        auto orient = glm::angleAxis(-glm::half_pi<float>(), glm::vec3{1, 0, 0});
        auto yAxis = createAxis("y_axis", 1, 0.05, {0, 1, 0}, orient);
        yAxis.add<YAxis>();

        auto zAxis = createAxis("z_axis", 1.10, 0.05, {0, 0, 1}, {1, 0, 0, 0});
        zAxis.add<ZAxis>();

        orient = glm::angleAxis(-glm::half_pi<float>(), glm::vec3{0, 1, 0});
        auto xAxis = createAxis("x_axis", 1.20, 0.05, {1, 0, 0}, orient);
        xAxis.add<XAxis>();

        yAxis.get<component::Transform>().parent = &zAxis.get<component::Transform>();
        zAxis.get<component::Transform>().parent = &xAxis.get<component::Transform>();
    }else if (rotationOrder == YXZ){
        auto zAxis = createAxis("z_axis", 1, 0.05, {0, 0, 1}, {1, 0, 0, 0});
        zAxis.add<ZAxis>();

        auto orient = glm::angleAxis(-glm::half_pi<float>(), glm::vec3{0, 1, 0});
        auto xAxis = createAxis("x_axis", 1.10, 0.05, {1, 0, 0}, orient);
        xAxis.add<XAxis>();

        orient = glm::angleAxis(-glm::half_pi<float>(), glm::vec3{1, 0, 0});
        auto yAxis = createAxis("y_axis", 1.20, 0.05, {0, 1, 0}, orient);
        yAxis.add<YAxis>();

        zAxis.get<component::Transform>().parent = &xAxis.get<component::Transform>();
        xAxis.get<component::Transform>().parent = &yAxis.get<component::Transform>();
    }else if(rotationOrder == YZX){
        auto orient = glm::angleAxis(-glm::half_pi<float>(), glm::vec3{0, 1, 0});
        auto xAxis = createAxis("x_axis", 1, 0.05, {1, 0, 0}, orient);
        xAxis.add<XAxis>();

        auto zAxis = createAxis("z_axis", 1.10, 0.05, {0, 0, 1}, {1, 0, 0, 0});
        zAxis.add<ZAxis>();

        orient = glm::angleAxis(-glm::half_pi<float>(), glm::vec3{1, 0, 0});
        auto yAxis = createAxis("y_axis", 1.20, 0.05, {0, 1, 0}, orient);
        yAxis.add<YAxis>();

        xAxis.get<component::Transform>().parent = &zAxis.get<component::Transform>();
        zAxis.get<component::Transform>().parent = &yAxis.get<component::Transform>();
    }else if(rotationOrder == ZXY){
        auto orient = glm::angleAxis(-glm::half_pi<float>(), glm::vec3{1, 0, 0});
        auto yAxis = createAxis("y_axis", 1, 0.05, {0, 1, 0}, orient);
        yAxis.add<YAxis>();

        orient = glm::angleAxis(-glm::half_pi<float>(), glm::vec3{0, 1, 0});
        auto xAxis = createAxis("x_axis", 1.10, 0.05, {1, 0, 0}, orient);
        xAxis.add<XAxis>();

        auto zAxis = createAxis("z_axis", 1.20, 0.05, {0, 0, 1}, {1, 0, 0, 0});
        zAxis.add<ZAxis>();

        yAxis.get<component::Transform>().parent = &xAxis.get<component::Transform>();
        xAxis.get<component::Transform>().parent = &zAxis.get<component::Transform>();
    }else if(rotationOrder == ZYX){
        auto orient = glm::angleAxis(-glm::half_pi<float>(), glm::vec3{0, 1, 0});
        auto xAxis = createAxis("x_axis", 1, 0.05, {1, 0, 0}, orient);
        xAxis.add<XAxis>();

        orient = glm::angleAxis(-glm::half_pi<float>(), glm::vec3{1, 0, 0});
        auto yAxis = createAxis("y_axis", 1.10, 0.05, {0, 1, 0}, orient);
        yAxis.add<YAxis>();

        auto zAxis = createAxis("z_axis", 1.20, 0.05, {0, 0, 1}, {1, 0, 0, 0});
        zAxis.add<ZAxis>();

        xAxis.get<component::Transform>().parent = &yAxis.get<component::Transform>();
        yAxis.get<component::Transform>().parent = &zAxis.get<component::Transform>();
    }
}

Entity RotationDemo::createAxis(const std::string& name, float innerR, float outerR, glm::vec3 color, glm::quat orientation) {
    auto torus = primitives::torus(50, 50, innerR, outerR, glm::mat4(orientation), glm::vec4(color, 1), VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

    auto vertices = device.createDeviceLocalBuffer(torus.vertices.data(), BYTE_SIZE(torus.vertices), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    auto indices = device.createDeviceLocalBuffer(torus.indices.data(), BYTE_SIZE(torus.indices), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);

    auto entity = createEntity(name);
    entity.add<RotationAxis>();

    auto& renderComp = entity.add<component::Render>();
    renderComp.vertexBuffers.push_back(vertices);
    renderComp.indexBuffer = indices;
    uint32_t indexCount = torus.indices.size();
    uint32_t vertexCount = torus.vertices.size();
    renderComp.primitives.push_back(vkn::Primitive::indexed(indexCount, 0, vertexCount, 0));
    renderComp.instanceCount = 1;
    renderComp.indexCount = indexCount;

    auto& pipelines = entity.add<component::Pipelines>();
    component::Pipeline pipeline{ render.pipeline, render.layout };
    pipelines.add(pipeline);

    return entity;
}

void RotationDemo::loadModel() {
    VulkanDrawable model;
    phong::load("../../data/models/bigship1.obj", device, descriptorPool, model);

    spaceShip = createEntity("model");
    auto& renderComp = spaceShip.add<component::Render>();
    renderComp.vertexBuffers.push_back(model.vertexBuffer);
    renderComp.indexBuffer = model.indexBuffer;
    renderComp.indexCount = model.numTriangles() * 3;
    renderComp.indexCount = 1;

    for(auto& mesh : model.meshes){
        renderComp.primitives.push_back(mesh);
    }

    auto& pipelines = spaceShip.add<component::Pipelines>();
    component::Pipeline pipeline{ render.pipeline, render.layout };
    pipelines.add(pipeline);
}

void RotationDemo::updateModelParentTransform() {

    component::Transform* parent = nullptr;
    auto tView = m_registry.view<RotationAxis, component::Transform>();
    for(auto entity : tView){
        parent = &tView.get<component::Transform>(entity);  // last oldest entry is parent
    };
    spaceShip.get<component::Transform>().parent = parent;

}


void RotationDemo::renderUI(VkCommandBuffer commandBuffer) {
    ImGui::Begin("Rotation");
    ImGui::SetWindowSize("Rotation", {300, 350});

    auto previousOrder = rotationOrder;
    ImGui::Text("Rotation Order");
    ImGui::RadioButton("XYZ", &rotationOrder, XYZ); ImGui::SameLine();
    ImGui::RadioButton("XZY", &rotationOrder, XZY); ImGui::SameLine();
    ImGui::RadioButton("YXZ", &rotationOrder, YXZ);

    ImGui::RadioButton("YZX", &rotationOrder, YZX); ImGui::SameLine();
    ImGui::RadioButton("ZXY", &rotationOrder, ZXY); ImGui::SameLine();
    ImGui::RadioButton("ZYX", &rotationOrder, ZYX);
    orderUpdated = previousOrder != rotationOrder;


    float range = 90.0f;
    auto renderOrder = [](const char* aLabel, const char* bLabel, const char* cLabel, float* a, float* b, float* c, float range){
        if(ImGui::TreeNodeEx(aLabel, ImGuiTreeNodeFlags_DefaultOpen)){
            ImGui::SliderFloat(aLabel, a, -range, range);

            ImGui::SetNextItemOpen(true, ImGuiCond_Once);
            if(ImGui::TreeNode(bLabel)){
                ImGui::SliderFloat(bLabel, b, -range, range);

                ImGui::SetNextItemOpen(true, ImGuiCond_Once);
                if(ImGui::TreeNode(cLabel)) {
                    ImGui::SliderFloat(cLabel, c, -range, range);
                    ImGui::TreePop();
                }
                ImGui::TreePop();
            }
            ImGui::TreePop();
        }
    };

    ImGui::Text("Rotations");
    if(rotationOrder == XYZ){
        renderOrder("X", "Y", "Z", &axisRotation.x, &axisRotation.y, &axisRotation.z, range);
    }else if(rotationOrder == XZY){
        renderOrder("X", "Z", "Y", &axisRotation.x, &axisRotation.z, &axisRotation.y, range);
    }else if(rotationOrder == YXZ){
        renderOrder("Y", "X", "Z", &axisRotation.y, &axisRotation.x, &axisRotation.z, range);
    }else if(rotationOrder == YZX){
        renderOrder("Y", "Z", "X", &axisRotation.y, &axisRotation.z, &axisRotation.x, range);
    }else if(rotationOrder == ZXY){
        renderOrder("Z", "X", "Y", &axisRotation.z, &axisRotation.x, &axisRotation.y, range);
    }else if(rotationOrder == ZYX){
        renderOrder("Z", "Y", "X", &axisRotation.z, &axisRotation.y, &axisRotation.x, range);
    }

    ImGui::Checkbox("Enable Camera", &moveCamera);
    ImGui::End();

    plugin(IM_GUI_PLUGIN).draw(commandBuffer);
}

int main(){
    try{

        Settings settings;
        settings.depthTest = true;

        auto app = RotationDemo{ settings };
        std::unique_ptr<Plugin> plugin = std::make_unique<ImGuiPlugin>();
        app.addPlugin(plugin);
        app.run();
    }catch(std::runtime_error& err){
        spdlog::error(err.what());
    }
}