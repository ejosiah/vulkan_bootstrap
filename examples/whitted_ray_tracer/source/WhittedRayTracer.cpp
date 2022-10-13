#include "WhittedRayTracer.hpp"
#include "GraphicsPipelineBuilder.hpp"
#include "DescriptorSetBuilder.hpp"
#include "sampling.hpp"

WhittedRayTracer::WhittedRayTracer(const Settings& settings) : VulkanRayTraceBaseApp("Whitted Ray tracer", settings) {
    fileManager.addSearchPath(".");
    fileManager.addSearchPath("../../examples/whitted_ray_tracer");
    fileManager.addSearchPath("../../examples/whitted_ray_tracer/spv");
    fileManager.addSearchPath("../../examples/whitted_ray_tracer/models");
    fileManager.addSearchPath("../../examples/whitted_ray_tracer/textures");
    fileManager.addSearchPath("../../data/shaders");
    fileManager.addSearchPath("../../data/models");
    fileManager.addSearchPath("../../data/textures");
    fileManager.addSearchPath("../../data");
}

void WhittedRayTracer::initApp() {
    initCamera();
    initCanvas();
    createInverseCam();
    createModels();
    createDescriptorPool();
    createSkyBox();
    createDescriptorSetLayouts();
    updateDescriptorSets();
    createCommandPool();
    createPipelineCache();
    createRayTracingPipeline();
}

void WhittedRayTracer::initCamera() {
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

void WhittedRayTracer::createModels() {
    auto bufferUsage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    spheres[Cook_Torrance].buffer = device.createBuffer(bufferUsage, VMA_MEMORY_USAGE_GPU_ONLY, 128);
    spheres[Glass].buffer = device.createBuffer(bufferUsage, VMA_MEMORY_USAGE_GPU_ONLY, 128);
    spheres[Mirror].buffer = device.createBuffer(bufferUsage, VMA_MEMORY_USAGE_GPU_ONLY, 128);

    createPlanes();
    createSpheres();

    if(ctMaterials.empty()){
        ctMaterials.push_back({});
    }
    ctMatBuffer = device.createDeviceLocalBuffer(ctMaterials.data(), BYTE_SIZE(ctMaterials), bufferUsage);

    if(glassMaterials.empty()){
        glassMaterials.push_back({});
    }
    glassMatBuffer = device.createDeviceLocalBuffer( glassMaterials.data(), BYTE_SIZE(glassMaterials), bufferUsage);

    asInstances = rtBuilder.buildTlas();
}

void WhittedRayTracer::createPlanes() {
    imp::Plane plane{{0, 1, 0}, 0};
    planes.planes.push_back(plane);
    planes.buffer = device.createDeviceLocalBuffer(planes.planes.data(),
                                                   planes.planes.size() * sizeof(imp::Plane),
                                                   VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);
    rtBuilder.add(planes.planes, static_cast<uint32_t>(imp::ImplicitType::PLANE), 0, Brdf::Cook_Torrance);

    float radius = 1000;
    imp::Sphere sphere{glm::vec3(0, -radius, 0), radius};
    spheres[Cook_Torrance].spheres.push_back(sphere);
    Material mat{glm::vec3(1), 0.1, 1.0};
    ctMaterials.push_back(mat);

}

void WhittedRayTracer::createSpheres() {
    createSpheres(Cook_Torrance, 50);
    createSpheres(Glass, 50);
    createSpheres(Mirror, 5);
}

void WhittedRayTracer::createSpheres(Brdf brdf, int numSpheres) {
    if(numSpheres <= 0) return;
    auto seed = 1 << 20;
    auto x = rng(0, 1, seed);
    auto radius = rng( 0.1, 0.5, seed);
    auto center = [&](float y){
        auto u = [&]{ return glm::vec2(x(), x()); };
        constexpr float placementArea = 5;
        auto p = sampling::uniformSampleDisk(u()) * placementArea;
        return glm::vec3(p.x, y, p.y);
    };
    auto checkCollision = [&](auto& sphere) {
        bool collided = false;
        for(auto & group : spheres){
            const auto& lSpheres = group.spheres;
            collided = collided || std::any_of(lSpheres.begin(), lSpheres.end(), [&](auto& sceneSphere){
                auto ed = sphere.radius + sceneSphere.radius;
                auto d = sphere.center - sceneSphere.center;
                return glm::dot(d, d) <= ed * ed;
            });
        }
        return collided;
    };

    auto& lSpheres = spheres[brdf].spheres;
    while(lSpheres.size() < numSpheres){
        imp::Sphere sphere;
        sphere.radius = radius();
        sphere.center = center(sphere.radius);

        if(checkCollision(sphere)){
            continue;
        }
        lSpheres.push_back(sphere);

        if(brdf == Cook_Torrance) {
            Material mat{randomColor().xyz(), x(), x()};
            ctMaterials.push_back(mat);
        }else if(brdf == Glass){
            auto ior = rng(1.f, 2.5);
            GlassMaterial mat{glm::vec3(1), ior()};
            glassMaterials.emplace_back(mat);
        }
    }
    spheres[brdf].buffer = device.createDeviceLocalBuffer(lSpheres.data(), BYTE_SIZE(lSpheres), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);
    rtBuilder.add(spheres[brdf].spheres, static_cast<uint32_t>(imp::ImplicitType::SPHERE), brdf);
}

void WhittedRayTracer::createSkyBox() {
    SkyBox::load(skybox, R"(C:\Users\Josiah Ebhomenye\OneDrive\media\textures\skybox\005)"
            , {"right.jpg", "left.jpg", "top.jpg", "bottom.jpg", "front.jpg", "back.jpg"}, &device);
}


void WhittedRayTracer::createDescriptorPool() {
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

void WhittedRayTracer::createDescriptorSetLayouts() {
    raytrace.descriptorSetLayout =
        device.descriptorSetLayoutBuilder()
            .name("ray_trace")
            .binding(0)
                .descriptorType(VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR)
                .descriptorCount(1)
                .shaderStages(VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR)
            .binding(1)
                .descriptorType(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
                .descriptorCount(1)
                .shaderStages(VK_SHADER_STAGE_RAYGEN_BIT_KHR)
            .binding(2)
                .descriptorType(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
                .descriptorCount(1)
                .shaderStages(VK_SHADER_STAGE_RAYGEN_BIT_KHR)
            .binding(3)
                .descriptorType(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
                .descriptorCount(1)
                .shaderStages(VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR)
        .createLayout();
}

void WhittedRayTracer::updateDescriptorSets() {
    auto sets = descriptorPool.allocate( { raytrace.descriptorSetLayout });
    raytrace.descriptorSet = sets[0];

    device.setName<VK_OBJECT_TYPE_DESCRIPTOR_SET>("ray_tracing_base", raytrace.descriptorSet);

    auto writes = initializers::writeDescriptorSets<4>();

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

    writes[3].dstSet = raytrace.descriptorSet;
    writes[3].dstBinding = 3;
    writes[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[3].descriptorCount = 1;
    VkDescriptorImageInfo skyboxInfo{ skybox.sampler, skybox.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    writes[3].pImageInfo = &skyboxInfo;


    device.updateDescriptorSets(writes);
}

void WhittedRayTracer::createCommandPool() {
    commandPool = device.createCommandPool(*device.queueFamilyIndex.graphics, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    commandBuffers = commandPool.allocateCommandBuffers(swapChainImageCount);
}

void WhittedRayTracer::createPipelineCache() {
    pipelineCache = device.createPipelineCache();
}

void WhittedRayTracer::initCanvas() {
    canvas = Canvas{ this, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_FORMAT_R8G8B8A8_UNORM};
    canvas.init();
    std::vector<unsigned char> checkerboard(width * height * 4);
    textures::checkerboard1(checkerboard.data(), {width, height});
    const auto stagingBuffer = device.createCpuVisibleBuffer(checkerboard.data(), BYTE_SIZE(checkerboard), VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
    device.graphicsCommandPool().oneTimeCommand([&](auto commandBuffer){
        textures::transfer(commandBuffer, stagingBuffer, canvas.image, {width, height}, VK_IMAGE_LAYOUT_GENERAL);
    });
}

void WhittedRayTracer::createInverseCam() {
    inverseCamProj = device.createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, sizeof(glm::mat4) * 2);
}

void WhittedRayTracer::createRayTracingPipeline() {
    auto rayGenShaderModule = VulkanShaderModule{ resource("raygen.rgen.spv"), device };
    auto missShaderModule = VulkanShaderModule{ resource("miss.rmiss.spv"), device };
    auto shadowMissModule = VulkanShaderModule{ resource("shadow.rmiss.spv"), device };
    auto diffuseHitShaderModule = VulkanShaderModule{ resource("cook_torrance.rchit.spv"), device };
    auto mirrorHitShaderModule = VulkanShaderModule{ resource("mirror.rchit.spv"), device };
    auto glassHitShaderModule = VulkanShaderModule{ resource("glass.rchit.spv"), device };
    auto implicitsIntersectShaderModule = VulkanShaderModule{resource("implicits.rint.spv"), device};
    auto occlusionHitShaderModule = VulkanShaderModule{resource("occlusion.rchit.spv"), device};
    auto occlusionAnyHitShaderModule = VulkanShaderModule{resource("occlusion.rahit.spv"), device};
    auto glassOcclusionHitShaderModule = VulkanShaderModule{resource("glass_occlusion.rchit.spv"), device};
    auto occlusionMissShaderModule = VulkanShaderModule{resource("occlusion.rmiss.spv"), device};
    auto checkerboardShaderModule = VulkanShaderModule{resource("checkerboard.rcall.spv"), device};
    auto fresnelShaderModule = VulkanShaderModule{resource("fresnel.rcall.spv"), device};

    device.setName<VK_OBJECT_TYPE_SHADER_MODULE>("ray_gen", rayGenShaderModule);
    device.setName<VK_OBJECT_TYPE_SHADER_MODULE>("miss", missShaderModule);
    device.setName<VK_OBJECT_TYPE_SHADER_MODULE>("shadow_miss", shadowMissModule);
    device.setName<VK_OBJECT_TYPE_SHADER_MODULE>("ct_hit", diffuseHitShaderModule);
    device.setName<VK_OBJECT_TYPE_SHADER_MODULE>("mirror_hit", mirrorHitShaderModule);
    device.setName<VK_OBJECT_TYPE_SHADER_MODULE>("glass_hit", glassHitShaderModule);
    device.setName<VK_OBJECT_TYPE_SHADER_MODULE>("implicit_intersect", implicitsIntersectShaderModule);
    device.setName<VK_OBJECT_TYPE_SHADER_MODULE>("occlusion_hit", occlusionHitShaderModule);
    device.setName<VK_OBJECT_TYPE_SHADER_MODULE>("glass_occlusion", glassOcclusionHitShaderModule);
    device.setName<VK_OBJECT_TYPE_SHADER_MODULE>("occlusion_anyhit", occlusionAnyHitShaderModule);
    device.setName<VK_OBJECT_TYPE_SHADER_MODULE>("occlusion_miss", occlusionMissShaderModule);
    device.setName<VK_OBJECT_TYPE_SHADER_MODULE>("checker_board_callable", checkerboardShaderModule);
    device.setName<VK_OBJECT_TYPE_SHADER_MODULE>("fresnel_callable", fresnelShaderModule);

    std::vector<ShaderInfo> shaders(eShaderGroupCount);
    shaders[eRayGen] = { rayGenShaderModule, VK_SHADER_STAGE_RAYGEN_BIT_KHR};
    shaders[eMiss] =   { missShaderModule, VK_SHADER_STAGE_MISS_BIT_KHR};
    shaders[eShadowMiss] =   { shadowMissModule, VK_SHADER_STAGE_MISS_BIT_KHR};
    shaders[eOcclusionMiss] =   { occlusionMissShaderModule, VK_SHADER_STAGE_MISS_BIT_KHR};
    shaders[eCookTorranceHit] =   { diffuseHitShaderModule, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR};
    shaders[eImplicitIntersect] =   { implicitsIntersectShaderModule, VK_SHADER_STAGE_INTERSECTION_BIT_KHR};
    shaders[eMirrorHit] =   { mirrorHitShaderModule, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR};
    shaders[eGlassHit] =   { glassHitShaderModule, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR};
    shaders[eOcclusionHit] = { occlusionHitShaderModule, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR};
    shaders[eGlassOcclusionHit] = { glassOcclusionHitShaderModule, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR};
    shaders[eOcclusionHitAny] = { occlusionAnyHitShaderModule, VK_SHADER_STAGE_ANY_HIT_BIT_KHR};
    shaders[eCheckerboardCallable] =   { checkerboardShaderModule, VK_SHADER_STAGE_CALLABLE_BIT_KHR};
    shaders[eFresnelCallable] =   { fresnelShaderModule, VK_SHADER_STAGE_CALLABLE_BIT_KHR};

    auto stages = initializers::rayTraceShaderStages(shaders);

    std::vector<VkRayTracingShaderGroupCreateInfoKHR> shaderGroups;
    shaderGroups.push_back(shaderTablesDesc.rayGenGroup(eRayGen));
    shaderGroups.push_back(shaderTablesDesc.addMissGroup(eMiss));
    shaderGroups.push_back(shaderTablesDesc.addMissGroup(eShadowMiss));
    shaderGroups.push_back(shaderTablesDesc.addMissGroup(eOcclusionMiss));

    shaderGroups.push_back(shaderTablesDesc.addHitGroup(eCookTorranceHit, eImplicitIntersect, VK_SHADER_UNUSED_KHR, VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR));
    shaderGroups.push_back(shaderTablesDesc.addHitGroup(eGlassHit, eImplicitIntersect, VK_SHADER_UNUSED_KHR, VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR));
    shaderGroups.push_back(shaderTablesDesc.addHitGroup(eMirrorHit, eImplicitIntersect, VK_SHADER_UNUSED_KHR, VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR));

    shaderGroups.push_back(shaderTablesDesc.addHitGroup(eOcclusionHit, eImplicitIntersect, VK_SHADER_UNUSED_KHR, VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR));
    shaderGroups.push_back(shaderTablesDesc.addHitGroup(eGlassOcclusionHit, eImplicitIntersect, VK_SHADER_UNUSED_KHR, VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR));
    shaderGroups.push_back(shaderTablesDesc.addHitGroup(eOcclusionHit, eImplicitIntersect, VK_SHADER_UNUSED_KHR, VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR));

//    shaderGroups.push_back(shaderTablesDesc.addHitGroup(VK_SHADER_UNUSED_KHR, eImplicitIntersect, eOcclusionHitAny, VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR));
//    shaderGroups.push_back(shaderTablesDesc.addHitGroup(eGlassOcclusionHit, eImplicitIntersect, eOcclusionHitAny, VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR));
//    shaderGroups.push_back(shaderTablesDesc.addHitGroup(VK_SHADER_UNUSED_KHR, eImplicitIntersect, eOcclusionHitAny, VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR));

    shaderGroups.push_back(shaderTablesDesc.addCallableGroup(eCheckerboardCallable));
    shaderGroups.push_back(shaderTablesDesc.addCallableGroup(eFresnelCallable));

    shaderTablesDesc.hitGroups[Cook_Torrance].addRecord(device.getAddress(spheres[Cook_Torrance].buffer));
    shaderTablesDesc.hitGroups[Glass].addRecord(device.getAddress(spheres[Glass].buffer));
    shaderTablesDesc.hitGroups[Mirror].addRecord(device.getAddress(spheres[Mirror].buffer));

    auto address = device.getAddress(planes.buffer);
    shaderTablesDesc.hitGroups[Cook_Torrance].addRecord(address);
    shaderTablesDesc.hitGroups[Glass].addRecord(address);
    shaderTablesDesc.hitGroups[Mirror].addRecord(address);

    address = device.getAddress(ctMatBuffer);
    shaderTablesDesc.hitGroups.get(Cook_Torrance).addRecord(address);

    address = device.getAddress(glassMatBuffer);
    shaderTablesDesc.hitGroups[Glass].addRecord(address);

    shaderTablesDesc.hitGroups[Cook_Torrance + eOcclusion * Brdf::Count].addRecord(device.getAddress(spheres[Cook_Torrance].buffer));
    shaderTablesDesc.hitGroups[Glass + eOcclusion * Brdf::Count].addRecord(device.getAddress(spheres[Glass].buffer));
    shaderTablesDesc.hitGroups[Mirror + eOcclusion * Brdf::Count].addRecord(device.getAddress(spheres[Mirror].buffer));

    address = device.getAddress(planes.buffer);
    shaderTablesDesc.hitGroups[Cook_Torrance + eOcclusion * Brdf::Count].addRecord(address);
    shaderTablesDesc.hitGroups[Glass + eOcclusion * Brdf::Count].addRecord(address);
    shaderTablesDesc.hitGroups[Mirror + eOcclusion * Brdf::Count].addRecord(address);


    dispose(raytrace.layout);

    raytrace.layout = device.createPipelineLayout({ raytrace.descriptorSetLayout });
    VkRayTracingPipelineCreateInfoKHR createInfo{ VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR };
    createInfo.stageCount = COUNT(stages);
    createInfo.pStages = stages.data();
    createInfo.groupCount = COUNT(shaderGroups);
    createInfo.pGroups = shaderGroups.data();
    createInfo.maxPipelineRayRecursionDepth = 5;
    createInfo.layout = raytrace.layout;

    raytrace.pipeline = device.createRayTracingPipeline(createInfo);
    bindingTables = shaderTablesDesc.compile(device, raytrace.pipeline);
}

void WhittedRayTracer::rayTrace(VkCommandBuffer commandBuffer) {
    CanvasToRayTraceBarrier(commandBuffer);

    std::vector<VkDescriptorSet> sets{ raytrace.descriptorSet };
    assert(raytrace.pipeline);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, raytrace.pipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, raytrace.layout, 0, COUNT(sets), sets.data(), 0, VK_NULL_HANDLE);

    vkCmdTraceRaysKHR(commandBuffer, bindingTables.rayGen, bindingTables.miss, bindingTables.closestHit,
                      bindingTables.callable, swapChain.extent.width, swapChain.extent.height, 1);

    rayTraceToCanvasBarrier(commandBuffer);
}

void WhittedRayTracer::rayTraceToCanvasBarrier(VkCommandBuffer commandBuffer) const {
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

void WhittedRayTracer::CanvasToRayTraceBarrier(VkCommandBuffer commandBuffer) const {
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



void WhittedRayTracer::onSwapChainDispose() {
    dispose(raytrace.pipeline);
}

void WhittedRayTracer::onSwapChainRecreation() {
    initCanvas();
    createRayTracingPipeline();
}

VkCommandBuffer *WhittedRayTracer::buildCommandBuffers(uint32_t imageIndex, uint32_t &numCommandBuffers) {
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

void WhittedRayTracer::update(float time) {
    glfwSetWindowTitle(window, fmt::format("{} - FPS {}", title, framePerSecond).c_str());
    camera->update(time);
    auto cam = camera->cam();
    inverseCamProj.map<glm::mat4>([&](auto ptr){
        auto view = glm::inverse(cam.view);
        auto proj = glm::inverse(cam.proj);
        *ptr = view;
        *(ptr+1) = proj;
    });
}

void WhittedRayTracer::checkAppInputs() {
    camera->processInput();
}

void WhittedRayTracer::cleanup() {
}

void WhittedRayTracer::onPause() {
    VulkanBaseApp::onPause();
}


int main(){
    try{

        Settings settings;
        settings.depthTest = true;

        auto app = WhittedRayTracer{ settings };
        app.run();
    }catch(std::runtime_error& err){
        spdlog::error(err.what());
    }
}