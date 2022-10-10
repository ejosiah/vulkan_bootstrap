#include "ShaderBindingTableDemo.hpp"
#include "GraphicsPipelineBuilder.hpp"
#include "DescriptorSetBuilder.hpp"

ShaderBindingTableDemo::ShaderBindingTableDemo(const Settings& settings) : VulkanRayTraceBaseApp("shader binding table", settings) {
    fileManager.addSearchPath(".");
    fileManager.addSearchPath("../../examples/shader_binding_table");
    fileManager.addSearchPath("../../examples/shader_binding_table/spv");
    fileManager.addSearchPath("../../examples/shader_binding_table/models");
    fileManager.addSearchPath("../../examples/shader_binding_table/textures");
    fileManager.addSearchPath("../../data/shaders");
    fileManager.addSearchPath("../../data/models");
    fileManager.addSearchPath("../../data/textures");
    fileManager.addSearchPath("../../data");
}

void ShaderBindingTableDemo::initApp() {
    initCanvas();
    initCamera();
    createDescriptorPool();
    loadModels();
    createInverseCam();
    createDescriptorSetLayouts();
    updateDescriptorSets();
    createCommandPool();
    createPipelineCache();
    createRenderPipeline();
    createRayTracingPipeline();
}

void ShaderBindingTableDemo::initCanvas() {
    canvas = Canvas{ this, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_FORMAT_R8G8B8A8_UNORM};
    canvas.init();
    std::vector<unsigned char> checkerboard(width * height * 4);
    textures::checkerboard1(checkerboard.data(), {width, height});
    const auto stagingBuffer = device.createCpuVisibleBuffer(checkerboard.data(), BYTE_SIZE(checkerboard), VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
    device.graphicsCommandPool().oneTimeCommand([&](auto commandBuffer){
       textures::transfer(commandBuffer, stagingBuffer, canvas.image, {width, height}, VK_IMAGE_LAYOUT_GENERAL);
    });
}

void ShaderBindingTableDemo::loadModels() {
    std::vector<rt::MeshObjectInstance> instances;
    VulkanDrawable bunny;
    loadBunny();

    rt::MeshObjectInstance bunnyInstance{};
    bunnyInstance.object = rt::TriangleMesh{ &drawables["bunny"]};
    bunnyInstance.hitGroupId = 0;
    bunnyInstance.xform = glm::rotate(glm::mat4(1), -glm::half_pi<float>(), {1, 0, 0});
    bunnyInstance.xform = glm::translate(bunnyInstance.xform, -drawables["bunny"].bounds.min);
    bunnyInstance.xformIT = glm::inverseTranspose(bunnyInstance.xform);
    instances.push_back(bunnyInstance);

    bunnyInstance.hitGroupId = 1;
    bunnyInstance.xform = glm::translate(bunnyInstance.xform, glm::vec3(1, 0, 0));
    instances.push_back(bunnyInstance);
    createAccelerationStructure(instances);
}

void ShaderBindingTableDemo::loadBunny() {
    phong::VulkanDrawableInfo info{};
    info.vertexUsage |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    info.indexUsage |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    info.materialUsage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    info.materialIdUsage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    info.generateMaterialId = true;
    info.vertexUsage += VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    info.indexUsage += VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    VulkanDrawable drawable;
    phong::load(resource("bunny.obj"), device, descriptorPool, drawable, info, true, 1);

    // bunny is Left handed so we need to flip its normals
    auto vertexBuffer = device.createBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                            VMA_MEMORY_USAGE_CPU_ONLY, drawable.vertexBuffer.size);

    device.copy(drawable.vertexBuffer, vertexBuffer, drawable.vertexBuffer.size, 0, 0);

    auto vertices = reinterpret_cast<Vertex*>(vertexBuffer.map());
    auto size = vertexBuffer.size/sizeof(Vertex);
    for(int i = 0; i < size; i++){
        vertices[i].normal *= -1;
    }
    vertexBuffer.unmap();
    device.copy(vertexBuffer, drawable.vertexBuffer, vertexBuffer.size);
    drawables.insert(std::make_pair("bunny", std::move(drawable)));
}

void ShaderBindingTableDemo::createDescriptorPool() {
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

void ShaderBindingTableDemo::createDescriptorSetLayouts() {
    raytrace.descriptorSetLayout =
        device.descriptorSetLayoutBuilder()
            .name("raytrace_base")
            .binding(0)
                .descriptorType(VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR)
                .descriptorCount(1)
                .shaderStages(VK_SHADER_STAGE_RAYGEN_BIT_KHR)
            .binding(1)
                .descriptorType(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
                .descriptorCount(1)
                .shaderStages(VK_SHADER_STAGE_RAYGEN_BIT_KHR)
            .binding(2)
                .descriptorType(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
                .descriptorCount(1)
                .shaderStages(VK_SHADER_STAGE_RAYGEN_BIT_KHR)
        .createLayout();

    raytrace.instanceDescriptorSetLayout =
        device.descriptorSetLayoutBuilder()
            .name("raytrace_instance")
            .binding(0) // materials
                .descriptorType(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
                .descriptorCount(1)
                .shaderStages(VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR)
            .binding(1) // material ids
                .descriptorType(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
                .descriptorCount(1)
                .shaderStages(VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR)
            .binding(2) // scene objects
                .descriptorType(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
                .descriptorCount(1)
                .shaderStages(VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR)
        .createLayout();

    raytrace.vertexDescriptorSetLayout =
        device.descriptorSetLayoutBuilder()
            .name("raytrace_vertex")
            .binding(0) // vertex buffer binding
                .descriptorType(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
                .descriptorCount(1)
                .shaderStages(VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR)
            .binding(1)     // index buffer bindings
                .descriptorType(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
                .descriptorCount(1)
                .shaderStages(VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR)
            .binding(2)     // vertex offset buffer
                .descriptorType(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
                .descriptorCount(1)
                .shaderStages(VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR)
        .createLayout();
}

void ShaderBindingTableDemo::updateDescriptorSets() {
    auto sets = descriptorPool.allocate( { raytrace.descriptorSetLayout, raytrace.instanceDescriptorSetLayout, raytrace.vertexDescriptorSetLayout });
    raytrace.descriptorSet = sets[0];
    device.setName<VK_OBJECT_TYPE_DESCRIPTOR_SET>("raytrace_base", raytrace.descriptorSet);

    raytrace.instanceDescriptorSet = sets[1];
    device.setName<VK_OBJECT_TYPE_DESCRIPTOR_SET>("raytrace_instance", raytrace.instanceDescriptorSet);

    raytrace.vertexDescriptorSet = sets[2];
    device.setName<VK_OBJECT_TYPE_DESCRIPTOR_SET>("raytrace_vertex", raytrace.vertexDescriptorSet);

    auto writes = initializers::writeDescriptorSets<9>();
    
    VkWriteDescriptorSetAccelerationStructureKHR asWrites{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR};
    asWrites.accelerationStructureCount = 1;
    asWrites.pAccelerationStructures =  rtBuilder.accelerationStructure();
    writes[0].pNext = &asWrites;
    writes[0].dstSet = raytrace.descriptorSet;
    writes[0].dstBinding = 0;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
    writes[0].descriptorCount = 1;
    
    writes[1].dstSet = raytrace.descriptorSet;
    writes[1].dstBinding = 1;
    writes[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writes[1].descriptorCount = 1;
    VkDescriptorBufferInfo camInfo{ inverseCamProj, 0, VK_WHOLE_SIZE};
    writes[1].pBufferInfo = &camInfo;
    
    writes[2].dstSet = raytrace.descriptorSet;
    writes[2].dstBinding = 2;
    writes[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    writes[2].descriptorCount = 1;
    VkDescriptorImageInfo imageInfo{ VK_NULL_HANDLE, canvas.imageView, VK_IMAGE_LAYOUT_GENERAL};
    writes[2].pImageInfo = &imageInfo;

    std::array<VkDescriptorBufferInfo, 1> materialBufferInfo{};
    materialBufferInfo[0].buffer = drawables["bunny"].materialBuffer;
    materialBufferInfo[0].offset = 0;
    materialBufferInfo[0].range = VK_WHOLE_SIZE;

    writes[3].dstSet = raytrace.instanceDescriptorSet;
    writes[3].dstBinding = 0;
    writes[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[3].descriptorCount = COUNT(materialBufferInfo);
    writes[3].pBufferInfo = materialBufferInfo.data();

    // instance descriptorSet
    std::array<VkDescriptorBufferInfo, 1> matIdBufferInfo{};
    matIdBufferInfo[0].buffer = drawables["bunny"].materialIdBuffer;
    matIdBufferInfo[0].offset = 0;
    matIdBufferInfo[0].range = VK_WHOLE_SIZE;

    writes[4].dstSet = raytrace.instanceDescriptorSet;
    writes[4].dstBinding = 1;
    writes[4].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[4].descriptorCount = COUNT(matIdBufferInfo);
    writes[4].pBufferInfo = matIdBufferInfo.data();

    VkDescriptorBufferInfo sceneBufferInfo{};
    sceneBufferInfo.buffer = sceneObjectBuffer;
    sceneBufferInfo.offset = 0;
    sceneBufferInfo.range = VK_WHOLE_SIZE;

    writes[5].dstSet= raytrace.instanceDescriptorSet;
    writes[5].dstBinding = 2;
    writes[5].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[5].descriptorCount = 1;
    writes[5].pBufferInfo = &sceneBufferInfo;

    // vertex descriptorSet
    std::array<VkDescriptorBufferInfo, 1> vertexBufferInfo{};
    vertexBufferInfo[0].buffer = drawables["bunny"].vertexBuffer;
    vertexBufferInfo[0].offset = 0;
    vertexBufferInfo[0].range = VK_WHOLE_SIZE;

    writes[6].dstSet = raytrace.vertexDescriptorSet;
    writes[6].dstBinding = 0;
    writes[6].descriptorCount = COUNT(vertexBufferInfo);
    writes[6].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[6].pBufferInfo = vertexBufferInfo.data();

    std::array<VkDescriptorBufferInfo, 1> indexBufferInfo{};
    indexBufferInfo[0].buffer = drawables["bunny"].indexBuffer;
    indexBufferInfo[0].offset = 0;
    indexBufferInfo[0].range = VK_WHOLE_SIZE;

    writes[7].dstSet = raytrace.vertexDescriptorSet;
    writes[7].dstBinding = 1;
    writes[7].descriptorCount = COUNT(indexBufferInfo);
    writes[7].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[7].pBufferInfo = indexBufferInfo.data();

    std::array<VkDescriptorBufferInfo, 1> offsetBufferInfo{};
    offsetBufferInfo[0].buffer = drawables["bunny"].offsetBuffer;
    offsetBufferInfo[0].offset = 0;
    offsetBufferInfo[0].range = VK_WHOLE_SIZE;

    writes[8].dstSet = raytrace.vertexDescriptorSet;
    writes[8].dstBinding = 2;
    writes[8].descriptorCount = COUNT(offsetBufferInfo);
    writes[8].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[8].pBufferInfo = offsetBufferInfo.data();

    device.updateDescriptorSets(writes);
}

void ShaderBindingTableDemo::initCamera() {
    OrbitingCameraSettings cameraSettings;
//    FirstPersonSpectatorCameraSettings cameraSettings;
    cameraSettings.orbitMinZoom = 0.1;
    cameraSettings.orbitMaxZoom = 512.0f;
    cameraSettings.offsetDistance = 1.0f;
    cameraSettings.modelHeight = 0.5;
    cameraSettings.fieldOfView = 60.0f;
    cameraSettings.aspectRatio = float(swapChain.extent.width)/float(swapChain.extent.height);

    camera = std::make_unique<OrbitingCameraController>(dynamic_cast<InputManager&>(*this), cameraSettings);
}

void ShaderBindingTableDemo::createInverseCam() {
    inverseCamProj = device.createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, sizeof(glm::mat4) * 2);
}

void ShaderBindingTableDemo::createCommandPool() {
    commandPool = device.createCommandPool(*device.queueFamilyIndex.graphics, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    commandBuffers = commandPool.allocateCommandBuffers(swapChainImageCount);
}

void ShaderBindingTableDemo::createPipelineCache() {
    pipelineCache = device.createPipelineCache();
}


void ShaderBindingTableDemo::createRenderPipeline() {
    //    @formatter:off
    auto builder = device.graphicsPipelineBuilder();
    render.pipeline =
        builder
            .shaderStage()
                .vertexShader("../../data/shaders/pass_through.vert.spv")
                .fragmentShader("../../data/shaders/pass_through.frag.spv")
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
                .renderPass(renderPass)
                .subpass(0)
                .name("render")
                .pipelineCache(pipelineCache)
            .build(render.layout);
    //    @formatter:on
}

void ShaderBindingTableDemo::createRayTracingPipeline() {
    auto rayGenShaderModule = VulkanShaderModule{ resource("raygen.rgen.spv"), device };
    auto hitGroup0ShaderModule = VulkanShaderModule{ resource("hitgroup0.rchit.spv"), device };
    auto hitGroup1ShaderModule = VulkanShaderModule{ resource("hitgroup1.rchit.spv"), device };
    auto missGroup0ShaderModule = VulkanShaderModule{ resource("miss0.rmiss.spv"), device };

    auto stages = initializers::rayTraceShaderStages({
        { rayGenShaderModule, VK_SHADER_STAGE_RAYGEN_BIT_KHR},
        { missGroup0ShaderModule, VK_SHADER_STAGE_MISS_BIT_KHR},
        { hitGroup0ShaderModule, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR},
        { hitGroup1ShaderModule, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR},
    });
    std::vector<VkRayTracingShaderGroupCreateInfoKHR> shaderGroups;
    shaderGroups.push_back(shaderTablesDesc.rayGenGroup());
    shaderGroups.push_back(shaderTablesDesc.addMissGroup("miss0"));
    shaderGroups.push_back(shaderTablesDesc.addHitGroup("hit0"));
    shaderGroups.push_back(shaderTablesDesc.addHitGroup("hit1"));

    shaderTablesDesc.hitGroups.addRecord("hit0", rgba(240, 129, 196));
    shaderTablesDesc.hitGroups.addRecord("hit1", rgba(29, 224, 205));

    dispose(raytrace.layout);

    raytrace.layout = device.createPipelineLayout({ raytrace.descriptorSetLayout, raytrace.instanceDescriptorSetLayout, raytrace.vertexDescriptorSetLayout });
    VkRayTracingPipelineCreateInfoKHR createInfo{ VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR };
    createInfo.stageCount = COUNT(stages);
    createInfo.pStages = stages.data();
    createInfo.groupCount = COUNT(shaderGroups);
    createInfo.pGroups = shaderGroups.data();
    createInfo.maxPipelineRayRecursionDepth = 0;
    createInfo.layout = raytrace.layout;

    raytrace.pipeline = device.createRayTracingPipeline(createInfo);
    bindingTables = shaderTablesDesc.compile(device, raytrace.pipeline);
}

void ShaderBindingTableDemo::onSwapChainDispose() {
    dispose(render.pipeline);
    dispose(raytrace.pipeline);
}

void ShaderBindingTableDemo::onSwapChainRecreation() {
    initCanvas();
    createRenderPipeline();
    createRayTracingPipeline();
}

VkCommandBuffer *ShaderBindingTableDemo::buildCommandBuffers(uint32_t imageIndex, uint32_t &numCommandBuffers) {
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

    canvas.draw(commandBuffer);

    vkCmdEndRenderPass(commandBuffer);

    rayTrace(commandBuffer);

    vkEndCommandBuffer(commandBuffer);

    return &commandBuffer;
}

void ShaderBindingTableDemo::rayTrace(VkCommandBuffer commandBuffer) {
    CanvasToRayTraceBarrier(commandBuffer);

    std::vector<VkDescriptorSet> sets{ raytrace.descriptorSet, raytrace.instanceDescriptorSet, raytrace.vertexDescriptorSet };
    assert(raytrace.pipeline);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, raytrace.pipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, raytrace.layout, 0, COUNT(sets), sets.data(), 0, VK_NULL_HANDLE);

    vkCmdTraceRaysKHR(commandBuffer, bindingTables.rayGen, bindingTables.miss, bindingTables.closestHit,
                      bindingTables.callable, swapChain.extent.width, swapChain.extent.height, 1);

    rayTraceToCanvasBarrier(commandBuffer);
}

void ShaderBindingTableDemo::rayTraceToCanvasBarrier(VkCommandBuffer commandBuffer) const {
    VkImageMemoryBarrier barrier = initializers::ImageMemoryBarrier();
    barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
    barrier.image = canvas.image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;


    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
                         VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                         0,

                         0,
                         VK_NULL_HANDLE,
                         0,
                         VK_NULL_HANDLE,
                         1,
                         &barrier);
}

void ShaderBindingTableDemo::CanvasToRayTraceBarrier(VkCommandBuffer commandBuffer) const {
    VkImageMemoryBarrier barrier = initializers::ImageMemoryBarrier();
    barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
    barrier.image = canvas.image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;

    VkMemoryBarrier mBarrier{};
    mBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    mBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    mBarrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;

    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                         VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
                         0,
                         0,
                         VK_NULL_HANDLE,
                         0,
                         VK_NULL_HANDLE,
                         1,
                         &barrier);
}

void ShaderBindingTableDemo::update(float time) {
    camera->update(time);
    auto cam = camera->cam();
    inverseCamProj.map<glm::mat4>([&](auto ptr){
        auto view = glm::inverse(cam.view);
        auto proj = glm::inverse(cam.proj);
        *ptr = view;
        *(ptr+1) = proj;
    });
}

void ShaderBindingTableDemo::checkAppInputs() {
    camera->processInput();
}

void ShaderBindingTableDemo::cleanup() {
    // TODO save pipeline cache
    VulkanBaseApp::cleanup();
}

void ShaderBindingTableDemo::onPause() {
    VulkanBaseApp::onPause();
}


int main(){
    try{

        Settings settings;
        settings.depthTest = true;

        auto app = ShaderBindingTableDemo{ settings };
        app.run();
    }catch(std::runtime_error& err){
        spdlog::error(err.what());
    }
}