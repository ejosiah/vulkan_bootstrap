#include "PbrDemo.hpp"
#include "GraphicsPipelineBuilder.hpp"
#include "DescriptorSetBuilder.hpp"
#include "ImGuiPlugin.hpp"

PbrDemo::PbrDemo(const Settings& settings) : VulkanBaseApp("Physically based rendering", settings) {
    fileManager.addSearchPath(".");
    fileManager.addSearchPath("../../examples/pbr");
    fileManager.addSearchPath("../../examples/pbr/spv");
    fileManager.addSearchPath("../../examples/pbr/models");
    fileManager.addSearchPath("../../examples/pbr/textures");
    fileManager.addSearchPath("../../data/shaders");
    fileManager.addSearchPath("../../data/shaders/pbr"); // TODO recursive paths
    fileManager.addSearchPath("../../data/models");
    fileManager.addSearchPath("../../data/textures");
    fileManager.addSearchPath("../../data");

    atomicFloatFeatures = VkPhysicalDeviceShaderAtomicFloatFeaturesEXT{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_FLOAT_FEATURES_EXT};
    atomicFloatFeatures.shaderBufferFloat32Atomics = VK_TRUE;
    atomicFloatFeatures.shaderBufferFloat32AtomicAdd = VK_TRUE;
    atomicFloatFeatures.shaderBufferFloat64Atomics = VK_TRUE;
    atomicFloatFeatures.shaderBufferFloat64AtomicAdd = VK_TRUE;
    atomicFloatFeatures.shaderSharedFloat32Atomics = VK_TRUE;
    atomicFloatFeatures.shaderSharedFloat32AtomicAdd = VK_TRUE;
    atomicFloatFeatures.shaderSharedFloat64Atomics = VK_TRUE;
    atomicFloatFeatures.shaderSharedFloat64AtomicAdd = VK_TRUE;
    atomicFloatFeatures.shaderImageFloat32Atomics = VK_TRUE;
    atomicFloatFeatures.shaderImageFloat32AtomicAdd = VK_TRUE;
    atomicFloatFeatures.sparseImageFloat32Atomics = VK_TRUE;
    atomicFloatFeatures.sparseImageFloat32AtomicAdd = VK_TRUE;
    deviceCreateNextChain = &atomicFloatFeatures;
    swapSet = &mapToKey(Key::S, "swap", Action::detectInitialPressOnly());
}

void PbrDemo::initApp() {
    initCamera();
    createDescriptorPool();
    createCommandPool();

    initModel();
    createBRDF_LUT();
    createValueSampler();
    createSRGBSampler();
    loadEnvironmentMap();
    loadMaterials();
    createDescriptorSetLayouts();
    createUboBuffers();
    updateDescriptorSets();
    updatePbrDescriptors(materials[activeMaterial]);
    createPipelineCache();
    createRenderPipeline();
    createComputePipeline();
}

void PbrDemo::createUboBuffers() {
    uboBuffers.mvp = device.createCpuVisibleBuffer(&cameraController->cam(), sizeof(Camera), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    uboBuffers.lights = device.createCpuVisibleBuffer(lights.data(), BYTE_SIZE(lights), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
}

void PbrDemo::createDescriptorPool() {
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

void PbrDemo::createCommandPool() {
    commandPool = device.createCommandPool(*device.queueFamilyIndex.graphics, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    commandBuffers = commandPool.allocateCommandBuffers(swapChainImageCount);
}

void PbrDemo::createPipelineCache() {
    pipelineCache = device.createPipelineCache();
}


void PbrDemo::createRenderPipeline() {
    auto builder = device.graphicsPipelineBuilder();
    environmentMap.pipeline =
        builder
            .allowDerivatives()
            .shaderStage()
                .vertexShader(load("skybox.vert.spv"))
                .fragmentShader(load("environment_map.frag.spv"))
            .vertexInputState()
                .addVertexBindingDescription(0, sizeof(glm::vec3), VK_VERTEX_INPUT_RATE_VERTEX)
                .addVertexAttributeDescription(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0)
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
                    .cullFrontFace()
                    .frontFaceCounterClockwise()
                    .polygonModeFill()
                .multisampleState()
                    .rasterizationSamples(settings.msaaSamples)
                .depthStencilState()
                    .enableDepthWrite()
                    .enableDepthTest()
                    .compareOpLessOrEqual()
                    .minDepthBounds(0)
                    .maxDepthBounds(1)
                .colorBlendState()
                    .attachment()
                    .add()
                .layout()
                    .addDescriptorSetLayout(environmentMap.setLayout)
                    .addPushConstantRange(Camera::pushConstant())
                    .addPushConstantRange(VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(Camera), sizeof(environmentMap.constants))
                .renderPass(renderPass)
                .subpass(0)
                .name("environment_map")
                .pipelineCache(pipelineCache)
            .build(environmentMap.layout);

    render.pipeline =
        builder
            .basePipeline(environmentMap.pipeline)
            .shaderStage()
                .vertexShader(load("pbr.vert.spv"))
                .fragmentShader(load("pbr.frag.spv"))
            .vertexInputState()
                .clear()
                .addVertexBindingDescriptions(Vertex::bindingDisc())
                .addVertexAttributeDescriptions(Vertex::attributeDisc())
            .rasterizationState()
                .cullBackFace()
            .depthStencilState()
                .compareOpLess()
            .colorBlendState()
                .attachments(1)
            .layout()
                .clear()
                .addPushConstantRange(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pbr.constants))
                .addDescriptorSetLayout(uboSetLayouts)
                .addDescriptorSetLayout(pbr.setLayout)
                .addDescriptorSetLayout(environmentMap.setLayout)
            .renderPass(renderPass)
            .subpass(0)
            .name("pbr")
        .build(render.layout);
			
	screenQuad.pipeline =
        builder
            .basePipeline(environmentMap.pipeline)
            .shaderStage()
                .vertexShader(load("quad.vert.spv"))
                .fragmentShader(load("quad.frag.spv"))
            .vertexInputState()
                .clear()
                .addVertexBindingDescriptions(ClipSpace::bindingDescription())
                .addVertexAttributeDescriptions(ClipSpace::attributeDescriptions())
            .inputAssemblyState()
                .triangleStrip()
                .rasterizationState()
                    .cullBackFace()
                .colorBlendState()
                    .attachments(1)
                .layout()
                    .clear()
                    .addDescriptorSetLayout(environmentMapSetLayout)
                .renderPass(renderPass)
                .subpass(0)
                .name("quad")
                .pipelineCache(pipelineCache)
            .build(screenQuad.layout);
}

void PbrDemo::createComputePipeline() {
    auto module = VulkanShaderModule{ "../../data/shaders/pass_through.comp.spv", device};
    auto stage = initializers::shaderStage({ module, VK_SHADER_STAGE_COMPUTE_BIT});

    compute.layout = device.createPipelineLayout();

    auto computeCreateInfo = initializers::computePipelineCreateInfo();
    computeCreateInfo.stage = stage;
    computeCreateInfo.layout = compute.layout;

    compute.pipeline = device.createComputePipeline(computeCreateInfo, pipelineCache);
}


void PbrDemo::onSwapChainDispose() {
    dispose(render.pipeline);
    dispose(environmentMap.pipeline);
    dispose(screenQuad.pipeline);
    dispose(compute.pipeline);
}

void PbrDemo::onSwapChainRecreation() {
    createRenderPipeline();
    createComputePipeline();
}

VkCommandBuffer *PbrDemo::buildCommandBuffers(uint32_t imageIndex, uint32_t &numCommandBuffers) {
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

    renderEnvironment(commandBuffer);
//    drawScreenQuad(commandBuffer);

    renderModel(commandBuffer);
    renderUI(commandBuffer);

    vkCmdEndRenderPass(commandBuffer);

    vkEndCommandBuffer(commandBuffer);

    return &commandBuffer;
}

void PbrDemo::renderUI(VkCommandBuffer commandBuffer) {
    ImGui::Begin("Environment");

    static int oldId = 0;
    oldId = environmentMap.constants.mapId;
    ImGui::SliderInt("", &environmentMap.constants.mapId, 0, environmentMap.numMaps - 1);

    static int selection = 0;
    ImGui::SetWindowSize("Environment", {260, 250});
    ImGui::RadioButton("Environment", &selection, 0);
    ImGui::RadioButton("Irradiance", &selection, 1);
    ImGui::RadioButton("Specular", &selection, 2);
    environmentMap.constants.selection = selection;

    if(selection == 2){
        static int lod = 0;
        if(oldId != environmentMap.constants.mapId){
            lod = 0;
        }
        ImGui::SliderInt("LOD", &lod, 0, 5);
        environmentMap.constants.lod = static_cast<float>(lod);
    }
    ImGui::End();

    bool settings = true;
    ImGui::Begin("Object Properties");

    if(ImGui::CollapsingHeader("Materials", &settings, ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::SetWindowSize("Materials", {150, 250});
        ImGui::RadioButton("Rust Iron", &activeMaterial, 0);
        ImGui::RadioButton("Chrome", &activeMaterial, 1);
        ImGui::RadioButton("Gold", &activeMaterial, 2);
        ImGui::RadioButton("Weaved Metal", &activeMaterial, 3);
        ImGui::RadioButton("Plastic", &activeMaterial, 4);
        ImGui::RadioButton("Leather", &activeMaterial, 5);
        ImGui::RadioButton("Brick", &activeMaterial, 6);
    }

    static bool normalMapping = pbr.constants.normalMapping;
    if(ImGui::CollapsingHeader("settings", &settings, ImGuiTreeNodeFlags_DefaultOpen)){
        ImGui::Checkbox("normal mapping", &normalMapping);
    }
    pbr.constants.normalMapping = normalMapping;

    ImGui::End();

    plugin(IM_GUI_PLUGIN).draw(commandBuffer);
}

void PbrDemo::renderEnvironment(VkCommandBuffer commandBuffer){
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, environmentMap.pipeline);
    cameraController->push(commandBuffer, environmentMap.layout);
    vkCmdPushConstants(commandBuffer, environmentMap.layout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(Camera), sizeof(environmentMap.constants), &environmentMap.constants);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, environmentMap.layout, 0, 1, &environmentMap.descriptorSet, 0, VK_NULL_HANDLE);
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, skybox.vertices, &offset);
    vkCmdBindIndexBuffer(commandBuffer, skybox.indices, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(commandBuffer, skybox.indices.size/sizeof(uint32_t), 1, 0, 0, 0);
}

void PbrDemo::drawScreenQuad(VkCommandBuffer commandBuffer) {
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, screenQuad.pipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, screenQuad.layout, 0, 1, &environmentMap.descriptorSet, 0, VK_NULL_HANDLE);
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, screenQuad.vertices, &offset);
    vkCmdDraw(commandBuffer, 4, 1, 0, 0);

}

void PbrDemo::update(float time) {
    cameraController->update(time);
    uboBuffers.mvp.copy(&cameraController->cam(), sizeof(Camera));

    static int prevActiveMaterial = 0;
    if(prevActiveMaterial != activeMaterial){
        prevActiveMaterial = activeMaterial;
        updatePbrDescriptors(materials[activeMaterial]);
    }
}

void PbrDemo::checkAppInputs() {
    if(swapSet->isPressed()){
        environmentMap.constants.selection = !environmentMap.constants.selection;
    }
    cameraController->processInput();
}

void PbrDemo::cleanup() {
    // TODO save pipeline cache
    VulkanBaseApp::cleanup();
}

void PbrDemo::onPause() {
    VulkanBaseApp::onPause();
}

void PbrDemo::initCamera() {
    OrbitingCameraSettings settings{};
    settings.offsetDistance = 2.0f;
    settings.rotationSpeed = 0.1f;
    settings.zNear = 0.1f;
    settings.zFar = 10.0f;
    settings.fieldOfView = 45.0f;
    settings.modelHeight = 0;
    settings.aspectRatio = static_cast<float>(swapChain.extent.width)/static_cast<float>(swapChain.extent.height);
    cameraController = std::make_unique<OrbitingCameraController>(device, swapChain.imageCount(), currentImageIndex, dynamic_cast<InputManager&>(*this), settings);
}

void PbrDemo::initModel() {
    auto cube = primitives::cube();
    std::vector<glm::vec3> vertices;
    for(auto vertex : cube.vertices){
        vertices.push_back(vertex.position.xyz());
    }
    skybox.vertices = device.createDeviceLocalBuffer(vertices.data(), BYTE_SIZE(vertices), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    skybox.indices = device.createDeviceLocalBuffer(cube.indices.data(), BYTE_SIZE(cube.indices), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);

    auto quad = ClipSpace::Quad::positions;
    screenQuad.vertices = device.createDeviceLocalBuffer(quad.data(), BYTE_SIZE(quad), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);

    phong::load(resource("lte-orb.obj"), device, descriptorPool, model, {}, true, 1);
}

void PbrDemo::loadEnvironmentMap() {
    textures::color(device, placeHolderTexture, glm::vec3(1), {32, 32});

    std::vector<std::string> maps{"old_hall_4k.hdr", "LA_Downtown_Helipad_GoldenHour_3k.hdr", "ocean.jpg"};
    environmentMap.numMaps = maps.size();

    uint32_t width = 2048;
    auto imageInfo = initializers::imageCreateInfo(VK_IMAGE_TYPE_2D, VK_FORMAT_R32G32B32A32_SFLOAT,
                                                   VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, width, width);
    imageInfo.arrayLayers = environmentMap.numMaps;

    auto subResource = DEFAULT_SUB_RANGE;
    subResource.layerCount = environmentMap.numMaps;
    environmentMap.texture.image = device.createImage(imageInfo);
    environmentMap.texture.image.transitionLayout(device.graphicsCommandPool(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, subResource);

    imageInfo.extent = {512, 512, 1};
    environmentMap.irradiance.image = device.createImage(imageInfo);
    environmentMap.irradiance.image.transitionLayout(device.graphicsCommandPool(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, subResource);

    subResource.levelCount = 5;
    imageInfo.extent = {1024, 1024, 1};
    imageInfo.mipLevels = 5;
    environmentMap.specular.image = device.createImage(imageInfo);
    environmentMap.specular.image.transitionLayout(device.graphicsCommandPool(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, subResource);

    VkImageLayout finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    for(int i = 0; i < environmentMap.numMaps; i++) {
        auto path = maps[i];
        auto texture = textures::equirectangularToOctahedralMap(device, resource(path), width, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        Texture irradiance, specular;
        textures::ibl(device, texture, irradiance, specular, finalLayout);

        device.graphicsCommandPool().oneTimeCommand([&](auto commandBuffer){
            VkImageCopy copy{};
            copy.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            copy.srcSubresource.mipLevel = 0;
            copy.srcSubresource.baseArrayLayer = 0;
            copy.srcSubresource.layerCount = 1;
            copy.srcOffset = {0, 0, 0};

            copy.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            copy.dstSubresource.mipLevel = 0;
            copy.dstSubresource.baseArrayLayer = i;
            copy.dstSubresource.layerCount = 1;
            copy.dstOffset = {0, 0, 0};
            copy.extent = {width, width, 1};


            texture.image.transitionLayout(commandBuffer, finalLayout, DEFAULT_SUB_RANGE
                                           , VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_TRANSFER_WRITE_BIT
                                           , VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
            vkCmdCopyImage(commandBuffer, texture.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
                           , environmentMap.texture.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);


            copy.extent = {512, 512, 1};
            vkCmdCopyImage(commandBuffer, irradiance.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
                    , environmentMap.irradiance.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);


            uint32_t levels = 5;
            std::vector<VkImageCopy> copies(levels, copy);
            for(uint32_t level = 0; level < levels; level++){
                copies[level].srcSubresource.mipLevel = level;
                copies[level].dstSubresource.mipLevel = level;
                copies[level].extent = {1024u >> level, 1024u >> level, 1u};
            }
            vkCmdCopyImage(commandBuffer, specular.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
                    , environmentMap.specular.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, COUNT(copies), copies.data());

        });

        if(i == (environmentMap.numMaps - 1)){
            environmentMap.texture.sampler = std::move(texture.sampler);
            environmentMap.irradiance.sampler = std::move(irradiance.sampler);
            environmentMap.specular.sampler = std::move(specular.sampler);
        }
    }

    finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    VkFormat format = VK_FORMAT_R32G32B32A32_SFLOAT;
    VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
    subResource = DEFAULT_SUB_RANGE;
    subResource.layerCount = environmentMap.numMaps;
    environmentMap.texture.image.transitionLayout(device.graphicsCommandPool(), finalLayout, subResource);
    environmentMap.texture.imageView = environmentMap.texture.image.createView(format, viewType, subResource);

    environmentMap.irradiance.image.transitionLayout(device.graphicsCommandPool(), finalLayout, subResource);
    environmentMap.irradiance.imageView = environmentMap.irradiance.image.createView(format, viewType, subResource);

    subResource.levelCount = 5;
    environmentMap.specular.image.transitionLayout(device.graphicsCommandPool(), finalLayout, subResource);
    environmentMap.specular.imageView = environmentMap.specular.image.createView(format, viewType, subResource);

}

void PbrDemo::createBRDF_LUT(){
    brdfLUT = textures::brdf_lut(device);
}

void PbrDemo::createValueSampler() {
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_NEAREST;
    samplerInfo.minFilter = VK_FILTER_NEAREST;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_WHITE;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;

    valueSampler = device.createSampler(samplerInfo);
}

void PbrDemo::createSRGBSampler() {
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

    sRgbSampler = device.createSampler(samplerInfo);
}

void PbrDemo::createDescriptorSetLayouts() {
    environmentMapSetLayout =
        device.descriptorSetLayoutBuilder()
            .binding(0)
                .descriptorType(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
                .descriptorCount(1)
                .shaderStages(VK_SHADER_STAGE_FRAGMENT_BIT)
        .createLayout();

    uboSetLayouts =
        device.descriptorSetLayoutBuilder()
            .binding(0)
                .descriptorType(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
                .descriptorCount(1)
                .shaderStages(VK_SHADER_STAGE_VERTEX_BIT)
            .binding(1)
                .descriptorType(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
                .descriptorCount(1)
                .shaderStages(VK_SHADER_STAGE_VERTEX_BIT)
        .createLayout();

    pbr.setLayout =
        device.descriptorSetLayoutBuilder()
            .binding(0)
                .descriptorType(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
                .descriptorCount(1)
                .shaderStages(VK_SHADER_STAGE_FRAGMENT_BIT)
                .immutableSamplers(sRgbSampler)
            .binding(1)
                .descriptorType(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
                .descriptorCount(1)
                .shaderStages(VK_SHADER_STAGE_FRAGMENT_BIT)
                .immutableSamplers(valueSampler)
            .binding(2)
                .descriptorType(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
                .descriptorCount(1)
                .shaderStages(VK_SHADER_STAGE_FRAGMENT_BIT)
                .immutableSamplers(valueSampler)
            .binding(3)
                .descriptorType(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
                .descriptorCount(1)
                .shaderStages(VK_SHADER_STAGE_FRAGMENT_BIT)
                .immutableSamplers(valueSampler)
            .binding(4)
                .descriptorType(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
                .descriptorCount(1)
                .shaderStages(VK_SHADER_STAGE_FRAGMENT_BIT)
                .immutableSamplers(valueSampler)
            .binding(5)
                .descriptorType(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
                .descriptorCount(1)
                .shaderStages(VK_SHADER_STAGE_FRAGMENT_BIT)
                .immutableSamplers(sRgbSampler)
        .createLayout();

    environmentMap.setLayout =
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

    auto sets = descriptorPool.allocate({ environmentMap.setLayout, uboSetLayouts, pbr.setLayout });
    environmentMap.descriptorSet = sets[0];
    uboDescriptorSet = sets[1];
    pbr.descriptorSet = sets[2];
}

void PbrDemo::updateDescriptorSets() {
    auto writes = initializers::writeDescriptorSets<6>();

    writes[0].dstSet = environmentMap.descriptorSet;
    writes[0].dstBinding = 0;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[0].descriptorCount = 1;
    VkDescriptorImageInfo envMapInfo{environmentMap.texture.sampler, environmentMap.texture.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    writes[0].pImageInfo = &envMapInfo;

    writes[1].dstSet = environmentMap.descriptorSet;
    writes[1].dstBinding = 1;
    writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[1].descriptorCount = 1;
    VkDescriptorImageInfo irrMapInfo{environmentMap.irradiance.sampler, environmentMap.irradiance.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    writes[1].pImageInfo = &irrMapInfo;

    writes[2].dstSet = environmentMap.descriptorSet;
    writes[2].dstBinding = 2;
    writes[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[2].descriptorCount = 1;
    VkDescriptorImageInfo specMapInfo{environmentMap.specular.sampler, environmentMap.specular.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    writes[2].pImageInfo = &specMapInfo;

    writes[3].dstSet = uboDescriptorSet;
    writes[3].dstBinding = 0;
    writes[3].descriptorType =  VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[3].descriptorCount = 1;
    VkDescriptorBufferInfo lightBufferInfo{uboBuffers.lights, 0, VK_WHOLE_SIZE};
    writes[3].pBufferInfo = &lightBufferInfo;

    writes[4].dstSet = uboDescriptorSet;
    writes[4].dstBinding = 1;
    writes[4].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writes[4].descriptorCount = 1;
    VkDescriptorBufferInfo mvpBufferInfo{uboBuffers.mvp, 0, VK_WHOLE_SIZE};
    writes[4].pBufferInfo = &mvpBufferInfo;

    writes[5].dstSet = pbr.descriptorSet;
    writes[5].dstBinding = 5;
    writes[5].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[5].descriptorCount = 1;
    VkDescriptorImageInfo brdfLutImageInfo{VK_NULL_HANDLE, brdfLUT.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    writes[5].pImageInfo = &brdfLutImageInfo;

    device.updateDescriptorSets(writes);

}

void PbrDemo::updatePbrDescriptors(const Material& material) {
    auto writes = initializers::writeDescriptorSets<5>();

    writes[0].dstSet = pbr.descriptorSet;
    writes[0].dstBinding = 0;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[0].descriptorCount = 1;
    VkDescriptorImageInfo albedoImageInfo{ VK_NULL_HANDLE, material.albedo.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    writes[0].pImageInfo = &albedoImageInfo;

    writes[1].dstSet = pbr.descriptorSet;
    writes[1].dstBinding = 1;
    writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[1].descriptorCount = 1;
    VkDescriptorImageInfo metallicImageInfo{ VK_NULL_HANDLE, material.metalness.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    writes[1].pImageInfo = &metallicImageInfo;

    writes[2].dstSet = pbr.descriptorSet;
    writes[2].dstBinding = 2;
    writes[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[2].descriptorCount = 1;
    VkDescriptorImageInfo roughnessImageInfo{ VK_NULL_HANDLE, material.roughness.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    writes[2].pImageInfo = &roughnessImageInfo;

    writes[3].dstSet = pbr.descriptorSet;
    writes[3].dstBinding = 3;
    writes[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[3].descriptorCount = 1;
    VkDescriptorImageInfo normalImageInfo{ VK_NULL_HANDLE, material.normal.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    writes[3].pImageInfo = &normalImageInfo;

    writes[4].dstSet = pbr.descriptorSet;
    writes[4].dstBinding = 4;
    writes[4].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[4].descriptorCount = 1;
    VkDescriptorImageInfo aoImageInfo{ VK_NULL_HANDLE, material.ao.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    writes[4].pImageInfo = &aoImageInfo;

    device.updateDescriptorSets(writes);
}

void PbrDemo::renderModel(VkCommandBuffer commandBuffer) {
    static std::array<VkDescriptorSet, 3> sets{
        uboDescriptorSet, pbr.descriptorSet, environmentMap.descriptorSet
    };
    pbr.constants.mapId = environmentMap.constants.mapId;
    pbr.constants.invertRoughness = materials[activeMaterial].invertRoughness;
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, render.pipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, render.layout, 0, COUNT(sets), sets.data(), 0, VK_NULL_HANDLE);
    vkCmdPushConstants(commandBuffer, render.layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pbr.constants), &pbr.constants);
    model.draw(commandBuffer);
}

void PbrDemo::loadMaterials() {

    auto rustedIronPath = resource("textures/materials/rusted_iron");
    textures::fromFile(device, materials[0].albedo, rustedIronPath + "/albedo.png", VK_FORMAT_R8G8B8A8_SRGB);
    textures::fromFile(device, materials[0].metalness, rustedIronPath + "/metallic.png");
    textures::fromFile(device, materials[0].normal, rustedIronPath + "/normal.png");
    textures::fromFile(device, materials[0].roughness, rustedIronPath + "/roughness.png");
    textures::fromFile(device, materials[0].ao, rustedIronPath + "/ao.png");

    auto chromePath = resource("textures/materials/chrome");
    textures::color(device, materials[1].albedo, glm::vec3(0.8), {256, 256});
    textures::fromFile(device, materials[1].metalness, chromePath + "/metallic.jpg");
    textures::fromFile(device, materials[1].normal, chromePath + "/normal.jpg");
    textures::fromFile(device, materials[1].roughness, chromePath + "/gloss.jpg");
    textures::fromFile(device, materials[1].ao, chromePath + "/ao.jpg");
    materials[1].invertRoughness = true;

    auto goldIronPath = resource("textures/materials/gold");
    textures::fromFile(device, materials[2].albedo, goldIronPath + "/albedo.png", VK_FORMAT_R8G8B8A8_SRGB);
    textures::fromFile(device, materials[2].metalness, goldIronPath + "/metallic.png");
    textures::fromFile(device, materials[2].normal, goldIronPath + "/normal.png");
    textures::fromFile(device, materials[2].roughness, goldIronPath + "/roughness.png");
    textures::fromFile(device, materials[2].ao, goldIronPath + "/ao.png");

    auto weavedMetalPath = resource("textures/materials/weavedMetal");
    textures::color(device, materials[3].albedo, glm::vec3(0.8), {256, 256});
    textures::fromFile(device, materials[3].metalness, weavedMetalPath + "/metallic.jpg");
    textures::fromFile(device, materials[3].normal, weavedMetalPath + "/normal.jpg");
    textures::fromFile(device, materials[3].roughness, weavedMetalPath + "/gloss.jpg");
    textures::fromFile(device, materials[3].ao, weavedMetalPath + "/ao.jpg");
    materials[3].invertRoughness = true;


    auto plasticPath = resource("textures/materials/plastic");
    textures::fromFile(device, materials[4].albedo, plasticPath + "/albedo.png", VK_FORMAT_R8G8B8A8_SRGB);
    textures::fromFile(device, materials[4].metalness, plasticPath + "/metallic.png");
    textures::fromFile(device, materials[4].normal, plasticPath + "/normal.png");
    textures::fromFile(device, materials[4].roughness, plasticPath + "/roughness.png");
    textures::fromFile(device, materials[4].ao, plasticPath + "/ao.png");

    auto leatherPath = resource("textures/materials/leather");
    textures::fromFile(device, materials[5].albedo, leatherPath + "/albedo.jpg", VK_FORMAT_R8G8B8A8_SRGB);
    textures::color(device, materials[5].metalness, glm::vec3(0), {256, 256});
    textures::fromFile(device, materials[5].normal, leatherPath + "/normal.jpg");
    textures::fromFile(device, materials[5].roughness, leatherPath + "/gloss.jpg");
    textures::fromFile(device, materials[5].ao, leatherPath + "/ao.jpg");
    materials[5].invertRoughness = true;

    auto brickPath = resource("textures/materials/brick");
    textures::fromFile(device, materials[6].albedo, brickPath + "/albedo.jpg", VK_FORMAT_R8G8B8A8_SRGB);
    textures::color(device, materials[6].metalness, glm::vec3(0), {256, 256});
    textures::fromFile(device, materials[6].normal, brickPath + "/normal.jpg");
    textures::fromFile(device, materials[6].roughness, brickPath + "/gloss.jpg");
    textures::fromFile(device, materials[6].ao, brickPath + "/ao.jpg");
    materials[6].invertRoughness = true;


}


int main(){
    try{

        Settings settings;
        settings.depthTest = true;
        settings.deviceExtensions.push_back("VK_EXT_shader_atomic_float");

        auto app = PbrDemo{ settings };
        std::unique_ptr<Plugin> plugin = std::make_unique<ImGuiPlugin>();
        app.addPlugin(plugin);
        app.run();
    }catch(std::runtime_error& err){
        spdlog::error(err.what());
    }
}