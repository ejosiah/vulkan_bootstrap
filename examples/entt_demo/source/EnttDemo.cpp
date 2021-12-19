#include "EnttDemo.hpp"
#include "GraphicsPipelineBuilder.hpp"
#include "DescriptorSetBuilder.hpp"

EnttDemo::EnttDemo(const Settings& settings) : VulkanBaseApp("entt demo", settings) {
    fileManager.addSearchPath(".");
    fileManager.addSearchPath("../../examples/entt_demo");
    fileManager.addSearchPath("../../examples/entt_demo/spv");
    fileManager.addSearchPath("../../examples/entt_demo/models");
    fileManager.addSearchPath("../../examples/entt_demo/textures");
    fileManager.addSearchPath("../../data/shaders");
    fileManager.addSearchPath("../../data/models");
    fileManager.addSearchPath("../../data/textures");
    fileManager.addSearchPath("../../data");
}

void EnttDemo::initApp() {
    SkyBox::init(this);
    createDescriptorPool();
    createCommandPool();
    initCamera();
    createSkyBox();
    createPipelineCache();
    createRenderPipeline();
    createComputePipeline();
    createCube();
    createCubeInstance({1, 0, 0});
}

void EnttDemo::createDescriptorPool() {
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

void EnttDemo::createCommandPool() {
    commandPool = device.createCommandPool(*device.queueFamilyIndex.graphics, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    commandBuffers = commandPool.allocateCommandBuffers(swapChainImageCount);
}

void EnttDemo::createPipelineCache() {
    pipelineCache = device.createPipelineCache();
}


void EnttDemo::createRenderPipeline() {
    auto builder = device.graphicsPipelineBuilder();
    render.pipeline =
        builder
            .shaderStage()
                .vertexShader(load("render.vert.spv"))
                .fragmentShader(load("render.frag.spv"))
            .vertexInputState()
                .addVertexBindingDescription(0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX)
                .addVertexBindingDescription(1, sizeof(InstanceData), VK_VERTEX_INPUT_RATE_INSTANCE)
                .addVertexAttributeDescription(0, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetOf(Vertex, position))
                .addVertexAttributeDescription(1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetOf(Vertex, normal))
                .addVertexAttributeDescription(2, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetOf(InstanceData, color))
                .addVertexAttributeDescription(3, 1, VK_FORMAT_R32G32B32A32_SFLOAT, offsetOf(InstanceData, transform))
                .addVertexAttributeDescription(4, 1, VK_FORMAT_R32G32B32A32_SFLOAT, offsetOf(InstanceData, transform) + 16)
                .addVertexAttributeDescription(5, 1, VK_FORMAT_R32G32B32A32_SFLOAT, offsetOf(InstanceData, transform) + 32)
                .addVertexAttributeDescription(6, 1, VK_FORMAT_R32G32B32A32_SFLOAT, offsetOf(InstanceData, transform) + 48)
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

void EnttDemo::createComputePipeline() {
    auto module = VulkanShaderModule{ "../../data/shaders/pass_through.comp.spv", device};
    auto stage = initializers::shaderStage({ module, VK_SHADER_STAGE_COMPUTE_BIT});

    compute.layout = device.createPipelineLayout();

    auto computeCreateInfo = initializers::computePipelineCreateInfo();
    computeCreateInfo.stage = stage;
    computeCreateInfo.layout = compute.layout;

    compute.pipeline = device.createComputePipeline(computeCreateInfo, pipelineCache);
}


void EnttDemo::onSwapChainDispose() {
    dispose(render.pipeline);
    dispose(compute.pipeline);
}

void EnttDemo::onSwapChainRecreation() {
    createRenderPipeline();
    createComputePipeline();
}

VkCommandBuffer *EnttDemo::buildCommandBuffers(uint32_t imageIndex, uint32_t &numCommandBuffers) {
    numCommandBuffers = 1;
    auto& commandBuffer = commandBuffers[imageIndex];

    VkCommandBufferBeginInfo beginInfo = initializers::commandBufferBeginInfo();
    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    static std::array<VkClearValue, 2> clearValues;
    clearValues[0].color = {0, 0, 1, 1};
    clearValues[1].depthStencil = {1.0, 0u};

    VkRenderPassBeginInfo rPassInfo = initializers::renderPassBeginInfo();
    rPassInfo.clearValueCount = COUNT(clearValues);
    rPassInfo.pClearValues = clearValues.data();
    rPassInfo.framebuffer = framebuffers[imageIndex];
    rPassInfo.renderArea.offset = {0u, 0u};
    rPassInfo.renderArea.extent = swapChain.extent;
    rPassInfo.renderPass = renderPass;

    vkCmdBeginRenderPass(commandBuffer, &rPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    renderEntities(commandBuffer);

    vkCmdEndRenderPass(commandBuffer);

    vkEndCommandBuffer(commandBuffer);

    return &commandBuffer;
}

void EnttDemo::update(float time) {
    cameraController->update(time);
}

void EnttDemo::checkAppInputs() {
    cameraController->processInput();
}

void EnttDemo::cleanup() {
    // TODO save pipeline cache
    VulkanBaseApp::cleanup();
}

void EnttDemo::onPause() {
    VulkanBaseApp::onPause();
}

void EnttDemo::createCube() {
    cubeEntity = createEntity("cube");
    cubeEntity.add<component::Render>();

    auto cube = primitives::cube();

    cubeBuffers.vertices = device.createDeviceLocalBuffer(cube.vertices.data(), BYTE_SIZE(cube.vertices), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    cubeBuffers.indexes = device.createDeviceLocalBuffer(cube.indices.data(), BYTE_SIZE(cube.indices), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    cubeBuffers.instances = device.createBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, sizeof(InstanceData) * 1000, "cube_xforms");
    auto& renderComponent = cubeEntity.get<component::Render>();
    renderComponent.instanceCount = 0;
    renderComponent.indexCount = cube.indices.size();
    renderComponent.vertexBuffers.push_back(cubeBuffers.vertices);
    renderComponent.vertexBuffers.push_back(cubeBuffers.instances);
    renderComponent.indexBuffer = cubeBuffers.indexes;

    auto indexCount = cube.indices.size();
    auto vertexCount = cube.vertices.size();
    renderComponent.primitives.push_back(vkn::Primitive::indexed(indexCount, 0, vertexCount, 0));

    component::Pipeline pipeline{render.pipeline, render.layout, 0};
    renderComponent.pipelines.push_back(pipeline);
}

void EnttDemo::createCubeInstance(glm::vec3 color, const component::Transform& transform) {
    auto& cubeRender = cubeEntity.get<component::Render>();
    InstanceData* instances = reinterpret_cast<InstanceData*>(cubeBuffers.instances.map());
    instances[cubeRender.instanceCount].transform = transform.get();
    instances[cubeRender.instanceCount].color = color;
    cubeBuffers.instances.unmap();

    auto entity = createEntity(fmt::format("cube_{}", cubeRender.instanceCount));
    entity.add<Cube>();
    entity.add<Color>().value = color;
    entity.get<component::Transform>() = transform;
    cubeRender.instanceCount++;


}

void EnttDemo::initCamera() {
    CameraSettings settings{};
    settings.aspectRatio = static_cast<float>(swapChain.extent.width)/static_cast<float>(swapChain.extent.height);
    settings.horizontalFov = true;
    cameraController = std::make_unique<CameraController>(device, swapChain.imageCount(), currentImageIndex, dynamic_cast<InputManager&>(*this), settings);
    cameraController->setMode(CameraMode::SPECTATOR);
    cameraController->lookAt(glm::vec3(5), {0, 0, 0}, {0, 1, 0});
}

void EnttDemo::createSkyBox() {
    SkyBox::create(skyBox, R"(C:\Users\Josiah\OneDrive\media\textures\skybox\005)"
            , {"right.jpg", "left.jpg", "top.jpg", "bottom.jpg", "front.jpg", "back.jpg"});

    auto entity = createEntity("sky_box");
    auto& renderComponent = entity.add<component::Render>();
    renderComponent.vertexBuffers.push_back(skyBox.cube.vertices);
    renderComponent.indexBuffer = skyBox.cube.indices;

    component::Pipeline pipeline{ *skyBox.pipeline, *skyBox.layout, 0, {}, { skyBox.descriptorSet } };
    renderComponent.pipelines.push_back(pipeline);

    auto indexCount = skyBox.cube.indices.size/sizeof(uint32_t);
    auto vertexCount = skyBox.cube.vertices.size/sizeof(glm::vec3);
    renderComponent.primitives.push_back(vkn::Primitive::indexed(indexCount, 0, vertexCount, 0));
    renderComponent.indexCount = indexCount;
}

void EnttDemo::renderEntities(VkCommandBuffer commandBuffer) {
    auto view = registry.view<const component::Render, const component::Transform>();
    auto camera = cameraController->cam();
    view.each([&](const component::Render& renderComp, const auto& transform){
        if(renderComp.instanceCount > 0) {
            auto model = transform.get();
            std::vector<VkDeviceSize> offsets(renderComp.vertexBuffers.size(), 0);
            vkCmdBindVertexBuffers(commandBuffer, 0, COUNT(renderComp.vertexBuffers), renderComp.vertexBuffers.data(),
                                   offsets.data());
            if (renderComp.indexCount > 0) {
                vkCmdBindIndexBuffer(commandBuffer, renderComp.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
            }
            for (const auto &pipeline : renderComp.pipelines) {
                vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline);
                cameraController->push(commandBuffer, pipeline.layout, model);
                if (!pipeline.descriptorSets.empty()) {
                    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.layout, 0,
                                            COUNT(pipeline.descriptorSets), pipeline.descriptorSets.data(), 0,
                                            VK_NULL_HANDLE);
                }
                for (auto primitive : renderComp.primitives) {
                    if (renderComp.indexCount > 0) {
                        primitive.drawIndexed(commandBuffer, 0, renderComp.instanceCount);
                    } else {
                        primitive.draw(commandBuffer, 0, renderComp.instanceCount);
                    }
                }
            }
        }
    });
}

int main(){
    try{
        Settings settings;
        settings.depthTest = true;

        auto app = EnttDemo{ settings };
        app.run();
    }catch(std::runtime_error& err){
        spdlog::error(err.what());
    }
}