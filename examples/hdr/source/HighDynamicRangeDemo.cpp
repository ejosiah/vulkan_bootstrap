#include "HighDynamicRangeDemo.hpp"
#include "GraphicsPipelineBuilder.hpp"
#include "DescriptorSetBuilder.hpp"
#include "ImGuiPlugin.hpp"
#include <set>

HighDynamicRangeDemo::HighDynamicRangeDemo(const Settings& settings) : VulkanBaseApp("High dynamic range", settings) {
    fileManager.addSearchPath(".");
    fileManager.addSearchPath("../../examples/hdr");
    fileManager.addSearchPath("../../examples/hdr/spv");
    fileManager.addSearchPath("../../examples/hdr/models");
    fileManager.addSearchPath("../../examples/hdr/textures");
    fileManager.addSearchPath("../../data/shaders");
    fileManager.addSearchPath("../../data/models");
    fileManager.addSearchPath("../../data/textures");
    fileManager.addSearchPath("../../data");
}

void HighDynamicRangeDemo::initApp() {
    createDescriptorPool();
    createDescriptorSetLayout();
    createCommandPool();
    createTunnel();
    initCamera();
    initLights();
    loadTexture();
    updateDescriptorSet();
    createPipelineCache();
    createRenderPipeline();
    createComputePipeline();
}

void HighDynamicRangeDemo::initCamera() {
    FirstPersonSpectatorCameraSettings settings;
    cameraController = std::make_unique<SpectatorCameraController>(device, swapChainImageCount, currentImageIndex
                                                                   , dynamic_cast<InputManager&>(*this), settings);
    cameraController->lookAt({0, 0, 1}, glm::vec3(0, 0, 50), {0, 1, 0});
}

void HighDynamicRangeDemo::initLights() {
    auto light = Light{{0, 0, 49.5}, glm::vec3(200)};
    lights.push_back(light);

    light = {{-1.4, -1.9, 9.0}, glm::vec3(0.1, 0, 0)};
    lights.push_back(light);

    light = { {0, -1.8, 4}, {0, 0, 0.2}};
    lights.push_back(light);

    light = {{0.8, -1.7, 6.0}, {0, 0.1, 0.0f}};
    lights.push_back(light);

    lightBuffer = device.createDeviceLocalBuffer(lights.data(), BYTE_SIZE(lights), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
}

void HighDynamicRangeDemo::createTunnel() {
    auto cube = primitives::cube();

    glm::vec3 scale{2.5, 2.5, 27.5};
    glm::vec3 uvScale{0.5, 0.5, 11};

    std::set<uint32_t> processed;
    for(int i = 0; i < cube.indices.size(); i+= 3){
        auto v0 = cube.vertices[cube.indices[i]].position.xyz();
        auto v1 = cube.vertices[cube.indices[i + 1]].position.xyz();
        auto v2 = cube.vertices[cube.indices[i + 2]].position.xyz();

        auto n = glm::cross(v1 - v0, v2 - v0);
        n = glm::normalize(n);

        for(int j = 0; j < 3; j++){
            auto id = cube.indices[i + j];

            if(processed.find(id) == processed.end()){
                auto& uv = cube.vertices[id].uv;
                if(n.x < 0 || n.x > 0){
                    uv *= uvScale.zy() * 0.5f;
                }else if(n.y < 0 || n.y > 0){
                    uv *= uvScale.xz()  * 0.5f;

                } else if(n.z < 0 || n.z > 0){
                    uv *= uvScale.xy();
                }
                processed.insert(id);
            }
        }
    }

    tunnel.vertices = device.createDeviceLocalBuffer(cube.vertices.data(), BYTE_SIZE(cube.vertices), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    tunnel.indices = device.createDeviceLocalBuffer(cube.indices.data(), BYTE_SIZE(cube.indices), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);

    tunnel.model = glm::translate(glm::mat4(1), {0, 0, 25});
    tunnel.model = glm::scale(tunnel.model, scale * 2.0f);

}

void HighDynamicRangeDemo::createDescriptorPool() {
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

void HighDynamicRangeDemo::createCommandPool() {
    commandPool = device.createCommandPool(*device.queueFamilyIndex.graphics, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    commandBuffers = commandPool.allocateCommandBuffers(swapChainImageCount);
}

void HighDynamicRangeDemo::createPipelineCache() {
    pipelineCache = device.createPipelineCache();
}


void HighDynamicRangeDemo::createRenderPipeline() {
    auto builder = device.graphicsPipelineBuilder();
    render.pipeline =
        builder
            .shaderStage()
                .vertexShader(load("tunnel.vert.spv"))
                .fragmentShader(load("tunnel.frag.spv"))
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
                    .cullNone()
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
                    .addPushConstantRange(VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(Camera), sizeof(displaySettings))
                    .addDescriptorSetLayout(setLayout)
                .renderPass(renderPass)
                .subpass(0)
                .name("render")
                .pipelineCache(pipelineCache)
            .build(render.layout);
}

void HighDynamicRangeDemo::createComputePipeline() {
    auto module = VulkanShaderModule{ "../../data/shaders/pass_through.comp.spv", device};
    auto stage = initializers::shaderStage({ module, VK_SHADER_STAGE_COMPUTE_BIT});

    compute.layout = device.createPipelineLayout();

    auto computeCreateInfo = initializers::computePipelineCreateInfo();
    computeCreateInfo.stage = stage;
    computeCreateInfo.layout = compute.layout;

    compute.pipeline = device.createComputePipeline(computeCreateInfo, pipelineCache);
}


void HighDynamicRangeDemo::onSwapChainDispose() {
    dispose(render.pipeline);
    dispose(compute.pipeline);
}

void HighDynamicRangeDemo::onSwapChainRecreation() {
    createRenderPipeline();
    createComputePipeline();
}

VkCommandBuffer *HighDynamicRangeDemo::buildCommandBuffers(uint32_t imageIndex, uint32_t &numCommandBuffers) {
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

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, render.pipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, render.layout, 0, 1, &descriptorSet, 0, VK_NULL_HANDLE);
    cameraController->push(commandBuffer, render.layout, tunnel.model);
    vkCmdPushConstants(commandBuffer, render.layout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(Camera), sizeof(displaySettings), &displaySettings);
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, tunnel.vertices, &offset);
    vkCmdBindIndexBuffer(commandBuffer, tunnel.indices, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(commandBuffer, tunnel.indices.size/sizeof(uint32_t), 1, 0, 0, 0);

    renderUI(commandBuffer);

    vkCmdEndRenderPass(commandBuffer);

    vkEndCommandBuffer(commandBuffer);

    return &commandBuffer;
}

void HighDynamicRangeDemo::update(float time) {
    if(!ImGui::IsAnyItemActive()) {
        cameraController->update(time);
    }
}

void HighDynamicRangeDemo::checkAppInputs() {
    cameraController->processInput();
}

void HighDynamicRangeDemo::cleanup() {
    // TODO save pipeline cache
    VulkanBaseApp::cleanup();
}

void HighDynamicRangeDemo::onPause() {
    VulkanBaseApp::onPause();
}

void HighDynamicRangeDemo::loadTexture() {

    auto path = fileManager.getFullPath("wood.png");
    assert(path.has_value());
    textures::fromFile(device, woodTexture, path->string(), true, VK_FORMAT_R8G8B8A8_SRGB);
}

void HighDynamicRangeDemo::createDescriptorSetLayout() {
    setLayout =
        device.descriptorSetLayoutBuilder()
            .binding(0)
                .descriptorType(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
                .descriptorCount(1)
                .shaderStages(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)
            .binding(1)
                .descriptorType(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
                .descriptorCount(1)
                .shaderStages(VK_SHADER_STAGE_FRAGMENT_BIT)
        .createLayout();

    descriptorSet = descriptorPool.allocate({ setLayout }).front();
}

void HighDynamicRangeDemo::updateDescriptorSet() {
    auto writes = initializers::writeDescriptorSets<2>();

    writes[0].dstSet = descriptorSet;
    writes[0].dstBinding = 0;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[0].descriptorCount = 1;

    VkDescriptorBufferInfo lightInfo{lightBuffer, 0, VK_WHOLE_SIZE};
    writes[0].pBufferInfo = &lightInfo;

    writes[1].dstSet = descriptorSet;
    writes[1].dstBinding = 1;
    writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[1].descriptorCount = 1;

    VkDescriptorImageInfo imageInfo{woodTexture.sampler, woodTexture.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    writes[1].pImageInfo = &imageInfo;

    device.updateDescriptorSets(writes);
}

void HighDynamicRangeDemo::renderUI(VkCommandBuffer commandBuffer) {
    ImGui::Begin("settings");
    ImGui::SetWindowSize({250, 100});

    static bool gammaOn = false;
    ImGui::Checkbox("gamma correct", &gammaOn);

    static bool hdrOn = false;
    ImGui::Checkbox("hdr", &hdrOn);
    if(hdrOn){
        ImGui::SliderFloat("exposure", &displaySettings.exposure, 0.1f, 5.0f);
    }

    ImGui::End();

    plugin(IM_GUI_PLUGIN).draw(commandBuffer);

    displaySettings.gammaCorrect = static_cast<int>(gammaOn);
    displaySettings.hdrEnabled = static_cast<int>(hdrOn);
}


int main(){
    try{

        Settings settings;
        settings.depthTest = true;
        std::unique_ptr<Plugin> plugin = std::make_unique<ImGuiPlugin>();

        auto app = HighDynamicRangeDemo{ settings };
        app.addPlugin(plugin);
        app.run();
    }catch(std::runtime_error& err){
        spdlog::error(err.what());
    }
}