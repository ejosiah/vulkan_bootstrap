#include "ParallaxMappingDemo.hpp"
#include "GraphicsPipelineBuilder.hpp"
#include "DescriptorSetBuilder.hpp"
#include "ImGuiPlugin.hpp"

ParallaxMappingDemo::ParallaxMappingDemo(const Settings& settings) : VulkanBaseApp("Parallax Mapping", settings) {
    fileManager.addSearchPath(".");
    fileManager.addSearchPath("../../examples/parallax_mapping");
    fileManager.addSearchPath("../../examples/parallax_mapping/spv");
    fileManager.addSearchPath("../../examples/parallax_mapping/models");
    fileManager.addSearchPath("../../examples/parallax_mapping/textures");
    fileManager.addSearchPath("../../data/shaders");
    fileManager.addSearchPath("../../data/models");
    fileManager.addSearchPath("../../data/textures");
    fileManager.addSearchPath("../../data");
}

void ParallaxMappingDemo::initApp() {
    createDescriptorPool();
    createDescriptorSetLayout();
    createCommandPool();
    createPipelineCache();

    loadMaterial();
    createCube();
    initCamera();
    updateDescriptorSets();
    createRenderPipeline();
    createComputePipeline();
}

void ParallaxMappingDemo::createDescriptorPool() {
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

void ParallaxMappingDemo::createDescriptorSetLayout() {
    materialSetLayout = 
            device.descriptorSetLayoutBuilder()
                .binding(0)
                    .descriptorType(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
                    .descriptorCount(1)
                    .shaderStages(VK_SHADER_STAGE_FRAGMENT_BIT)
                .binding(1)
                    .descriptorType(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
                    .descriptorCount(1)
                    .shaderStages(VK_SHADER_STAGE_FRAGMENT_BIT)
                .binding(2)
                    .descriptorType(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
                    .descriptorCount(1)
                    .shaderStages(VK_SHADER_STAGE_FRAGMENT_BIT)
                .createLayout();
                    
    materialSet = descriptorPool.allocate({ materialSetLayout }).front();
}

void ParallaxMappingDemo::updateDescriptorSets() {
    auto writes = initializers::writeDescriptorSets<3>();
    
    writes[0].dstSet = materialSet;
    writes[0].dstBinding = 0;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[0].descriptorCount = 1;
    VkDescriptorImageInfo albedoInfo{ material.albedo.sampler, material.albedo.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
    writes[0].pImageInfo = &albedoInfo;
    
    writes[1].dstSet = materialSet;
    writes[1].dstBinding = 1;
    writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[1].descriptorCount = 1;
    VkDescriptorImageInfo normalInfo{ material.normal.sampler, material.normal.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
    writes[1].pImageInfo = &normalInfo;
    
    writes[2].dstSet = materialSet;
    writes[2].dstBinding = 2;
    writes[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[2].descriptorCount = 1;
    VkDescriptorImageInfo depthInfo{ material.depth.sampler, material.depth.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
    writes[2].pImageInfo = &depthInfo;
    
    device.updateDescriptorSets(writes);
}

void ParallaxMappingDemo::createCommandPool() {
    commandPool = device.createCommandPool(*device.queueFamilyIndex.graphics, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    commandBuffers = commandPool.allocateCommandBuffers(swapChainImageCount);
}

void ParallaxMappingDemo::createPipelineCache() {
    pipelineCache = device.createPipelineCache();
}


void ParallaxMappingDemo::createRenderPipeline() {
    auto builder = device.graphicsPipelineBuilder();
    render.pipeline =
        builder
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
                    .addPushConstantRange(VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(Camera), sizeof(heightScale))
                    .addDescriptorSetLayout(materialSetLayout)
                .renderPass(renderPass)
                .subpass(0)
                .name("render")
                .pipelineCache(pipelineCache)
            .build(render.layout);
}

void ParallaxMappingDemo::createComputePipeline() {
    auto module = VulkanShaderModule{ "../../data/shaders/pass_through.comp.spv", device};
    auto stage = initializers::shaderStage({ module, VK_SHADER_STAGE_COMPUTE_BIT});

    compute.layout = device.createPipelineLayout();

    auto computeCreateInfo = initializers::computePipelineCreateInfo();
    computeCreateInfo.stage = stage;
    computeCreateInfo.layout = compute.layout;

    compute.pipeline = device.createComputePipeline(computeCreateInfo, pipelineCache);
}


void ParallaxMappingDemo::onSwapChainDispose() {
    dispose(render.pipeline);
    dispose(compute.pipeline);
}

void ParallaxMappingDemo::onSwapChainRecreation() {
    createRenderPipeline();
    createComputePipeline();
}

VkCommandBuffer *ParallaxMappingDemo::buildCommandBuffers(uint32_t imageIndex, uint32_t &numCommandBuffers) {
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

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, render.pipeline);
    cameraController->push(commandBuffer, render.layout);
    vkCmdPushConstants(commandBuffer, render.layout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(Camera), sizeof(heightScale), &heightScale);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, render.layout, 0, 1, &materialSet, 0, VK_NULL_HANDLE);
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, cube.vertexBuffer, &offset);
    vkCmdBindIndexBuffer(commandBuffer, cube.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(commandBuffer, cube.numIndices, 1, 0, 0, 0);

    renderUI(commandBuffer);

    vkCmdEndRenderPass(commandBuffer);

    vkEndCommandBuffer(commandBuffer);

    return &commandBuffer;
}

void ParallaxMappingDemo::update(float time) {
    if(!ImGui::IsAnyItemActive()){
        cameraController->update(time);
    }
}

void ParallaxMappingDemo::checkAppInputs() {
    cameraController->processInput();
}

void ParallaxMappingDemo::cleanup() {
    // TODO save pipeline cache
    VulkanBaseApp::cleanup();
}

void ParallaxMappingDemo::onPause() {
    VulkanBaseApp::onPause();
}

void ParallaxMappingDemo::loadMaterial() {
    textures::fromFile(device, material.albedo, resource("bricks.jpg"));
    textures::fromFile(device, material.normal, resource("bricks_normal.jpg"));
    textures::fromFile(device, material.depth, resource("bricks_disp.jpg"));
}

void ParallaxMappingDemo::createCube() {
    auto mesh = primitives::cube();

    cube.vertexBuffer = device.createDeviceLocalBuffer(mesh.vertices.data(), BYTE_SIZE(mesh.vertices), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    cube.indexBuffer = device.createDeviceLocalBuffer(mesh.indices.data(), BYTE_SIZE(mesh.indices), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    cube.numIndices = mesh.indices.size();
}

void ParallaxMappingDemo::initCamera() {
    OrbitingCameraSettings settings{};
    settings.offsetDistance = 2.0f;
    settings.rotationSpeed = 0.1f;
    settings.zNear = 0.1f;
    settings.zFar = 10.0f;
    settings.fieldOfView = 45.0f;
    settings.modelHeight = 0;
    settings.aspectRatio = static_cast<float>(swapChain.extent.width)/static_cast<float>(swapChain.extent.height);
    cameraController = std::unique_ptr<BaseCameraController>{new OrbitingCameraController{ device, swapChain.imageCount(), currentImageIndex, *this, settings}};
}

void ParallaxMappingDemo::renderUI(VkCommandBuffer commandBuffer) {

    ImGui::Begin("Options");
    ImGui::SetWindowSize("Options", {200, 80});
    ImGui::SliderFloat("height scale", &heightScale, 0.1, 1.0);
    ImGui::End();

    plugin<ImGuiPlugin>(IM_GUI_PLUGIN).draw(commandBuffer);
}


int main(){
    try{

        Settings settings;
        settings.width = 1080;
        settings.height = 720;
        settings.depthTest = true;

        auto app = ParallaxMappingDemo{ settings };
        std::unique_ptr<Plugin> plugin = std::make_unique<ImGuiPlugin>();
        app.addPlugin(plugin);
        app.run();
    }catch(std::runtime_error& err){
        spdlog::error(err.what());
    }
}