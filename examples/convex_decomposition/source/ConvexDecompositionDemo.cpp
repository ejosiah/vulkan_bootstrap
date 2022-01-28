#include "ConvexDecompositionDemo.hpp"
#include "GraphicsPipelineBuilder.hpp"
#include "DescriptorSetBuilder.hpp"
#include "ImGuiPlugin.hpp"
#include "ThreadPool.hpp"

ConvexDecompositionDemo::ConvexDecompositionDemo(const Settings& settings) : VulkanBaseApp("Hierarchical Approximate Convex Decomposition", settings) {
    fileManager.addSearchPath(".");
    fileManager.addSearchPath("../../examples/convex_decomposition");
    fileManager.addSearchPath("../../examples/convex_decomposition/spv");
    fileManager.addSearchPath("../../examples/convex_decomposition/models");
    fileManager.addSearchPath("../../examples/convex_decomposition/textures");
    fileManager.addSearchPath("../../data/shaders");
    fileManager.addSearchPath("../../data/models");
    fileManager.addSearchPath("../../data/textures");
    fileManager.addSearchPath("../../data");
}

void ConvexDecompositionDemo::initApp() {
    initOpenCL();
    initVHACD();

    createDescriptorPool();
    createCommandPool();
    createPipelineCache();

    createFloor();
    loadBunny();
    loadDragon();
    loadArmadillo();
    loadSkull();
    loadWereWolf();
    constructConvexHull();

    initCamera();
    createRenderPipeline();
}

void ConvexDecompositionDemo::initOpenCL(){
#ifdef CL_VERSION_1_1
    std::vector<std::string> info;

    auto good = oclHelper.GetPlatformsInfo(info, "\t\t");

    if(!good) return;
    std::stringstream ss;
    const auto numPlatforms = info.size();
    ss << "\t Number of OpenCL platforms: " << numPlatforms << "\n";
    for(auto i = 0; i < numPlatforms; i++){
        ss << "\t OpenCL platform [" << i << "]" << "\n";
        ss << info[i];
    }
    ss << "\t Using OpenCL platform [" << params.oclPlatformID << "]" << "\n";
    good = oclHelper.InitPlatform(params.oclPlatformID);
    if(!good) return;

    info.clear();
    good = oclHelper.GetDevicesInfo(info, "\t\t");
    if(!good) return;

    const auto numDevices = info.size();
    ss << "\t Number of OpenCL devices: " << numDevices << "\n";
    for(auto i = 0; i < numDevices; i++){
        ss << "\t OpenCL device [" << i << "]" << "\n";
        ss << info[i];
    }
    good = oclHelper.InitDevice(params.oclDeviceID);

    if(good){
        spdlog::info("OpenCL (ON)");
        spdlog::debug(ss.str());
        spdlog::info("Using OpenCL device [{}]", params.oclDeviceID);
        openCLOnline = true;
    }
#else
    spdlog::info("OpenCL (OFF)")
#endif
}

void ConvexDecompositionDemo::createDescriptorPool() {
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

void ConvexDecompositionDemo::createCommandPool() {
    commandPool = device.createCommandPool(*device.queueFamilyIndex.graphics, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    commandBuffers = commandPool.allocateCommandBuffers(swapChainImageCount);

    secondaryCommandPool = device.createCommandPool(*device.queueFamilyIndex.graphics,
                                                    VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    for(int i = 0; i < MAX_IN_FLIGHT_FRAMES; i++) {
        secondaryCommandBuffers[i] = secondaryCommandPool.allocateCommandBuffers(NUM_COMMANDS,
                                                                              VK_COMMAND_BUFFER_LEVEL_SECONDARY);
    }

    commandPoolCH = device.createCommandPool(*device.queueFamilyIndex.transfer,
                                             VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);
}

void ConvexDecompositionDemo::createPipelineCache() {
    pipelineCache = device.createPipelineCache();
}


void ConvexDecompositionDemo::createRenderPipeline() {
    auto builder = device.graphicsPipelineBuilder();
    render[0].pipeline =
        builder
            .allowDerivatives()
            .shaderStage()
                .vertexShader(load("render.vert.spv"))
                .fragmentShader(load("render.frag.spv"))
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
                .addPushConstantRange(VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(Camera), sizeof(glm::vec4))
            .renderPass(renderPass)
            .subpass(0)
            .name("render")
            .pipelineCache(pipelineCache)
        .build(render[0].layout);

    render[1].pipeline =
        builder
            .basePipeline(render[0].pipeline)
        .rasterizationState()
            .frontFaceClockwise()
        .build(render[1].layout);

    ch.pipeline =
        builder
            .shaderStage()
                .vertexShader(load("render.vert.spv"))
                .fragmentShader(load("render.frag.spv"))
            .rasterizationState()
                .frontFaceCounterClockwise()
            .basePipeline(render[0].pipeline)
            .name("convex_hull")
        .build(ch.layout);


    mirrorRender[0].pipeline =
        builder
            .basePipeline(render[0].pipeline)
            .shaderStage()
                .vertexShader(load("mirror.vert.spv"))
                .fragmentShader(load("render.frag.spv"))
            .rasterizationState()
                .frontFaceClockwise()
            .name("mirror_render")
        .build(mirrorRender[0].layout);

    mirrorRender[1].pipeline =
        builder
            .rasterizationState()
                .frontFaceCounterClockwise()
        .build(mirrorRender[1].layout);

    mirrorCH.pipeline =
        builder
            .basePipeline(ch.pipeline)
            .shaderStage()
                .vertexShader(load("mirror.vert.spv"))
                .fragmentShader(load("render.frag.spv"))
            .rasterizationState()
                .frontFaceClockwise()
            .name("mirror_convex_hull")
        .build(mirrorCH.layout);

    floor.pipeline =
        builder
            .basePipeline(render[0].pipeline)
            .shaderStage()
            .vertexShader(load("render.vert.spv"))
                .fragmentShader(load("checkerboard.frag.spv"))
            .rasterizationState()
                .cullNone()
                .frontFaceCounterClockwise()
                .colorBlendState()
                    .attachment().clear()
                    .enableBlend()
                    .colorBlendOp().add()
                    .alphaBlendOp().add()
                    .srcColorBlendFactor().dstAlpha()
                    .dstColorBlendFactor().oneMinusDstAlpha()
                    .srcAlphaBlendFactor().srcAlpha()
                    .dstAlphaBlendFactor().zero()
                .add()
            .layout().clear()
                .addPushConstantRange(Camera::pushConstant())
            .name("floor")
        .build(floor.layout);

    normals.pipeline =
        builder
            .basePipeline(render[0].pipeline)
            .shaderStage()
                .vertexShader(load("draw_normals.vert.spv"))
                .geometryShader(load("draw_normals.geom.spv"))
                .fragmentShader(load("point.frag.spv"))
            .inputAssemblyState()
                .points()
            .rasterizationState()
            .colorBlendState()
                .attachment().clear()
                .disableBlend()
                .add()
            .layout()
                .addPushConstantRange(VK_SHADER_STAGE_GEOMETRY_BIT, 0, sizeof(normals.constants))
            .name("normals")
        .build(normals.layout);
}



void ConvexDecompositionDemo::onSwapChainDispose() {
}

void ConvexDecompositionDemo::onSwapChainRecreation() {
    createRenderPipeline();
}

VkCommandBuffer *ConvexDecompositionDemo::buildCommandBuffers(uint32_t imageIndex, uint32_t &numCommandBuffers) {
    numCommandBuffers = 1;
    auto& commandBuffer = commandBuffers[imageIndex];

    VkCommandBufferBeginInfo beginInfo = initializers::commandBufferBeginInfo();
    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    static std::array<VkClearValue, 2> clearValues;
    clearValues[0].color = {0.8, 0.8, 0.8, 0.8};
    clearValues[1].depthStencil = {1.0, 0u};

    VkRenderPassBeginInfo rPassInfo = initializers::renderPassBeginInfo();
    rPassInfo.clearValueCount = COUNT(clearValues);
    rPassInfo.pClearValues = clearValues.data();
    rPassInfo.framebuffer = framebuffers[imageIndex];
    rPassInfo.renderArea.offset = {0u, 0u};
    rPassInfo.renderArea.extent = swapChain.extent;
    rPassInfo.renderPass = renderPass;

    vkCmdBeginRenderPass(commandBuffer, &rPassInfo, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
    VkCommandBufferInheritanceInfo inheritanceInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO};
    inheritanceInfo.renderPass = rPassInfo.renderPass;
    inheritanceInfo.framebuffer = rPassInfo.framebuffer;

    renderModel(inheritanceInfo);
    renderConvexHull(inheritanceInfo);
    mirror(inheritanceInfo);
    renderFloor(inheritanceInfo);
    renderUI(inheritanceInfo);

    vkCmdExecuteCommands(commandBuffer, secondaryCommandBuffers[currentFrame].size(), secondaryCommandBuffers[currentFrame].data());

    vkCmdEndRenderPass(commandBuffer);

    vkEndCommandBuffer(commandBuffer);

    return &commandBuffer;
}

void ConvexDecompositionDemo::update(float time) {
    glfwSetWindowTitle(window, fmt::format("{} - FPS {}", title, framePerSecond).c_str());
    if(!ImGui::IsAnyItemActive()) {
        cameraController->update(time);
    }
    if(nextModel != currentModel){
        currentModelChanged();
    }

    if(shouldUpdateConvexHull()){
        constructConvexHull();
    }
}

void ConvexDecompositionDemo::checkAppInputs() {
    cameraController->processInput();
}

void ConvexDecompositionDemo::cleanup() {
//    for(int i = 0; i < MAX_IN_FLIGHT_FRAMES; i++){
//        secondaryCommandPool.free(secondaryCommandBuffers[i]);
//    }
}

void ConvexDecompositionDemo::onPause() {
    VulkanBaseApp::onPause();
}

void ConvexDecompositionDemo::loadBunny() {
    phong::VulkanDrawableInfo info{};

    info.vertexUsage += VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT ;
    info.indexUsage += VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    Model bunny;
    phong::load(resource("bunny.obj"), device, descriptorPool, bunny.drawable, info, true);

    // bunny is Left handed so we need to flip its normals
    auto vertexBuffer = device.createBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                            VMA_MEMORY_USAGE_CPU_ONLY, bunny.drawable.vertexBuffer.size);

    device.copy(bunny.drawable.vertexBuffer, vertexBuffer, bunny.drawable.vertexBuffer.size, 0, 0);

    auto vertices = reinterpret_cast<Vertex*>(vertexBuffer.map());
    auto size = vertexBuffer.size/sizeof(Vertex);
    for(int i = 0; i < size; i++){
        vertices[i].normal *= -1;
    }
    vertexBuffer.unmap();
    device.copy(vertexBuffer, bunny.drawable.vertexBuffer, vertexBuffer.size);

    auto d = bunny.drawable.bounds.max - bunny.drawable.bounds.min;
    bunny.transform = glm::rotate(glm::mat4(1), -glm::half_pi<float>(), {1, 0, 0});;
    bunny.transform = glm::translate(bunny.transform, -bunny.drawable.bounds.min);

    models[0].drawable = std::move(bunny.drawable);
    models[0].transform = bunny.transform;
    models[0].ready = true;
    models[0].pipeline = 1;

    updateHACDData();
}

void ConvexDecompositionDemo::loadDragon() {
    auto future = loadModel("ChineseDragon.obj", 1, [&](auto& model){
        auto yOffset = -model.drawable.bounds.min.y;
        model.transform = glm::translate(glm::mat4(1), {0, yOffset, 0});
        model.transform = glm::rotate(model.transform, -glm::radians(45.f), {0, 1, 0});
    });
    par::ThreadPool::global().onComplete(future.share(), [](auto done){
        spdlog::info("Dragon loaded and ready to use");
    });
}

void ConvexDecompositionDemo::loadArmadillo() {
    auto future = loadModel("Armadillo.obj", 2, [&](auto& model){
        auto yOffset = -model.drawable.bounds.min.y;
        model.transform = glm::translate(glm::mat4(1), {0, yOffset, 0});
        model.transform = glm::rotate(model.transform, glm::radians(180.f), {0, 1, 0});
    });
    par::ThreadPool::global().onComplete(future.share(), [](auto done){
        spdlog::info("Armadillo loaded and ready to use");
    });
}

void ConvexDecompositionDemo::loadSkull() {
    auto future = loadModel("skull/Male_Human_Skull_convert.obj", 3, [&](auto& model){
        auto center = (model.drawable.bounds.min + model.drawable.bounds.max) * 0.5f;
        auto yOffset = -model.drawable.bounds.min.y;
        model.transform = glm::translate(glm::mat4(1), {0, yOffset, 0});
        model.transform = glm::translate(model.transform, center);
        model.transform = glm::rotate(model.transform, glm::radians(-90.f), {1, 0, 0});
        model.transform = glm::translate(model.transform, -center);
    });
    par::ThreadPool::global().onComplete(future.share(), [](auto done){
        spdlog::info("Skull loaded and ready to use");
    });
}

void ConvexDecompositionDemo::loadWereWolf() {
    auto future = loadModel("werewolf.obj", 4, [&](auto& model){
        auto yOffset = -model.drawable.bounds.min.y;
        model.transform = glm::translate(glm::mat4(1), {0, yOffset, 0});
    });
    par::ThreadPool::global().onComplete(future.share(), [](auto done){
        spdlog::info("Were Wolf loaded and ready to use");
    });
}

std::future<par::done> ConvexDecompositionDemo::loadModel(const std::string &path, const int index){
    return loadModel(path, index, [](auto& model){});
}

std::future<par::done> ConvexDecompositionDemo::loadModel(const std::string &path, const int index, std::function<void(Model&)>&& postLoadOp) {
    return
    par::ThreadPool::global().async([=]{
        phong::VulkanDrawableInfo info{};

        info.vertexUsage += VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT ;
        info.indexUsage += VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        Model model;
        auto fullPath = resource(path);
        phong::load(fullPath, device, descriptorPool, model.drawable, info, true);

        postLoadOp(model);

        models[index].drawable = std::move(model.drawable);
        models[index].transform = model.transform;
        models[index].ready = true;

        return par::done{};
    });
    phong::VulkanDrawableInfo info{};

}

void ConvexDecompositionDemo::initCamera() {
    OrbitingCameraSettings settings{};
    settings.modelHeight =  models[currentModel].drawable.height();
    settings.aspectRatio = swapChain.aspectRatio();
    settings.horizontalFov = true;
    settings.fieldOfView = 60.0f;
    settings.orbitMinZoom = 0;
    settings.offsetDistance = models[currentModel].drawable.height() * 5;
    settings.orbitMaxZoom = settings.offsetDistance * 10;
    settings.target = getCameraTarget(models[currentModel]);

    cameraController = std::make_unique<OrbitingCameraController>(device, swapChainImageCount, currentImageIndex
                                                                  , dynamic_cast<InputManager&>(*this), settings);
}

void ConvexDecompositionDemo::renderUI(VkCommandBufferInheritanceInfo& inheritanceInfo) {
    auto commandBuffer = secondaryCommandBuffers[currentFrame][RENDER_UI];

    VkCommandBufferBeginInfo beginInfo = initializers::commandBufferBeginInfo();
    beginInfo.pInheritanceInfo = &inheritanceInfo;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    ImGui::Begin("HACD options");
    ImGui::SetWindowSize({450, 400});

    ImGui::Text("CH Mode:");
    ImGui::RadioButton("voxel", &params.mode, 0); ImGui::SameLine();
    ImGui::RadioButton("tetrahedron", &params.mode, 1);

    ImGui::SliderInt("resolution", &params.resolution, 10000, 16000000, "%d", ImGuiSliderFlags_Logarithmic);
    ImGui::SliderFloat("concavity", &params.concavity, 0.0010, 1.0);
    ImGui::SliderInt("plane downsampling", &params.planeDownSampling, 1, 16);
    ImGui::SliderFloat("alpha", &params.alpha, 0.0, 1.0);
    ImGui::SliderFloat("beta", &params.beta, 0.0, 1.0);
    ImGui::SliderFloat("gamma", &params.gamma, 0.0, 1.0);
    ImGui::SliderFloat("delta", &params.delta, 0.0, 1.0);

    static bool pcaEnabled = false;
    ImGui::Checkbox("pca", &pcaEnabled);
    params.pca = static_cast<int>(pcaEnabled);


    ImGui::SliderInt("max vertices per CH", &params.maxNumVerticesPerCH, 4, 1024);
    ImGui::SliderFloat("min volume per CH", &params.minVolumePerCH, 0.0, 0.01);

    static bool chApproximationEnabled = true;
    ImGui::Checkbox("CH approximation", &chApproximationEnabled);
    params.convexHullApproximation = static_cast<int>(chApproximationEnabled);


    if(ImGui::CollapsingHeader("Model", ImGuiTreeNodeFlags_DefaultOpen)){
        static int prevModel;
        prevModel = nextModel;

        ImGui::RadioButton("Bunny", &nextModel, 0);

        if(models[1].ready) {
            ImGui::SameLine();
            ImGui::RadioButton("Dragon", &nextModel, 1);
        }
        if(models[2].ready) {
            ImGui::SameLine();
            ImGui::RadioButton("Armadillo", &nextModel, 2);
        }
        if(models[3].ready) {
            ImGui::SameLine();
            ImGui::RadioButton("Skull", &nextModel, 3);
        }
        if(models[4].ready) {
            ImGui::SameLine();
            ImGui::RadioButton("Were Wolf", &nextModel, 4);
        }
    }


    updateHulls |= ImGui::Button("update", {100, 20});

    ImGui::End();

    plugin(IM_GUI_PLUGIN).draw(commandBuffer);

    vkEndCommandBuffer(commandBuffer);
}

void ConvexDecompositionDemo::initVHACD() {
    interfaceVHACD = VHACD::CreateVHACD();
#ifdef CL_VERSION_1_1
    if(params.oclAcceleration){
        auto good = interfaceVHACD->OCLInit(oclHelper.GetDevice(), &loggerVHACD);
        if(!good){
            params.oclAcceleration = 0;
        }
    }
#endif
}

void ConvexDecompositionDemo::updateHACDData() {
    for(int i = 0; i < 2; i++) {
        auto& convexHulls = this->convexHulls[i];
        auto vertexBuffer = device.createBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                                VMA_MEMORY_USAGE_CPU_ONLY, models[currentModel].drawable.vertexBuffer.size);
        auto indexBuffer = device.createBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                               VMA_MEMORY_USAGE_CPU_ONLY, models[currentModel].drawable.indexBuffer.size);

        device.copy(models[currentModel].drawable.vertexBuffer, vertexBuffer, models[currentModel].drawable.vertexBuffer.size, 0, 0);
        device.copy(models[currentModel].drawable.indexBuffer, indexBuffer, models[currentModel].drawable.indexBuffer.size, 0, 0);

        auto vertices = reinterpret_cast<Vertex *>(vertexBuffer.map());
        auto numVertices = vertexBuffer.size / sizeof(Vertex);
        convexHulls.points.clear();

        for (int i = 0; i < numVertices; i++) {
            auto &point = vertices[i].position;
            convexHulls.points.push_back(point.x);
            convexHulls.points.push_back(point.y);
            convexHulls.points.push_back(point.z);
        };
        vertexBuffer.unmap();

        auto indices = reinterpret_cast<uint32_t *>(indexBuffer.map());
        auto numIndices = indexBuffer.size / sizeof(uint32_t);

        convexHulls.triangles.clear();
        for (int i = 0; i < numIndices; i++) {
            convexHulls.triangles.push_back(indices[i]);
        }
        indexBuffer.unmap();
    }
}

void ConvexDecompositionDemo::renderModel(VkCommandBufferInheritanceInfo& inheritanceInfo) {
    auto commandBuffer = secondaryCommandBuffers[currentFrame][RENDER_MODEL];

    VkCommandBufferBeginInfo beginInfo = initializers::commandBufferBeginInfo();
    beginInfo.pInheritanceInfo = &inheritanceInfo;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    if (models[currentModel].ready) {

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, render[models[currentModel].pipeline].pipeline);

        glm::mat4 model = positionModel(models[currentModel], -1) * models[currentModel].transform;
        cameraController->push(commandBuffer, render[models[currentModel].pipeline].layout, model);
        glm::vec4 color{0.8, 0.8, 0.8, 1.0};
        if (cameraController->position().y < 0) {
            color.a = 0.3;
        }
        vkCmdPushConstants(commandBuffer, render[models[currentModel].pipeline].layout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(Camera),
                           sizeof(glm::vec4), &color);
        models[currentModel].drawable.draw(commandBuffer);
    }

    vkEndCommandBuffer(commandBuffer);
}

void ConvexDecompositionDemo::renderConvexHull(VkCommandBufferInheritanceInfo &inheritanceInfo) {
    auto commandBuffer = secondaryCommandBuffers[currentFrame][RENDER_CONVEX_HULL];
    auto beginInfo = initializers::commandBufferBeginInfo();
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
    beginInfo.pInheritanceInfo = &inheritanceInfo;
    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    if(hullIsReady[currentHull]){
        auto& convexHulls = this->convexHulls[currentHull];

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, ch.pipeline);

        glm::mat4 model = positionModel(models[currentModel]) * models[currentModel].transform;

        VkDeviceSize offset = 0;
        auto numHulls = convexHulls.vertices.size();
        for(int i = 0; i < numHulls; i++){
            auto& vertexBuffer = convexHulls.vertices[i];
            auto& indexBuffer = convexHulls.indices[i];
            auto& color = convexHulls.colors[i];
            if(cameraController->position().y < 0){
                color.a = 0.3;
            }
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, ch.pipeline);
            cameraController->push(commandBuffer, ch.layout, model);
            vkCmdPushConstants(commandBuffer, ch.layout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(Camera), sizeof(glm::vec4), &color);
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffer, &offset);
            vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
            vkCmdDrawIndexed(commandBuffer, indexBuffer.size/sizeof(uint32_t), 1, 0, 0, 0);
        }
    }

    vkEndCommandBuffer(commandBuffer);
}

inline VHACD::IVHACD::Parameters ConvexDecompositionDemo::getParams() {
    static VHACD::IVHACD::Parameters parameters{};
    parameters.m_resolution = params.resolution;
    parameters.m_concavity = params.concavity;
    parameters.m_planeDownsampling = params.planeDownSampling;
    parameters.m_convexhullDownsampling = params.convexHullDownSampling;
    parameters.m_alpha = params.alpha;
    parameters.m_beta = params.beta;
    parameters.m_pca = params.pca;
    parameters.m_mode = params.mode;
    parameters.m_maxNumVerticesPerCH = params.maxNumVerticesPerCH;
    parameters.m_minVolumePerCH = params.minVolumePerCH;
    parameters.m_convexhullApproximation = params.convexHullApproximation;
    parameters.m_maxConvexHulls = params.maxHulls;
    parameters.m_oclAcceleration = params.oclAcceleration;
    parameters.m_callback = &callbackVHACD;
    return parameters;
}

bool ConvexDecompositionDemo::shouldUpdateConvexHull() {
    static decltype(params) cParams = params;
    auto isSame = !std::abs(std::memcmp(&cParams, &params, sizeof(params)));
    if(updateHulls && !isSame){
        cParams = params;
        return true;
    }
    updateHulls = false;
    return false;
}

void ConvexDecompositionDemo::constructConvexHull() {

    spdlog::info("constructing convex hulls");
    hullIsReady[!currentHull] = false;

    auto& threadPool = par::ThreadPool::global();

    auto fHulls =
        threadPool.async([&]{
           int hullIndex = !currentHull;
           auto& convexHulls = this->convexHulls[hullIndex];

           auto params = getParams();
           auto res = interfaceVHACD->Compute(convexHulls.points.data(), convexHulls.points.size()/3
                                    , convexHulls.triangles.data(), convexHulls.triangles.size()/3, params );

           if(!res) throw std::runtime_error{"unable to build convex hull"};

           auto numConvexHulls = interfaceVHACD->GetNConvexHulls();

           ConvexHulls hulls{};
           hulls.points = convexHulls.points;
           hulls.triangles = convexHulls.triangles;
           hulls.vertices.reserve(numConvexHulls);

           static uint32_t seed = 1 << 20;
           VHACD::IVHACD::ConvexHull ch{};
           std::vector<std::vector<Vertex>> mesh;
           mesh.reserve(numConvexHulls);

           std::vector<std::vector<uint32_t>> meshIndices;
           meshIndices.reserve(numConvexHulls);

           auto numTriangles = 0u;
           auto totalVertices = 0u;
           auto rnd = rng(0, 1, 1 << 20);
           for(int i = 0; i < numConvexHulls; i++){
               interfaceVHACD->GetConvexHull(i, ch);

               std::vector<Vertex> vertices;
               auto numVertices = ch.m_nPoints * 3;
               for(int j = 0; j < numVertices; j += 3){
                   auto x = static_cast<float>(ch.m_points[j + 0]);
                   auto y = static_cast<float>(ch.m_points[j + 1]);
                   auto z = static_cast<float>(ch.m_points[j + 2]);
                   Vertex v{};
                   v.position = glm::vec4(x, y, z, 1);
                   vertices.push_back(v);
               }

               std::vector<uint32_t> indices;
               numTriangles += ch.m_nTriangles;
               totalVertices += ch.m_nPoints;
               auto numIndices = ch.m_nTriangles * 3;
               for(int j = 0; j < numIndices; j+=3){
                   auto i0 = ch.m_triangles[j + 0];
                   auto i1 = ch.m_triangles[j + 1];
                   auto i2 = ch.m_triangles[j + 2];

                   auto& v0 = vertices[i0];
                   auto& v1 = vertices[i1];
                   auto& v2 = vertices[i2];

                   //  generate normals
                   glm::vec3 centerCH{ ch.m_center[0], ch.m_center[2], ch.m_center[2]};
                   glm::vec3 centerTri = ((v0.position + v1.position + v2.position)/3.0f).xyz();
                   auto a = v1.position.xyz() - v0.position.xyz();
                   auto b = v2.position.xyz() - v0.position.xyz();
                   auto normal = glm::cross(a, b);
                   normal = glm::normalize(normal);
                   v0.normal = normal;
                   v1.normal = normal;
                   v2.normal = normal;


                   indices.push_back(i0);
                   indices.push_back(i1);
                   indices.push_back(i2);
               }

               mesh.push_back(vertices);
               meshIndices.push_back(indices);
               hulls.colors.emplace_back(rnd(), rnd(), rnd(), 1.0f);
           }

           std::vector<VulkanBuffer> stagingBuffers;
           stagingBuffers.clear();
           auto numBuffers = numConvexHulls * 2;
           stagingBuffers.reserve(numBuffers);
           for(auto i = 0; i < numConvexHulls; i++){
               auto vertices = mesh[i];
               if(vertices.empty()) continue;

               auto size = BYTE_SIZE(vertices);
               auto stagingBuffer = device.createStagingBuffer(size);
               stagingBuffer.copy(vertices.data(), size);
               stagingBuffers.push_back(stagingBuffer);

               auto vertexBuffer = device.createBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
                                                        , VMA_MEMORY_USAGE_GPU_ONLY, size);
               hulls.vertices.push_back(vertexBuffer);
           }

           for(auto i = 0; i < numConvexHulls; i++){
               auto indices = meshIndices[i];

               if(indices.empty()) continue;

               auto size = BYTE_SIZE(indices);
               auto stagingBuffer = device.createStagingBuffer(size);
               stagingBuffer.copy(indices.data(), size);
               stagingBuffers.push_back(stagingBuffer);

               auto indexBuffer = device.createBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
                       , VMA_MEMORY_USAGE_GPU_ONLY, size);
               hulls.indices.push_back(indexBuffer);
           }

           // for mode (1)  some hulls have no vertices,
           // so we need to make sure we only processing hulls that have  vertices
           numBuffers = stagingBuffers.size();
           numConvexHulls = numBuffers/2;
           commandPoolCH.oneTimeCommands(numBuffers, [&, stagingBuffers = std::move(stagingBuffers)](auto cIndex, auto commandBuffer){
              auto& stagingBuffer = stagingBuffers[cIndex];
              auto index = cIndex % numConvexHulls;

              auto deviceBuffer = (cIndex < numConvexHulls) ? hulls.vertices[index] : hulls.indices[index];

               VkBufferCopy region{0, 0, stagingBuffer.size};
               vkCmdCopyBuffer(commandBuffer, stagingBuffer, deviceBuffer, 1, &region);

               VkBufferMemoryBarrier barrier = initializers::bufferMemoryBarrier();
               barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
               barrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
               barrier.srcQueueFamilyIndex = *device.queueFamilyIndex.transfer;
               barrier.dstQueueFamilyIndex = *device.queueFamilyIndex.graphics;
               barrier.buffer = deviceBuffer;
               barrier.offset = 0;
               barrier.size = deviceBuffer.size;

               vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT
                                    , 0, 0, VK_NULL_HANDLE, 1, &barrier, 0, VK_NULL_HANDLE);
           });

            spdlog::info("Generated {} convex hulls, containing {} triangles, and {} vertices", numConvexHulls, numTriangles, totalVertices);
           return hulls;
        });

        threadPool.onComplete(fHulls.share(), [this](ConvexHulls hulls){
            int hullIndex = !currentHull;

            convexHulls[hullIndex] = std::move(hulls);
            hullIsReady[hullIndex] = true;

            vkDeviceWaitIdle(device);
            currentHull = hullIndex;
        }, "error constructing convex hulls");
}

void ConvexDecompositionDemo::createFloor() {
    auto flip90Degrees = glm::rotate(glm::mat4{1}, -glm::half_pi<float>(), {1, 0, 0});
    auto plane = primitives::plane(100, 100, 100.0f, 100.0f, flip90Degrees, glm::vec4(1), VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    floor.vertices = device.createDeviceLocalBuffer(plane.vertices.data(), BYTE_SIZE(plane.vertices), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    floor.indices = device.createDeviceLocalBuffer(plane.indices.data(), BYTE_SIZE(plane.indices), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
}

void ConvexDecompositionDemo::renderFloor(VkCommandBufferInheritanceInfo &inheritanceInfo) {
    auto commandBuffer = secondaryCommandBuffers[currentFrame][RENDER_FLOOR];

    VkCommandBufferBeginInfo beginInfo = initializers::commandBufferBeginInfo();
    beginInfo.pInheritanceInfo = &inheritanceInfo;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, floor.pipeline);
    cameraController->push(commandBuffer, floor.layout, glm::mat4(1));
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, floor.vertices, &offset);
    vkCmdBindIndexBuffer(commandBuffer, floor.indices, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(commandBuffer, floor.indices.size/sizeof(uint32_t), 1, 0, 0, 0);

    vkEndCommandBuffer(commandBuffer);
}

void ConvexDecompositionDemo::mirror(VkCommandBufferInheritanceInfo &inheritanceInfo) {
    auto commandBuffer = secondaryCommandBuffers[currentFrame][MIRROR_SCENE];

    VkCommandBufferBeginInfo beginInfo = initializers::commandBufferBeginInfo();
    beginInfo.pInheritanceInfo = &inheritanceInfo;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    if(cameraController->position().y > 0) {
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mirrorRender[models[currentModel].pipeline].pipeline);

        glm::mat4 model = positionModel(models[currentModel], -1) * models[currentModel].transform;
        glm::vec4 color{0.6, 0.6, 0.6, 0.3};

        cameraController->push(commandBuffer, mirrorRender[models[currentModel].pipeline].layout, model);
        vkCmdPushConstants(commandBuffer, mirrorRender[models[currentModel].pipeline].layout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(Camera),
                           sizeof(glm::vec4), &color);
        models[currentModel].drawable.draw(commandBuffer);

        // mirror convex hull
        model = positionModel(models[currentModel]) * models[currentModel].transform;

        if (hullIsReady[currentHull]) {
            auto &convexHulls = this->convexHulls[currentHull];
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mirrorCH.pipeline);

            cameraController->push(commandBuffer, mirrorCH.layout, model);

            VkDeviceSize offset = 0;
            auto numHulls = convexHulls.vertices.size();
            for (int i = 0; i < numHulls; i++) {
                auto &vertexBuffer = convexHulls.vertices[i];
                auto &indexBuffer = convexHulls.indices[i];
                auto &color = convexHulls.colors[i];
                color.a = 0.3f;
                vkCmdPushConstants(commandBuffer, mirrorCH.layout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(Camera),
                                   sizeof(glm::vec4), &color);
                vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffer, &offset);
                vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
                vkCmdDrawIndexed(commandBuffer, indexBuffer.size / sizeof(uint32_t), 1, 0, 0, 0);
            }
        }
    }

    vkEndCommandBuffer(commandBuffer);
}

glm::vec3 ConvexDecompositionDemo::getCameraTarget(Model&  model) {
    auto [min, max] = getBounds(model);
    return (min + max) * 0.5f;
}

void ConvexDecompositionDemo::currentModelChanged() {
    assert(currentModel != nextModel);
    currentModel = nextModel;
    hullIsReady[currentHull] = false;
    updateHACDData();
    constructConvexHull();
    cameraController->position(getCameraTarget(models[currentModel]));
}

glm::mat4 ConvexDecompositionDemo::positionModel(Model &model, int multiplier) {
    auto min = model.drawable.bounds.min;
    auto max = model.drawable.bounds.max;

    auto d = max - min;
    glm::vec3 offset{0};
    if(d.x < d.z){
        offset.x = 0.5f * d.x  + 0.25f * d.x;
    }else{
        offset.x = 0.5f * d.z + 0.25f * d.z;
    }

    offset *= multiplier;
    return glm::translate(glm::mat4(1), offset);
}

std::tuple<glm::vec3, glm::vec3> ConvexDecompositionDemo::getBounds(Model &model) {
    auto d = model.drawable.bounds.max - model.drawable.bounds.min;
    glm::mat4 transform{1};
    transform = positionModel(model, -1) * model.transform;

    glm::vec3 min(1000);
    glm::vec3 max(-1000);

    auto p0 = (transform * glm::vec4(model.drawable.bounds.min, 1)).xyz();
    auto p1 = (transform * glm::vec4(model.drawable.bounds.max, 1)).xyz();

    min = glm::min(min, p0);
    min = glm::min(min, p1);

    max = glm::max(max, p0);
    max = glm::max(max, p1);

    transform = positionModel(model) * model.transform;

    p0 = (transform * glm::vec4(model.drawable.bounds.min, 1)).xyz();
    p1 = (transform * glm::vec4(model.drawable.bounds.max, 1)).xyz();

    min = glm::min(min, p0);
    min = glm::min(min, p1);

    max = glm::max(max, p0);
    max = glm::max(max, p1);

    return std::make_tuple(min, max);
}

int main(){
    try{

        Settings settings;
        settings.depthTest = true;
        settings.height = 1024;
        settings.uniqueQueueFlags |= VK_QUEUE_TRANSFER_BIT;
        settings.enabledFeatures.geometryShader = true;

        auto app = ConvexDecompositionDemo{ settings };
        std::unique_ptr<Plugin> plugin = std::make_unique<ImGuiPlugin>();
        app.addPlugin(plugin);
        app.run();
    }catch(std::runtime_error& err){
        spdlog::error(err.what());
    }
}

void LoggingAdaptor::Log(const char *const msg) {
    spdlog::info(msg);
}


void Callback::Update(const double overallProgress, const double stageProgress, const double operationProgress,
                      const char *const stage, const char *const operation) {
}