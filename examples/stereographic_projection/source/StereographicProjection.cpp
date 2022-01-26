#include "StereographicProjection.hpp"
#include "GraphicsPipelineBuilder.hpp"
#include "DescriptorSetBuilder.hpp"

StereographicProjection::StereographicProjection(const Settings& settings) : VulkanBaseApp("Stereographic projection", settings) {
    fileManager.addSearchPath(".");
    fileManager.addSearchPath("../../examples/stereographic_projection");
    fileManager.addSearchPath("../../examples/stereographic_projection/spv");
    fileManager.addSearchPath("../../examples/stereographic_projection/models");
    fileManager.addSearchPath("../../examples/stereographic_projection/textures");
    fileManager.addSearchPath("../../data/shaders");
    fileManager.addSearchPath("../../data/models");
    fileManager.addSearchPath("../../data/textures");
    fileManager.addSearchPath("../../data");
}

void StereographicProjection::initApp() {
    createDescriptorPool();
    createCommandPool();
    createSphere();
    createProjection();
    initCamera();
    createPipelineCache();
    createRenderPipeline();
    createComputePipeline();
}

void StereographicProjection::createDescriptorPool() {
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

void StereographicProjection::createCommandPool() {
    commandPool = device.createCommandPool(*device.queueFamilyIndex.graphics, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    commandBuffers = commandPool.allocateCommandBuffers(swapChainImageCount);
}

void StereographicProjection::createPipelineCache() {
    pipelineCache = device.createPipelineCache();
}


void StereographicProjection::createRenderPipeline() {
    auto builder = device.graphicsPipelineBuilder();
    render.pipeline =
        builder
            .shaderStage()
                .vertexShader(load("vertex.vert.spv"))
                .fragmentShader(load("checkerboard.frag.spv"))
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

void StereographicProjection::createComputePipeline() {
    auto module = VulkanShaderModule{ "../../data/shaders/pass_through.comp.spv", device};
    auto stage = initializers::shaderStage({ module, VK_SHADER_STAGE_COMPUTE_BIT});

    compute.layout = device.createPipelineLayout();

    auto computeCreateInfo = initializers::computePipelineCreateInfo();
    computeCreateInfo.stage = stage;
    computeCreateInfo.layout = compute.layout;

    compute.pipeline = device.createComputePipeline(computeCreateInfo, pipelineCache);
}


void StereographicProjection::onSwapChainDispose() {
    dispose(render.pipeline);
    dispose(compute.pipeline);
}

void StereographicProjection::onSwapChainRecreation() {
    createRenderPipeline();
    createComputePipeline();
}

VkCommandBuffer *StereographicProjection::buildCommandBuffers(uint32_t imageIndex, uint32_t &numCommandBuffers) {
    numCommandBuffers = 1;
    auto& commandBuffer = commandBuffers[imageIndex];

    VkCommandBufferBeginInfo beginInfo = initializers::commandBufferBeginInfo();
    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    static std::array<VkClearValue, 2> clearValues;
    clearValues[0].color = {0.8, 0.8, 0.8, 1};
    clearValues[1].depthStencil = {1.0, 0u};

    VkRenderPassBeginInfo rPassInfo = initializers::renderPassBeginInfo();
    rPassInfo.clearValueCount = COUNT(clearValues);
    rPassInfo.pClearValues = clearValues.data();
    rPassInfo.framebuffer = framebuffers[imageIndex];
    rPassInfo.renderArea.offset = {0u, 0u};
    rPassInfo.renderArea.extent = swapChain.extent;
    rPassInfo.renderPass = renderPass;

    vkCmdBeginRenderPass(commandBuffer, &rPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, render.pipeline);
    cameraController->push(commandBuffer, render.layout);
    VkDeviceSize offset = 0;
//    vkCmdBindVertexBuffers(commandBuffer, 0, 1, sphere.vertices, &offset);
//    vkCmdBindIndexBuffer(commandBuffer, sphere.indices, 0, VK_INDEX_TYPE_UINT32);
//    vkCmdDrawIndexed(commandBuffer, sphere.indices.size/sizeof(uint32_t), 1, 0, 0, 0);

    cameraController->push(commandBuffer, render.layout);
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, projection.vertices, &offset);
    vkCmdBindIndexBuffer(commandBuffer, projection.indices, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(commandBuffer, projection.indices.size/sizeof(uint32_t), 1, 0, 0, 0);

    vkCmdEndRenderPass(commandBuffer);

    vkEndCommandBuffer(commandBuffer);

    return &commandBuffer;
}

void StereographicProjection::update(float time) {
    cameraController->update(time);
}

void StereographicProjection::checkAppInputs() {
    cameraController->processInput();
}

void StereographicProjection::cleanup() {
    // TODO save pipeline cache
    VulkanBaseApp::cleanup();
}

void StereographicProjection::onPause() {
    VulkanBaseApp::onPause();
}

void StereographicProjection::createSphere() {
    auto s = primitives::sphere(100, 100, 1.0f, glm::mat4(1), glm::vec4(1), VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    sphere.vertices = device.createDeviceLocalBuffer(s.vertices.data(), BYTE_SIZE(s.vertices), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    sphere.indices = device.createDeviceLocalBuffer(s.indices.data(), BYTE_SIZE(s.indices), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
}

void StereographicProjection::createProjection() {
    auto s = primitives::sphere(100, 100, 1.0f, glm::mat4(1), glm::vec4(1), VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    glm::vec3 n{0, 1, 0};
    glm::vec3 o{0, 1, 0};
    for(auto& v : s.vertices){
        auto d = v.position.xyz() - o;
        auto t = -glm::dot(n, o)/glm::dot(n, d);
        auto x = o + d * t;
        if(glm::any(glm::isnan(x))){
            x = glm::vec3(0);
        }
        v.position = glm::vec4(x, 1);
        v.normal *= -1.0f;
    }
    projection.vertices = device.createDeviceLocalBuffer(s.vertices.data(), BYTE_SIZE(s.vertices), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    projection.indices = device.createDeviceLocalBuffer(s.indices.data(), BYTE_SIZE(s.indices), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
}

void StereographicProjection::initCamera() {
    OrbitingCameraSettings settings{};
    settings.modelHeight =  0;
    settings.aspectRatio = swapChain.aspectRatio();
    settings.horizontalFov = true;
    settings.fieldOfView = 60.0f;
    settings.orbitMinZoom = 0;
    settings.offsetDistance = 5;
    settings.orbitMaxZoom =  10;

    cameraController = std::make_unique<OrbitingCameraController>(device, swapChainImageCount, currentImageIndex
            , dynamic_cast<InputManager&>(*this), settings);
}


int main(){
    try{

        Settings settings;
        settings.depthTest = true;

        auto app = StereographicProjection{ settings };
        app.run();
    }catch(std::runtime_error& err){
        spdlog::error(err.what());
    }
}