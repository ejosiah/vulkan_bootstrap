#include "ConvextDecompositionDemo.hpp"
#include "GraphicsPipelineBuilder.hpp"
#include "DescriptorSetBuilder.hpp"
#include "ImGuiPlugin.hpp"
#include "ThreadPool.hpp"

ConvextDecompositionDemo::ConvextDecompositionDemo(const Settings& settings) : VulkanBaseApp("Hierarchical Approximate Convex Decomposition", settings) {
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

void ConvextDecompositionDemo::initApp() {
    initVHACD();

    createDescriptorPool();
    createCommandPool();
    createPipelineCache();

    createFloor();
    loadBunny();
    updateHACDData();
    constructConvexHull();

    initCamera();
    createRenderPipeline();
}

void ConvextDecompositionDemo::createDescriptorPool() {
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

void ConvextDecompositionDemo::createCommandPool() {
    commandPool = device.createCommandPool(*device.queueFamilyIndex.graphics, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    commandBuffers = commandPool.allocateCommandBuffers(swapChainImageCount);
    secondaryCommandPool = device.createCommandPool(*device.queueFamilyIndex.graphics, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);
    commandPoolCH = device.createCommandPool(*device.queueFamilyIndex.graphics, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);
}

void ConvextDecompositionDemo::createPipelineCache() {
    pipelineCache = device.createPipelineCache();
}


void ConvextDecompositionDemo::createRenderPipeline() {
    auto builder = device.graphicsPipelineBuilder();
    render.pipeline =
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
                .frontFaceClockwise()
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
        .build(render.layout);


    ch.pipeline =
        builder
            .shaderStage()
                .vertexShader(load("render.vert.spv"))
                .fragmentShader(load("render.frag.spv"))
            .rasterizationState()
                .frontFaceCounterClockwise()
            .basePipeline(render.pipeline)
            .name("convex_hull")
        .build(ch.layout);


    mirrorRender.pipeline =
        builder
            .basePipeline(render.pipeline)
            .shaderStage()
                .vertexShader(load("mirror.vert.spv"))
                .fragmentShader(load("render.frag.spv"))
            .rasterizationState()
                .frontFaceCounterClockwise()
            .name("mirror_render")
        .build(mirrorRender.layout);

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
            .basePipeline(render.pipeline)
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
}



void ConvextDecompositionDemo::onSwapChainDispose() {
    dispose(render.pipeline);
}

void ConvextDecompositionDemo::onSwapChainRecreation() {
    createRenderPipeline();
}

VkCommandBuffer *ConvextDecompositionDemo::buildCommandBuffers(uint32_t imageIndex, uint32_t &numCommandBuffers) {
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

    static std::vector<VkCommandBuffer> secondaryCommandBuffers;
    secondaryCommandBuffers.clear();
    VkCommandBuffer sCommandBuffer = VK_NULL_HANDLE;


    sCommandBuffer = renderScene(inheritanceInfo);
    secondaryCommandBuffers.push_back(sCommandBuffer);

    sCommandBuffer = renderConvexHull(inheritanceInfo);
    secondaryCommandBuffers.push_back(sCommandBuffer);

    sCommandBuffer = mirror(inheritanceInfo);
    secondaryCommandBuffers.push_back(sCommandBuffer);

    sCommandBuffer = renderFloor(inheritanceInfo);
    secondaryCommandBuffers.push_back(sCommandBuffer);

    sCommandBuffer = renderUI(inheritanceInfo);
    secondaryCommandBuffers.push_back(sCommandBuffer);

    vkCmdExecuteCommands(commandBuffer, secondaryCommandBuffers.size(), secondaryCommandBuffers.data());

    vkCmdEndRenderPass(commandBuffer);

    vkEndCommandBuffer(commandBuffer);

    return &commandBuffer;
}

void ConvextDecompositionDemo::update(float time) {
    if(!ImGui::IsAnyItemActive()) {
        cameraController->update(time);
    }

    if(shouldUpdateConvexHull()){
        constructConvexHull();
    }

}

void ConvextDecompositionDemo::checkAppInputs() {
    cameraController->processInput();
}

void ConvextDecompositionDemo::cleanup() {
    // TODO save pipeline cache
    VulkanBaseApp::cleanup();
}

void ConvextDecompositionDemo::onPause() {
    VulkanBaseApp::onPause();
}

void ConvextDecompositionDemo::loadBunny() {
    phong::VulkanDrawableInfo info{};
    info.vertexUsage += VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    info.indexUsage += VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    phong::load(resource("bunny.obj"), device, descriptorPool, bunny, info, true);
}

void ConvextDecompositionDemo::initCamera() {
    OrbitingCameraSettings settings{};
    settings.modelHeight =  bunny.height();
    settings.aspectRatio = swapChain.aspectRatio();
    settings.horizontalFov = true;
    settings.fieldOfView = 60.0f;
    settings.orbitMinZoom = 0;
    settings.offsetDistance = bunny.height() * 2;
    settings.orbitMaxZoom = settings.offsetDistance * 10;

    cameraController = std::make_unique<OrbitingCameraController>(device, swapChainImageCount, currentImageIndex
                                                                  , dynamic_cast<InputManager&>(*this), settings);
}

VkCommandBuffer ConvextDecompositionDemo::renderUI(VkCommandBufferInheritanceInfo& inheritanceInfo) {
    auto commandBuffer = secondaryCommandPool.allocate(VK_COMMAND_BUFFER_LEVEL_SECONDARY);

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
    
    updateHulls |= ImGui::Button("update", {100, 20});

    ImGui::End();

    plugin(IM_GUI_PLUGIN).draw(commandBuffer);

    vkEndCommandBuffer(commandBuffer);

    return commandBuffer;
}

void ConvextDecompositionDemo::initVHACD() {
    interfaceVHACD = VHACD::CreateVHACD();
}

void ConvextDecompositionDemo::updateHACDData() {
    for(int i = 0; i < 2; i++) {
        auto& convexHulls = this->convexHulls[i];
        auto vertexBuffer = device.createBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                                VMA_MEMORY_USAGE_CPU_ONLY, bunny.vertexBuffer.size);
        auto indexBuffer = device.createBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                               VMA_MEMORY_USAGE_CPU_ONLY, bunny.indexBuffer.size);

        device.copy(bunny.vertexBuffer, vertexBuffer, bunny.vertexBuffer.size, 0, 0);
        device.copy(bunny.indexBuffer, indexBuffer, bunny.indexBuffer.size, 0, 0);

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

VkCommandBuffer ConvextDecompositionDemo::renderScene(VkCommandBufferInheritanceInfo& inheritanceInfo) {
    auto commandBuffer = secondaryCommandPool.allocate(VK_COMMAND_BUFFER_LEVEL_SECONDARY);

    VkCommandBufferBeginInfo beginInfo = initializers::commandBufferBeginInfo();
    beginInfo.pInheritanceInfo = &inheritanceInfo;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, render.pipeline);

    auto d = bunny.bounds.max - bunny.bounds.min;
    glm::mat4 model{1};
    model = glm::translate(model, {-0.5f * d.x -0.25f * d.x, 0, 0});
    model = glm::rotate(model, -glm::half_pi<float>(), {1, 0, 0});
    model = glm::translate(model, -bunny.bounds.min);
    cameraController->push(commandBuffer, render.layout, model);
    glm::vec4 color{0.6, 0.6, 0.6, 1.0};
    if(cameraController->position().y < 0){
        color.a = 0.3;
    }
    vkCmdPushConstants(commandBuffer, render.layout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(Camera), sizeof(glm::vec4), &color);
    bunny.draw(commandBuffer);

    vkEndCommandBuffer(commandBuffer);
    return commandBuffer;
}

VkCommandBuffer ConvextDecompositionDemo::renderConvexHull(VkCommandBufferInheritanceInfo &inheritanceInfo) {
    auto commandBuffer = secondaryCommandPool.allocate(VK_COMMAND_BUFFER_LEVEL_SECONDARY);
    auto beginInfo = initializers::commandBufferBeginInfo();
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
    beginInfo.pInheritanceInfo = &inheritanceInfo;
    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    if(hullIsReady[currentHull]){
        auto& convexHulls = this->convexHulls[currentHull];
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, ch.pipeline);

        auto d = bunny.bounds.max - bunny.bounds.min;
        glm::mat4 model{1};
        model = glm::translate(model, {0.5f * d.x + 0.25f * d.x, 0, 0});
        model = glm::rotate(model, -glm::half_pi<float>(), {1, 0, 0});
        model = glm::translate(model, -bunny.bounds.min);

        cameraController->push(commandBuffer, ch.layout, model);

        VkDeviceSize offset = 0;
        auto numHulls = convexHulls.vertices.size();
        for(int i = 0; i < numHulls; i++){
            auto& vertexBuffer = convexHulls.vertices[i];
            auto& indexBuffer = convexHulls.indices[i];
            auto& color = convexHulls.colors[i];
            if(cameraController->position().y < 0){
                color.a = 0.3;
            }
            vkCmdPushConstants(commandBuffer, ch.layout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(Camera), sizeof(glm::vec4), &color);
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffer, &offset);
            vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
            vkCmdDrawIndexed(commandBuffer, indexBuffer.size/sizeof(uint32_t), 1, 0, 0, 0);
        }
    }

    vkEndCommandBuffer(commandBuffer);

    return commandBuffer;
}

inline VHACD::IVHACD::Parameters ConvextDecompositionDemo::getParams() {
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

    return parameters;
}

bool ConvextDecompositionDemo::shouldUpdateConvexHull() {
    static decltype(params) cParams = params;
    auto isSame = !std::abs(std::memcmp(&cParams, &params, sizeof(params)));
    if(updateHulls && !isSame){
        updateHulls = false;
        cParams = params;
        return true;
    }
    return false;
}

void ConvextDecompositionDemo::constructConvexHull() {

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
           spdlog::info("Generated {} convex hulls", numConvexHulls);

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
               auto numIndices = ch.m_nTriangles * 3;
               for(int j = 0; j < numIndices; j+=3){
                   auto i0 = ch.m_triangles[j + 0];
                   auto i1 = ch.m_triangles[j + 1];
                   auto i2 = ch.m_triangles[j + 2];

                   auto& v0 = vertices[i0];
                   auto& v1 = vertices[i1];
                   auto& v2 = vertices[i2];

                   // generate normals
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
           commandPoolCH.oneTimeCommands(numBuffers, [&](auto cIndex, auto commandBuffer){
              auto& stagingBuffer = stagingBuffers[cIndex];
              auto index = cIndex % numConvexHulls;

              auto deviceBuffer = (cIndex < numConvexHulls) ? hulls.vertices[index] : hulls.indices[index];

               VkBufferCopy region{0, 0, stagingBuffer.size};
               vkCmdCopyBuffer(commandBuffer, stagingBuffer, deviceBuffer, 1, &region);
           });
           
           return hulls;
        });

        using namespace std::chrono_literals;

//        static bool firstCall = true;
//
//        if(firstCall){
//            firstCall = false;
//            fHulls.wait();
//        }

        threadPool.onComplete(fHulls.share(), [this](ConvexHulls hulls){
            int hullIndex = !currentHull;

            convexHulls[hullIndex] = std::move(hulls);
            hullIsReady[hullIndex] = true;
            currentHull = hullIndex;
        });


}

void ConvextDecompositionDemo::createFloor() {
    auto flip90Degrees = glm::rotate(glm::mat4{1}, -glm::half_pi<float>(), {1, 0, 0});
    auto plane = primitives::plane(100, 100, 100.0f, 100.0f, flip90Degrees, glm::vec4(1), VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    floor.vertices = device.createDeviceLocalBuffer(plane.vertices.data(), BYTE_SIZE(plane.vertices), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    floor.indices = device.createDeviceLocalBuffer(plane.indices.data(), BYTE_SIZE(plane.indices), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
}

VkCommandBuffer ConvextDecompositionDemo::renderFloor(VkCommandBufferInheritanceInfo &inheritanceInfo) {
    auto commandBuffer = secondaryCommandPool.allocate(VK_COMMAND_BUFFER_LEVEL_SECONDARY);

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

    return commandBuffer;
}

VkCommandBuffer ConvextDecompositionDemo::mirror(VkCommandBufferInheritanceInfo &inheritanceInfo) {
    auto commandBuffer = secondaryCommandPool.allocate(VK_COMMAND_BUFFER_LEVEL_SECONDARY);

    VkCommandBufferBeginInfo beginInfo = initializers::commandBufferBeginInfo();
    beginInfo.pInheritanceInfo = &inheritanceInfo;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    if(cameraController->position().y > 0) {
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mirrorRender.pipeline);

        auto d = bunny.bounds.max - bunny.bounds.min;
        glm::mat4 model{1};
        model = glm::translate(model, {-0.5f * d.x - 0.25f * d.x, 0, 0});
        model = glm::rotate(model, -glm::half_pi<float>(), {1, 0, 0});
        model = glm::translate(model, -bunny.bounds.min);
        glm::vec4 color{0.6, 0.6, 0.6, 0.3};

        cameraController->push(commandBuffer, mirrorRender.layout, model);
        vkCmdPushConstants(commandBuffer, mirrorRender.layout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(Camera),
                           sizeof(glm::vec4), &color);
        bunny.draw(commandBuffer);

        if (hullIsReady[currentHull]) {
            auto &convexHulls = this->convexHulls[currentHull];
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mirrorCH.pipeline);

            auto d = bunny.bounds.max - bunny.bounds.min;
            glm::mat4 model{1};
            model = glm::translate(model, {0.5f * d.x + 0.25f * d.x, 0, 0});
            model = glm::rotate(model, -glm::half_pi<float>(), {1, 0, 0});
            model = glm::translate(model, -bunny.bounds.min);

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
    return commandBuffer;
}

int main(){
    try{

        Settings settings;
        settings.depthTest = true;

        auto app = ConvextDecompositionDemo{ settings };
        std::unique_ptr<Plugin> plugin = std::make_unique<ImGuiPlugin>();
        app.addPlugin(plugin);
        app.run();
    }catch(std::runtime_error& err){
        spdlog::error(err.what());
    }
}