#include <SPHFluidSimulation.hpp>
#include "SPHFluidSimulation.hpp"
#include "sampling.hpp"
#include "vulkan_util.h"

SPHFluidSimulation::SPHFluidSimulation(const Settings& settings) : VulkanBaseApp("SPH Fluid Simulation", settings) {
    deviceCreateNextChain = &scalarFeatures;
}

void SPHFluidSimulation::initApp() {
    bufferOffsetAlignment = device.getLimits().minStorageBufferOffsetAlignment;
    initCamera();
    createDescriptorPool();
    createDescriptorSetLayouts();
    createCommandPool();
    createParticles();
    createSdf();
    createEmitter();
    createCollisionResolver();
    createDescriptorSets();
    createRenderDescriptorSet();
    createPipelineCache();
    createRenderPipeline();
    createComputePipeline();
    createPointHashGrid();
    createForces();
    createTimeIntegrator();
    createComputePressure();
    computeMass();
    createInterpolator();
    emit();
}

void SPHFluidSimulation::createDescriptorPool() {
    constexpr uint32_t maxSets = 100;
    std::array<VkDescriptorPoolSize, 17> poolSizes{
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
                    { VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 100 * maxSets },
                    { VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV, 100 * maxSets },

            }
    };
    descriptorPool = device.createDescriptorPool(maxSets, poolSizes, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT);

}


void SPHFluidSimulation::createDescriptorSetLayouts() {
    std::vector<VkDescriptorSetLayoutBinding> bindings(2);
    bindings[0].binding = 0;
    bindings[0].descriptorCount = 1;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    bindings[1].binding = 1;
    bindings[1].descriptorCount = 1;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    particles.descriptorSetLayout = device.createDescriptorSetLayout(bindings);

    bindings.resize(1);
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    render.setLayout = device.createDescriptorSetLayout(bindings);
}

void SPHFluidSimulation::createDescriptorSets() {
    descriptorPool.allocate({ particles.descriptorSetLayout, particles.descriptorSetLayout }, particles.descriptorSets);

    std::array<VkWriteDescriptorSet, 4> writes = initializers::writeDescriptorSets<4>();

    VkDescriptorBufferInfo info0{ particles.buffers[0], 0, particles.pointSize};
    writes[0].dstSet = particles.descriptorSets[0];
    writes[0].dstBinding = 0;
    writes[0].descriptorCount = 1;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[0].pBufferInfo = &info0;

    VkDescriptorBufferInfo info1{ particles.buffers[0], particles.pointSize, particles.dataSize};
    writes[1].dstSet = particles.descriptorSets[0];
    writes[1].dstBinding = 1;
    writes[1].descriptorCount = 1;
    writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[1].pBufferInfo = &info1;

    VkDescriptorBufferInfo info2{ particles.buffers[1], 0, particles.pointSize};
    writes[2].dstSet = particles.descriptorSets[1];
    writes[2].dstBinding = 0;
    writes[2].descriptorCount = 1;
    writes[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[2].pBufferInfo = &info2;

    VkDescriptorBufferInfo info3{ particles.buffers[1], particles.pointSize, particles.dataSize};
    writes[3].dstSet = particles.descriptorSets[1];
    writes[3].dstBinding = 1;
    writes[3].descriptorCount = 1;
    writes[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[3].pBufferInfo = &info3;


    device.updateDescriptorSets(writes);
}

void SPHFluidSimulation::createRenderDescriptorSet() {
    render.descriptorSet = descriptorPool.allocate({ render.setLayout}).front();
    
    auto writes = initializers::writeDescriptorSets<1>(render.descriptorSet);

    VkDescriptorBufferInfo info = {render.mvpBuffer, 0, VK_WHOLE_SIZE};
    writes[0].dstBinding = 0;
    writes[0].descriptorCount = 1;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writes[0].pBufferInfo = &info;
    
    device.updateDescriptorSets(writes);
}

void SPHFluidSimulation::createCommandPool() {
    commandPool = device.createCommandPool(*device.queueFamilyIndex.graphics, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    commandBuffers = commandPool.allocateCommandBuffers(swapChainImageCount);
}

void SPHFluidSimulation::createPipelineCache() {
    pipelineCache = device.createPipelineCache();
}


void SPHFluidSimulation::createRenderPipeline() {
    auto vertModule = VulkanShaderModule{"../../data/shaders/sph/grid.vert.spv", device};
    auto fragModule = VulkanShaderModule{"../../data/shaders/sph/grid.frag.spv", device};

    auto shaderStages = initializers::vertexShaderStages({
             { vertModule, VK_SHADER_STAGE_VERTEX_BIT },
             {fragModule, VK_SHADER_STAGE_FRAGMENT_BIT}
     });

    std::vector<VkVertexInputBindingDescription> bindings{
            {0, sizeof(glm::vec3), VK_VERTEX_INPUT_RATE_VERTEX}
    };
    std::vector<VkVertexInputAttributeDescription> attribs {
            {0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0}
    };

    auto vertexInputState = initializers::vertexInputState(bindings, attribs);
    auto inputAssemblyState = initializers::inputAssemblyState(VK_PRIMITIVE_TOPOLOGY_LINE_LIST);

    auto viewport = initializers::viewport(swapChain.extent);
    auto scissor = initializers::scissor(swapChain.extent);

    auto viewportState = initializers::viewportState(viewport, scissor);

    auto rasterizationState = initializers::rasterizationState();
    rasterizationState.cullMode = VK_CULL_MODE_NONE;
    rasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizationState.lineWidth = 2.0;

    auto multisampleState = initializers::multisampleState();

    auto depthStencilState = initializers::depthStencilState();
    depthStencilState.depthTestEnable = VK_TRUE;
    depthStencilState.depthWriteEnable = VK_TRUE;
    depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencilState.minDepthBounds = 0.0;
    depthStencilState.maxDepthBounds = 1.0;

    auto colorBlendAttachment = initializers::colorBlendStateAttachmentStates();
    auto colorBlendState = initializers::colorBlendState(colorBlendAttachment);

    render.layout = device.createPipelineLayout({ render.setLayout}, {});

    auto createInfo = initializers::graphicsPipelineCreateInfo();
    createInfo.flags = VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT;
    createInfo.stageCount = COUNT(shaderStages);
    createInfo.pStages = shaderStages.data();
    createInfo.pVertexInputState = &vertexInputState;
    createInfo.pInputAssemblyState = &inputAssemblyState;
    createInfo.pViewportState = &viewportState;
    createInfo.pRasterizationState = &rasterizationState;
    createInfo.pMultisampleState = &multisampleState;
    createInfo.pDepthStencilState = &depthStencilState;
    createInfo.pColorBlendState = &colorBlendState;
    createInfo.layout = render.layout;
    createInfo.renderPass = renderPass;
    createInfo.subpass = 0;

    render.pipeline = device.createGraphicsPipeline(createInfo, pipelineCache);

    // particle pipeline
    auto particleVertModule = VulkanShaderModule{"../../data/shaders/sph/particle.vert.spv", device};
    auto particleFragModule = VulkanShaderModule{"../../data/shaders/sph/particle.frag.spv", device};
    shaderStages = initializers::vertexShaderStages({
             { particleVertModule, VK_SHADER_STAGE_VERTEX_BIT },
             {particleFragModule, VK_SHADER_STAGE_FRAGMENT_BIT}
     });

    VkSpecializationMapEntry entry{0, 0, sizeof(float)};
    float pointSize = 1.0f;
    VkSpecializationInfo specializationInfo{1, &entry, entry.size, &pointSize};
    shaderStages[0].pSpecializationInfo = &specializationInfo;

    bindings = std::vector<VkVertexInputBindingDescription>{
            {0, sizeof(glm::vec4), VK_VERTEX_INPUT_RATE_VERTEX},
            {1, sizeof(ParticleData), VK_VERTEX_INPUT_RATE_VERTEX}
    };
    attribs = std::vector<VkVertexInputAttributeDescription>{
            {0, 0, VK_FORMAT_R32G32B32A32_SFLOAT, 0},
            {1, 1, VK_FORMAT_R32G32B32A32_SFLOAT, 0},
    };

    vertexInputState = initializers::vertexInputState(bindings, attribs);
    inputAssemblyState = initializers::inputAssemblyState(VK_PRIMITIVE_TOPOLOGY_POINT_LIST);

    particles.layout = device.createPipelineLayout({ render.setLayout }, { });

    createInfo.flags = VK_PIPELINE_CREATE_DERIVATIVE_BIT;
    createInfo.stageCount = COUNT(shaderStages);
    createInfo.pStages = shaderStages.data();
    createInfo.pVertexInputState = &vertexInputState;
    createInfo.pInputAssemblyState = &inputAssemblyState;
    createInfo.layout = particles.layout;
    createInfo.basePipelineHandle = render.pipeline;
    createInfo.basePipelineIndex = -1;

    particles.pipeline = device.createGraphicsPipeline(createInfo, pipelineCache);
}

void SPHFluidSimulation::createComputePipeline() {
    auto module = VulkanShaderModule{"../../data/shaders/sph/sph_sim.comp.spv", device};
    auto stage = initializers::shaderStage({ module, VK_SHADER_STAGE_COMPUTE_BIT});

    compute.layout = device.createPipelineLayout({particles.descriptorSetLayout, particles.descriptorSetLayout}, { { VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(particles.constants) } });


    auto computeCreateInfo = initializers::computePipelineCreateInfo();
    computeCreateInfo.stage = stage;
    computeCreateInfo.layout = compute.layout;

    compute.pipeline = device.createComputePipeline(computeCreateInfo, pipelineCache);
}


void SPHFluidSimulation::onSwapChainDispose() {
    dispose(render.pipeline);
    dispose(particles.pipeline);
    dispose(compute.pipeline);
}

void SPHFluidSimulation::onSwapChainRecreation() {
    createRenderPipeline();
    createComputePipeline();
    camera->onResize(swapChain.width(), swapChain.height());
}

VkCommandBuffer *SPHFluidSimulation::buildCommandBuffers(uint32_t imageIndex, uint32_t &numCommandBuffers) {
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
    // render grid
     static std::array<VkDeviceSize, 2> offsets = {0, particles.pointSize};
//    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, render.pipeline);
//    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, render.layout, 0, 1, &render.descriptorSet, 0, nullptr);
//    vkCmdBindVertexBuffers(commandBuffer, 0, 1, grid.vertexBuffer, &offset);
//    vkCmdDraw(commandBuffer, grid.vertexBuffer.size/sizeof(glm::vec3), 1, 0, 0);

    // render particles
    static std::array<VkBuffer, 2> vertexBuffers;
    vertexBuffers[0] = particles.buffers[0];
    vertexBuffers[1] = particles.buffers[0];
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, particles.pipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, render.layout, 0, 1, &render.descriptorSet, 0, nullptr);
    vkCmdBindVertexBuffers(commandBuffer, 0, 2, vertexBuffers.data(), offsets.data());
    vkCmdDraw(commandBuffer, particles.constants.numParticles, 1, 0, 0);

    vkCmdEndRenderPass(commandBuffer);

    vkEndCommandBuffer(commandBuffer);

    return &commandBuffer;
}

void SPHFluidSimulation::update(float time) {
    particles.constants.time = time;
    camera->update(time);
    render.mvpBuffer.map<Camera>([&](auto ptr){
        *ptr = camera->cam();
    });
    pointHashGrid.setNumParticles(numParticles);
    forceDescriptor.setNumParticles(numParticles);
    computePressure.setNumParticles(numParticles);
    interpolator.setNumParticles(numParticles);
    resolveCollision.setNumParticles(numParticles);
    applyExternalForces.update(numParticles, time);
    timeIntegration.update(numParticles, time);

    std::vector<VkDescriptorSet> neighbourDescriptorSets{
            pointHashGrid.neighbourList.descriptorSet,
            pointHashGrid.neighbourList.neighbourSizeDescriptorSet,
            pointHashGrid.neighbourList.neighbourSizeOffsetDescriptorSet
    };
    interpolator.setNeighbourDescriptors(neighbourDescriptorSets.data());

    runPhysics();
}

void SPHFluidSimulation::checkAppInputs() {
    camera->processInput();
}

void SPHFluidSimulation::cleanup() {
    // TODO save pipeline cache
    VulkanBaseApp::cleanup();
}

void SPHFluidSimulation::onPause() {
    VulkanBaseApp::onPause();
}

void SPHFluidSimulation::createParticles() {
    particles.pointSize = alignedSize(sizeof(glm::vec4) * MAX_PARTICLES, bufferOffsetAlignment);
    particles.dataSize = alignedSize(sizeof(ParticleData) * MAX_PARTICLES, bufferOffsetAlignment);
    particles.constants.numParticles = 0;
    auto size = particles.pointSize + particles.dataSize;
    particles.buffers[0] = device.createBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, size);
    particles.buffers[1] = device.createBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, size);
}

void SPHFluidSimulation::runPhysics() {
    glfwSetWindowTitle(window, fmt::format("SPH Fluid Simulation - FPS {}", framePerSecond).c_str());
    int inIndex = currentFrame % 2;
    int outIndex = 1 - inIndex;
    static std::array<VkDescriptorSet, 2> sets{};
    sets[PARTICLE_IN] = particles.descriptorSets[inIndex];
    sets[PARTICLE_OUT] = particles.descriptorSets[outIndex];

//    device.graphicsCommandPool().oneTimeCommand([&](VkCommandBuffer commandBuffer){
//        buildHashPointGrid(commandBuffer, sets[PARTICLE_IN]);
//        addComputeBufferMemoryBarriers(commandBuffer, pointHashGrid.bucketBuffers());
//        buildNeighbourList(commandBuffer, sets[PARTICLE_IN]);
//        addComputeBufferMemoryBarriers(commandBuffer, pointHashGrid.neighbourBuffers());
////        interpolator.updateDensity(commandBuffer, sets[PARTICLE_IN]);
////        addComputeBufferMemoryBarriers(commandBuffer, { &particles.buffers[0], &particles.buffers[1] });
////        interpolator.updateColor(commandBuffer, sets[PARTICLE_IN]);
//        applyExternalForces(commandBuffer, sets[PARTICLE_IN]);
////        addComputeBufferMemoryBarriers(commandBuffer, { &forceDescriptor.forceBuffer });
////        computePressure(commandBuffer, sets[PARTICLE_IN]);
////        addComputeBufferMemoryBarriers(commandBuffer, { &particles.buffers[0], &particles.buffers[1] });
////        interpolator.updatePressure(commandBuffer, sets[PARTICLE_IN]);
////        addComputeBufferMemoryBarriers(commandBuffer, { &forceDescriptor.forceBuffer });
//        timeIntegration(commandBuffer, sets, forceDescriptor.set);
//        addComputeBufferMemoryBarriers(commandBuffer, { &particles.buffers[0], &particles.buffers[1] });
//        resolveCollision(commandBuffer, sets[PARTICLE_OUT]);
//    });

    static bool debug = false;
    if(debug){
        debug = false;
        forceDescriptor.forceBuffer.map<glm::vec3>([&](auto ptr){
            particles.buffers[PARTICLE_IN].map<char>([&](auto pPtr){
                pointHashGrid.neighbourList.neighbourSizeBuffer.map<int>([&](auto nSizePtr){
                    pointHashGrid.neighbourList.neighbourOffsetsBuffer.map<int>([&](auto offsetPtr){
                        for(int i = 0; i < numParticles; i++){
                            glm::vec3 f = ptr[i];
                            ParticleData data = reinterpret_cast<ParticleData*>(pPtr + particles.pointSize)[i];
                            auto size = nSizePtr[i];
                            int noNeighbours = 0;

                            if(data.density > 0){
                                spdlog::error("i: {}, force: {}, density: {}, pressure : {}, size: {}", i, f, data.density, data.pressure, size);
                            }
                        }
                    });
                });
            })  ;

        });
    }

}

void SPHFluidSimulation::buildHashPointGrid(VkCommandBuffer commandBuffer, VkDescriptorSet descriptorSet) {
    pointHashGrid.buildHashGrid(commandBuffer, descriptorSet);
}

void SPHFluidSimulation::buildNeighbourList(VkCommandBuffer commandBuffer, VkDescriptorSet descriptorSet) {
    pointHashGrid.generateNeighbourList(commandBuffer, descriptorSet);
}

void SPHFluidSimulation::createEmitter() {
    auto targetSpacing = particleProperties.targetSpacing;
    emitter = VolumeParticleEmitter{&device, &descriptorPool, &particles.descriptorSetLayout, sdf.sdf, targetSpacing};
    emitter.init();
    emitter.constants.jitter = 1.0;
}

void SPHFluidSimulation::createCollisionResolver() {
    BoxSurface box{domain, true };
    resolveCollision = Collider{ &device, &descriptorPool, &particles.descriptorSetLayout, box, particleProperties.targetSpacing};
    resolveCollision.init();
}

void SPHFluidSimulation::createSdf() {
    auto targetSpacing = particleProperties.targetSpacing;
    BoundingBox region = expand(domain, -targetSpacing);
    sdf = SdfCompute{ &device, &descriptorPool, region,
                      "../../data/sph/water_drop.comp.spv", glm::ivec3{128}};
}

void SPHFluidSimulation::createPointHashGrid() {
    auto radius = particleProperties.kernelRadius;
    assert(radius > 0);
    pointHashGrid = PointHashGrid{ &device, &descriptorPool, &particles.descriptorSetLayout, glm::vec3{64}, radius * 2.0f};
    pointHashGrid.init();
}

void SPHFluidSimulation::createForces() {
    forceDescriptor = ForceDescriptor{ &device, &descriptorPool};
    forceDescriptor.init();
    applyExternalForces = ExternalForces{ &device, &forceDescriptor, &particles.descriptorSetLayout, particles.constants.drag};
    applyExternalForces.init();
    applyExternalForces.constants.gravity = {0, -9.8, 0};
}

void SPHFluidSimulation::createComputePressure() {
    computePressure = ComputePressure{ &device, &particles.descriptorSetLayout,
                                       eosExponent, particleProperties.targetDensity,
                                       speedOfSound, negativePressureScale};
    computePressure.init();
}

void SPHFluidSimulation::createInterpolator() {
    interpolator = Interpolator{ &device
                                 , &particles.descriptorSetLayout
                                 , &forceDescriptor
                                 , &pointHashGrid.neighbourList.setLayout
                                 , &pointHashGrid.neighbourList.neighbourSizeSetLayout
                                 , &pointHashGrid.neighbourList.neighbourSizeSetLayout
                                 , particleProperties.kernelRadius
                                 , 1.0f/particleProperties.mass
    };
    interpolator.init();
}

void SPHFluidSimulation::createTimeIntegrator() {
    timeIntegration = TimeIntegration{ &device, &particles.descriptorSetLayout, &forceDescriptor.layoutSet};
    timeIntegration.init();
}

void SPHFluidSimulation::initCamera() {
    OrbitingCameraSettings settings{};
    settings.offsetDistance = 2.0f;
    settings.rotationSpeed = 0.1f;
    settings.orbitMinZoom = -5.0f;
    settings.fieldOfView = 45.0f;
//    settings.modelHeight = diagonal(domain).y;
    settings.aspectRatio = static_cast<float>(swapChain.extent.width)/static_cast<float>(swapChain.extent.height);
    camera = std::make_unique<OrbitingCameraController>(dynamic_cast<InputManager&>(*this), settings);
    render.mvpBuffer = device.createCpuVisibleBuffer(&camera->cam(), sizeof(Camera), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
}

void SPHFluidSimulation::addParticleBufferBarrier(VkCommandBuffer commandBuffer, int index) {
    addBufferMemoryBarriers(commandBuffer, { &particles.buffers[index] }
    , VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
}

void SPHFluidSimulation::emit(){
    device.computeCommandPool().oneTimeCommand([&](auto cmdBuffer){
        sdf.execute(cmdBuffer);
    });
    device.computeCommandPool().oneTimeCommand([&](auto cmdBuffer){
        emitter.execute(cmdBuffer, particles.descriptorSets[0]);
        emitter.execute(cmdBuffer, particles.descriptorSets[1]);
    });
    numParticles = emitter.numParticles();
    particles.constants.numParticles = numParticles;
    spdlog::info("{} particles emitted", particles.constants.numParticles);
}

void SPHFluidSimulation::computeMass() {
    particleProperties.kernelRadius = particleProperties.kernelRadiusOverTargetSpacing * particleProperties.targetSpacing;
    auto radius = particleProperties.kernelRadius * 1.5f;
    auto targetSpacing = particleProperties.targetSpacing;
    BoundingBox box{ -glm::vec3(radius), glm::vec3(radius) };

    pointGenerator = PointGenerator{&device, &descriptorPool, &particles.descriptorSetLayout, box, targetSpacing};
    pointGenerator.init();

    device.computeCommandPool().oneTimeCommand([&](auto cmdBuffer){
        pointGenerator.execute(cmdBuffer, particles.descriptorSets[0]);
    });


    numParticles = pointGenerator.numParticles();
    particles.constants.numParticles = numParticles;
    auto kernel = StdKernel(particleProperties.kernelRadius);
    spdlog::info("calculating mass for num particles {} with kernel radius: {}", numParticles, particleProperties.kernelRadius);

    float maxDensity = std::numeric_limits<float>::min();

    particles.buffers[0].map<glm::vec4>([&](glm::vec4* ptr){
        for(auto i = 0; i < numParticles; i++){
            float sum = 0.0f;
            glm::vec3 point = ptr[i].xyz();
            for(auto j = 0; j < numParticles; j++){
                glm::vec3 neighbour = ptr[j].xyz();
                sum += kernel(distance(point, neighbour));
            }
            maxDensity = std::max(maxDensity, sum);
        }
    });

    float mass = particleProperties.targetDensity/maxDensity;
    spdlog::info("running sim with particle mass of {}", mass);
    assert(mass > 0);
    particleProperties.mass = mass;
    applyExternalForces.constants.invMass = 1.0f/mass;
    timeIntegration.constants.invMass = 1.0f/mass;
}




int main(){
    try{

        Settings settings;
        settings.depthTest = true;
        settings.enabledFeatures.wideLines = true;
        settings.queueFlags |= VK_QUEUE_COMPUTE_BIT;
        auto app = SPHFluidSimulation{ settings };
        app.run();
    }catch(std::runtime_error& err){
        spdlog::error(err.what());
    }
}