#include "RayTracingImplicits.hpp"

RayTracingImplicits::RayTracingImplicits(const Settings& settings)
        : VulkanRayTraceBaseApp("Implicit Objects", settings)
{

}

void RayTracingImplicits::initApp() {
    initCamera();
    createModel();
    createCommandPool();
    createDescriptorPool();
    createDescriptorSetLayout();
    createDescriptorSet();
    createPipeline();
    createBindingTables();
}

void RayTracingImplicits::createModel() {
    createPlanes();
    createSpheres();
    asInstances = rtBuilder.buildTlas();
}

void RayTracingImplicits::createPlanes() {
    imp::Plane plane{{0, 1, 0}, 0};
    planes.planes.push_back(plane);
    planes.planeBuffer = device.createDeviceLocalBuffer(planes.planes.data(),
                                                        planes.planes.size() * sizeof(imp::Plane),
                                                        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);

    rtBuilder.add(planes.planes);
}

void RayTracingImplicits::createSpheres() {
    int maxSpheres = 100;
    auto randomCenter = []{
        static std::random_device rnd{};
        static std::default_random_engine engine{rnd()};
        static std::uniform_real_distribution<float> dist{-3.0f, 3.0f};
//        static std::normal_distribution<float> dist{-0.3f, 3.0f};
        return glm::vec3(dist(engine), dist(engine), dist(engine));
    };
    auto randomRadius = []{
        static std::random_device rnd{};
        static std::default_random_engine engine{rnd()};
        static std::uniform_real_distribution<float> dist{0.02f, 0.5f};

        return dist(engine);
    };

    spheres.spheres.reserve(maxSpheres);
    int numSpheres = 0;

    while(numSpheres < maxSpheres){
        imp::Sphere sphere{};
        sphere.center = randomCenter();
        sphere.radius = randomRadius();
        auto collision = std::any_of(begin(spheres.spheres), end(spheres.spheres), [&](auto& s){
            auto d = s.center - sphere.center;
            auto dd = glm::dot(d, d);
            return dd - pow(s.radius + sphere.radius, 2) <= 0;
        });
        if(collision) continue;
        spheres.spheres.push_back(sphere);
        numSpheres = spheres.spheres.size();
    }

    spheres.buffer = device.createDeviceLocalBuffer(spheres.spheres.data(),
                                                    spheres.spheres.size() * sizeof(imp::Sphere),
                                                    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);

    rtBuilder.add(spheres.spheres);
}

void RayTracingImplicits::initCamera() {
    OrbitingCameraSettings cameraSettings;
//    FirstPersonSpectatorCameraSettings cameraSettings;
    cameraSettings.orbitMinZoom = 0.1;
    cameraSettings.orbitMaxZoom = 512.0f;
    cameraSettings.offsetDistance = 3.0f;
    cameraSettings.fieldOfView = 60.0f;
    cameraSettings.aspectRatio = float(swapChain.extent.width)/float(swapChain.extent.height);

    camera = std::make_unique<OrbitingCameraController>
            (dynamic_cast<InputManager&>(*this), cameraSettings);
}

void RayTracingImplicits::onSwapChainDispose() {
    dispose(pipeline);
    descriptorPool.free(descriptorSet);
}

void RayTracingImplicits::onSwapChainRecreation() {
    canvas.recreate();
    createDescriptorSet();
    createPipeline();
    createBindingTables();
    camera->onResize(swapChain.width(), swapChain.height());
}

VkCommandBuffer *RayTracingImplicits::buildCommandBuffers(uint32_t imageIndex, uint32_t &numCommandBuffers) {
    numCommandBuffers = 1;
    auto& commandBuffer = commandBuffers[imageIndex];

    auto beginInfo = initializers::commandBufferBeginInfo();
    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    std::array<VkClearValue, 1> clearValues{{0.0f, 0.0f, 1.0f, 1.0f}};

    auto renderPassBeginInfo = initializers::renderPassBeginInfo();
    renderPassBeginInfo.renderPass = renderPass;
    renderPassBeginInfo.framebuffer = framebuffers[imageIndex];
    renderPassBeginInfo.renderArea.offset = {0u, 0u};
    renderPassBeginInfo.renderArea.extent = swapChain.extent;
    renderPassBeginInfo.clearValueCount = COUNT(clearValues);
    renderPassBeginInfo.pClearValues = clearValues.data();

    rayTracingToGraphicsBarrier(commandBuffer);
    vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    canvas.draw(commandBuffer);

    vkCmdEndRenderPass(commandBuffer);

    GraphicsToRayTracingBarrier(commandBuffer);
    rayTrace(commandBuffer);

    vkEndCommandBuffer(commandBuffer);

    return &commandBuffer;
}

void RayTracingImplicits::rayTrace(VkCommandBuffer commandBuffer) {
    static std::array<VkDescriptorSet, 2> sets;
    sets[0] = descriptorSet;
    sets[1] = objectsDescriptorSet;

    static std::array<char, sizeof(glm::mat4) * 2> cameraInverse{};
    auto viewInverse = glm::inverse(camera->cam().view);
    auto projInverse = glm::inverse(camera->cam().proj);
    std::memcpy(cameraInverse.data(), &viewInverse, sizeof(glm::mat4));
    std::memcpy(cameraInverse.data() + sizeof(glm::mat4), &projInverse, sizeof(glm::mat4));

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipeline);
    vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_RAYGEN_BIT_KHR, 0, sizeof(glm::mat4) * 2, cameraInverse.data());
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, layout, 0, COUNT(sets), sets.data(), 0, VK_NULL_HANDLE);

    VkStridedDeviceAddressRegionKHR addressRegion{};

    vkCmdTraceRaysKHR(commandBuffer, bindingTables.rayGen, bindingTables.miss, bindingTables.closestHit, &addressRegion, swapChain.width(), swapChain.height(), 1);
}

void RayTracingImplicits::GraphicsToRayTracingBarrier(VkCommandBuffer commandBuffer) {
    auto barrier = memoryBarrier(VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT);

    vkCmdPipelineBarrier(
            commandBuffer,
            VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0,
            0,
            VK_NULL_HANDLE,
            0,
            VK_NULL_HANDLE,
            1,
            &barrier
    );
}

void RayTracingImplicits::rayTracingToGraphicsBarrier(VkCommandBuffer commandBuffer) {
    auto barrier = memoryBarrier(VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_SHADER_WRITE_BIT);

    vkCmdPipelineBarrier(
            commandBuffer,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
            0,
            0,
            VK_NULL_HANDLE,
            0,
            VK_NULL_HANDLE,
            1,
            &barrier
    );
}

VkImageMemoryBarrier RayTracingImplicits::memoryBarrier(VkAccessFlags srcAccessMark, VkAccessFlags dstAccessMark) {
    auto barrier = initializers::ImageMemoryBarrier();
    barrier.srcAccessMask = srcAccessMark;
    barrier.srcAccessMask = dstAccessMark;
    barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
    barrier.image = canvas.image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.baseArrayLayer = 0;

    return barrier;
}

void RayTracingImplicits::update(float time) {
    camera->update(time);
}

void RayTracingImplicits::checkAppInputs() {
    camera->processInput();
}

void RayTracingImplicits::cleanup() {
    VulkanBaseApp::cleanup();
}

void RayTracingImplicits::createCommandPool() {
    commandPool = device.createCommandPool(*device.queueFamilyIndex.graphics, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    commandBuffers = commandPool.allocateCommandBuffers(swapChainImageCount);
}

void RayTracingImplicits::createDescriptorPool() {
    std::array<VkDescriptorPoolSize, 3> poolSizes{};
    poolSizes[0].descriptorCount = 1;
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;

    poolSizes[1].descriptorCount = 1;
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

    poolSizes[2].descriptorCount = 10;
    poolSizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

    descriptorPool = device.createDescriptorPool(2, poolSizes, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT);
}

void RayTracingImplicits::createDescriptorSetLayout() {
    std::vector<VkDescriptorSetLayoutBinding> bindings(2);
    bindings[0].binding = 0;
    bindings[0].descriptorCount = 1;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
    bindings[0].stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

    bindings[1].binding = 1;
    bindings[1].descriptorCount = 1;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    bindings[1].stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR;

    descriptorSetLayout = device.createDescriptorSetLayout(bindings);

    bindings.resize(2);
    bindings[0].binding = 0;
    bindings[0].descriptorCount = 1;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[0].stageFlags = VK_SHADER_STAGE_INTERSECTION_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

    bindings[1].binding = 1;
    bindings[1].descriptorCount = 1;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[1].stageFlags = VK_SHADER_STAGE_INTERSECTION_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

    objectsDescriptorSetLayout = device.createDescriptorSetLayout(bindings);
}

void RayTracingImplicits::createDescriptorSet() {
    auto sets = descriptorPool.allocate({ descriptorSetLayout, objectsDescriptorSetLayout});
    descriptorSet = sets[0];
    objectsDescriptorSet = sets[1];


    VkWriteDescriptorSetAccelerationStructureKHR asWrites{};
    asWrites.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
    asWrites.accelerationStructureCount = 1;
    asWrites.pAccelerationStructures = rtBuilder.accelerationStructure();

    auto writes = initializers::writeDescriptorSets<4>();
    writes[0].pNext = &asWrites;
    writes[0].dstSet = descriptorSet;
    writes[0].dstBinding = 0;
    writes[0].descriptorCount = 1;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;

    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageView = canvas.imageView;
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    imageInfo.sampler = canvas.sampler;

    writes[1].dstSet = descriptorSet;
    writes[1].dstBinding = 1;
    writes[1].descriptorCount = 1;
    writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    writes[1].pImageInfo = &imageInfo;

    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = spheres.buffer;
    bufferInfo.offset = 0;
    bufferInfo.range = VK_WHOLE_SIZE;

    writes[2].dstSet = objectsDescriptorSet;
    writes[2].dstBinding = 0;
    writes[2].descriptorCount = 1;
    writes[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[2].pBufferInfo = &bufferInfo;

    VkDescriptorBufferInfo bufferInfo1{};
    bufferInfo1.buffer = planes.planeBuffer;
    bufferInfo1.offset = 0;
    bufferInfo1.range = VK_WHOLE_SIZE;

    writes[3].dstSet = objectsDescriptorSet;
    writes[3].dstBinding = 1;
    writes[3].descriptorCount = 1;
    writes[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[3].pBufferInfo = &bufferInfo1;

    device.updateDescriptorSets(writes);

}


void RayTracingImplicits::createPipeline() {
    shaderGroups.clear();


    std::vector<VkPipelineShaderStageCreateInfo> stages;

    // Ray generation group
    auto rayGenShader = VulkanShaderModule{"../../data/shaders/raytracing_implicits/implicits.rgen.spv", device};
    shaderGroups.push_back(shaderTablesDesc.rayGenGroup());
    stages.push_back(initializers::shaderStage({ rayGenShader, VK_SHADER_STAGE_RAYGEN_BIT_KHR}));

    // miss group 0;
    auto missShader = VulkanShaderModule{"../../data/shaders/raytracing_implicits/implicits.rmiss.spv", device};
    shaderGroups.push_back(shaderTablesDesc.addMissGroup());
    stages.push_back(initializers::shaderStage({ missShader, VK_SHADER_STAGE_MISS_BIT_KHR}));

    // miss group 1
    auto shadowShader = VulkanShaderModule{"../../data/shaders/raytracing_implicits/shadow.rmiss.spv", device};
    shaderGroups.push_back(shaderTablesDesc.addMissGroup());
    stages.push_back(initializers::shaderStage({ shadowShader, VK_SHADER_STAGE_MISS_BIT_KHR}));

    // hit group 0;
    auto hitShader = VulkanShaderModule{"../../data/shaders/raytracing_implicits/implicits.rchit.spv", device};
    auto intersectShader = VulkanShaderModule{"../../data/shaders/raytracing_implicits/implicits.rint.spv", device};
    stages.push_back(initializers::shaderStage({ hitShader, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR}));
    stages.push_back(initializers::shaderStage({ intersectShader, VK_SHADER_STAGE_INTERSECTION_BIT_KHR}));
    shaderGroups.push_back(shaderTablesDesc.addHitGroup(VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR, true,
                                                        false, true));

    dispose(layout);
    layout = device.createPipelineLayout( { descriptorSetLayout, objectsDescriptorSetLayout },
                                          { {VK_SHADER_STAGE_RAYGEN_BIT_KHR, 0, sizeof(glm::mat4) * 2}});

    auto createInfo = initializers::rayTracingPipelineCreateInfo();
    createInfo.stageCount = COUNT(stages);
    createInfo.pStages = stages.data();
    createInfo.groupCount = COUNT(shaderGroups);
    createInfo.pGroups = shaderGroups.data();
    createInfo.maxPipelineRayRecursionDepth = 1;
    createInfo.layout = layout;

    pipeline = device.createRayTracingPipeline(createInfo);
}

void RayTracingImplicits::createBindingTables() {
    bindingTables = shaderTablesDesc.compile(device, pipeline);
}

int main() {
    auto raytracing = RayTracingImplicits{ {} };
    raytracing.run();
    return 0;
}
