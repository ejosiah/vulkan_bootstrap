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
    createPlane();
    initCamera();
    updateDescriptorSets(materials[MATERIAL_BRICK]);
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
                .binding(3)
                    .descriptorType(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
                    .descriptorCount(1)
                    .shaderStages(VK_SHADER_STAGE_FRAGMENT_BIT)
                .createLayout();
                    
    materialSet = descriptorPool.allocate({ materialSetLayout }).front();
}

void ParallaxMappingDemo::updateDescriptorSets(const Material& material) {
    auto [width, height, depth] = std::tuple(float(material.distanceMap.width), float(material.distanceMap.height), float(material.distanceMap.depth));
    distanceMapping.constants.normalizationFactor = {depth/width, depth/height, 1};
    auto writes = initializers::writeDescriptorSets<4>();
    
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

    writes[3].dstSet = materialSet;
    writes[3].dstBinding = 3;
    writes[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[3].descriptorCount = 1;
    VkDescriptorImageInfo distInfo{ material.distanceMap.sampler, material.distanceMap.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
    writes[3].pImageInfo = &distInfo;
    
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
    parallaxOcclusion.pipeline =
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
                    .addPushConstantRange(VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(Camera), sizeof(parallaxOcclusion.constants))
                    .addDescriptorSetLayout(materialSetLayout)
                .renderPass(renderPass)
                .subpass(0)
                .name("parallaxOcclusion")
                .pipelineCache(pipelineCache)
            .build(parallaxOcclusion.layout);

    reliefMapping.pipeline =
        builder
            .basePipeline(parallaxOcclusion.pipeline)
            .shaderStage()
                .fragmentShader(load("relief_mapping.frag.spv"))
            .layout().clear()
                .addPushConstantRange(Camera::pushConstant())
                .addPushConstantRange(VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(Camera), sizeof(reliefMapping.constants))
                .addDescriptorSetLayout(materialSetLayout)
            .name("relief_mapping")
        .build(reliefMapping.layout);

    distanceMapping.pipeline =
        builder
            .basePipeline(parallaxOcclusion.pipeline)
            .shaderStage()
                .fragmentShader(load("distance_mapping.frag.spv"))
            .layout().clear()
                .addPushConstantRange(Camera::pushConstant())
                .addPushConstantRange(VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(Camera), sizeof(distanceMapping.constants))
                .addDescriptorSetLayout(materialSetLayout)
            .name("distance_mapping")
        .build(distanceMapping.layout);
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
    dispose(parallaxOcclusion.pipeline);
    dispose(compute.pipeline);
}

void ParallaxMappingDemo::onSwapChainRecreation() {
    createRenderPipeline();
    createComputePipeline();
    cameraController->perspective(swapChain.aspectRatio());
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
    
    Object* object = nullptr;
    if(currentObject == OBJECT_CUBE){
        object = &cube;
    }else{
        object = &plane;
    }

    if(strategy == OCCLUSION) {
        render(commandBuffer, *object, parallaxOcclusion.pipeline, parallaxOcclusion.layout, &parallaxOcclusion.constants,sizeof(parallaxOcclusion.constants));
    }else if(strategy == RELIEF_MAPPING){
        render(commandBuffer, *object, reliefMapping.pipeline, reliefMapping.layout, &reliefMapping.constants,sizeof(reliefMapping.constants));
    }else if(strategy == DISTANCE_FUNCTION){
        render(commandBuffer, *object, distanceMapping.pipeline, distanceMapping.layout, &distanceMapping.constants,sizeof(distanceMapping.constants));
    }

    renderUI(commandBuffer);

    vkCmdEndRenderPass(commandBuffer);

    vkEndCommandBuffer(commandBuffer);

    return &commandBuffer;
}

void ParallaxMappingDemo::render(VkCommandBuffer commandBuffer, Object& object, VkPipeline pipeline, VkPipelineLayout layout,
                                 void *constants, uint32_t constantsSize) {

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    cameraController->push(commandBuffer, layout);
    vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(Camera), constantsSize, constants);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &materialSet, 0, VK_NULL_HANDLE);
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, object.vertexBuffer, &offset);
    vkCmdBindIndexBuffer(commandBuffer, object.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(commandBuffer, object.numIndices, 1, 0, 0, 0);

}

void ParallaxMappingDemo::update(float time) {
    auto title = fmt::format("{} - {} fps", this->title, framePerSecond);
    glfwSetWindowTitle(window, title.c_str());
    if(!ImGui::IsAnyItemActive()){
        cameraController->update(time);
    }

    static int prevMaterial = currentMaterial;
    if(prevMaterial != currentMaterial){
        prevMaterial = currentMaterial;
        if(currentMaterial > MATERIAL_BOX){
            parallaxOcclusion.constants.invertDepthMap = true;
            reliefMapping.constants.invertDepthMap = true;
        }else{
            parallaxOcclusion.constants.invertDepthMap = false;
            reliefMapping.constants.invertDepthMap = false;
        }
        updateDescriptorSets(materials[currentMaterial]);
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
    textures::fromFile(device, materials[MATERIAL_BRICK].albedo, resource("bricks.jpg"));
    textures::fromFile(device, materials[MATERIAL_BRICK].normal, resource("bricks_normal.jpg"));
    textures::fromFile(device, materials[MATERIAL_BRICK].depth, resource("bricks_disp.jpg"));
    createDistanceMap(materials[MATERIAL_BRICK].depth, materials[MATERIAL_BRICK].distanceMap);

    textures::fromFile(device, materials[MATERIAL_BOX].albedo, resource("wood.png"));
    textures::fromFile(device, materials[MATERIAL_BOX].normal, resource("toy_box_normal.png"));
    textures::fromFile(device, materials[MATERIAL_BOX].depth, resource("toy_box_disp.png"));
    createDistanceMap(materials[MATERIAL_BOX].depth, materials[MATERIAL_BOX].distanceMap);
    
    textures::fromFile(device, materials[MATERIAL_MAN_HOLE].albedo, resource("fan-color.png"), true);
    textures::fromFile(device, materials[MATERIAL_MAN_HOLE].normal, resource("fan-normal.png"), true);
    textures::fromFile(device, materials[MATERIAL_MAN_HOLE].depth, resource("fan-height.png"), true);
    createDistanceMap(materials[MATERIAL_MAN_HOLE].depth, materials[MATERIAL_MAN_HOLE].distanceMap, false);
    
    textures::fromFile(device, materials[MATERIAL_TEXT].albedo, resource("text-color.png"), true);
    textures::fromFile(device, materials[MATERIAL_TEXT].normal, resource("text-normal.png"), true);
    textures::fromFile(device, materials[MATERIAL_TEXT].depth, resource("text-height.png"), true);
    createDistanceMap(materials[MATERIAL_TEXT].depth, materials[MATERIAL_TEXT].distanceMap, false);

    textures::fromFile(device, materials[MATERIAL_NV_EYE].albedo, resource("nveye-color.png"), true);
    textures::fromFile(device, materials[MATERIAL_NV_EYE].normal, resource("nveye-normal.png"), true);
    textures::fromFile(device, materials[MATERIAL_NV_EYE].depth, resource("nveye-height.png"), true);
    createDistanceMap(materials[MATERIAL_NV_EYE].depth, materials[MATERIAL_NV_EYE].distanceMap, false);

    textures::fromFile(device, materials[MATERIAL_STONE_BRICK].albedo, resource("stone-color.png"), true);
    textures::fromFile(device, materials[MATERIAL_STONE_BRICK].normal, resource("stone-normal.png"), true);
    textures::fromFile(device, materials[MATERIAL_STONE_BRICK].depth, resource("stone-height.png"), true);
    createDistanceMap(materials[MATERIAL_STONE_BRICK].depth, materials[MATERIAL_STONE_BRICK].distanceMap, false);
}

void ParallaxMappingDemo::createDistanceMap(Texture &depthMap, Texture &distanceMap, bool invert) {
    distanceMap = textures::distanceMap(device, depthMap, distanceMapping.depth, invert);
}

void ParallaxMappingDemo::createCube() {
    auto mesh = primitives::cube();

    cube.vertexBuffer = device.createDeviceLocalBuffer(mesh.vertices.data(), BYTE_SIZE(mesh.vertices), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    cube.indexBuffer = device.createDeviceLocalBuffer(mesh.indices.data(), BYTE_SIZE(mesh.indices), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    cube.numIndices = mesh.indices.size();
}

void ParallaxMappingDemo::createPlane(){
    auto mesh = primitives::plane(4, 4, 1, 1, glm::rotate(glm::mat4(1), glm::radians(270.0f), {1, 0, 0}), glm::vec4(0.6), VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    mesh = primitives::calculateTangents(mesh);
    plane.vertexBuffer = device.createDeviceLocalBuffer(mesh.vertices.data(), BYTE_SIZE(mesh.vertices), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    plane.indexBuffer = device.createDeviceLocalBuffer(mesh.indices.data(), BYTE_SIZE(mesh.indices), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    plane.numIndices = mesh.indices.size();
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
    cameraController = std::unique_ptr<BaseCameraController>{new OrbitingCameraController{ *this, settings}};
}

void ParallaxMappingDemo::renderUI(VkCommandBuffer commandBuffer) {

    ImGui::Begin("Parallax mapping");
    ImGui::SetWindowSize("Parallax mapping", {0, 0});

    static bool visibility = true;
    if(ImGui::CollapsingHeader("Settings"), &visibility, ImGuiTreeNodeFlags_DefaultOpen){
        static bool enabled = true;
        ImGui::Checkbox("Parallax mapping", &enabled);
        if(enabled){
            static const char* items[3] = {"Occlusion Mapping", "Relief Mapping", "Distance Mapping"};
            ImGui::Combo("Strategy", &strategy, items, 3);

            if(strategy == OCCLUSION) {
                ImGui::SliderFloat("height scale", &parallaxOcclusion.constants.heightScale, 0.1, 0.25);
                parallaxOcclusion.constants.enabled = true;
            }else if(strategy == RELIEF_MAPPING){
                ImGui::SliderFloat("depth", &reliefMapping.constants.depth, 0.03, 0.25);
                reliefMapping.constants.enabled = true;
            }else if(strategy == DISTANCE_FUNCTION){
                distanceMapping.constants.enabled = true;
            }
        }else{
            parallaxOcclusion.constants.enabled = false;
            reliefMapping.constants.enabled = false;
            distanceMapping.constants.enabled = false;
        }
    }

    if(ImGui::CollapsingHeader("Object", &visibility, ImGuiTreeNodeFlags_DefaultOpen)){
        ImGui::RadioButton("Box", &currentObject, OBJECT_CUBE); ImGui::SameLine();
        ImGui::RadioButton("Plane", &currentObject, OBJECT_PLANE);

        static const char* cMaterials[6] = {"Bricks", "Box", "Man hole", "text", "NV Eye", "Stone bricks"};
        ImGui::Combo("Material", &currentMaterial, cMaterials, 6);
    }

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