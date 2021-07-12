#include "SPHFluidSimulation.hpp"
#include "sampling.hpp"
#include "vulkan_util.h"

SPHFluidSimulation::SPHFluidSimulation(const Settings& settings) : VulkanBaseApp("SPH Fluid Simulation", settings) {

}

void SPHFluidSimulation::initApp() {
    initCamera();
    createDescriptorPool();
    createDescriptorSetLayouts();
    createCommandPool();
    initGrid();
    createParticles();
    createSdf();
    createEmitter();
//    initGridBuilder();
    createDescriptorSets();
    createRenderDescriptorSet();
    createPipelineCache();
    createRenderPipeline();
    createComputePipeline();
    device.computeCommandPool().oneTimeCommand([&](auto cmdBuffer){
        sdf.execute(cmdBuffer);
    });
    device.computeCommandPool().oneTimeCommand([&](auto cmdBuffer){
        emitter.execute(cmdBuffer, particles.descriptorSets[0]);
        emitter.execute(cmdBuffer, particles.descriptorSets[1]);
    });
    particles.constants.numParticles = emitter.numParticles();
    spdlog::info("{} particles emitted", particles.constants.numParticles);
//    createGridBuilderDescriptorSetLayout();
//    createGridBuilderDescriptorSet(gridBuilder.buffer.size - sizeof(uint32_t), sizeof(uint32_t));
//    createGridBuilderPipeline();
//    buildPointHashGrid();
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
    std::vector<VkDescriptorSetLayoutBinding> bindings(1);
    bindings[0].binding = 0;
    bindings[0].descriptorCount = 1;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    particles.descriptorSetLayout = device.createDescriptorSetLayout(bindings);
    
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    render.setLayout = device.createDescriptorSetLayout(bindings);
}

void SPHFluidSimulation::createDescriptorSets() {
    descriptorPool.allocate({ particles.descriptorSetLayout, particles.descriptorSetLayout }, particles.descriptorSets);

    std::array<VkWriteDescriptorSet, 2> writes = initializers::writeDescriptorSets<2>();

    VkDescriptorBufferInfo info0{ particles.buffers[0], 0, VK_WHOLE_SIZE};
    writes[0].dstSet = particles.descriptorSets[0];
    writes[0].dstBinding = 0;
    writes[0].descriptorCount = 1;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[0].pBufferInfo = &info0;

    VkDescriptorBufferInfo info1{ particles.buffers[1], 0, VK_WHOLE_SIZE};
    writes[1].dstSet = particles.descriptorSets[1];
    writes[1].dstBinding = 0;
    writes[1].descriptorCount = 1;
    writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[1].pBufferInfo = &info1;

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
    commandBuffers = commandPool.allocate(swapChainImageCount);
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
            {0, sizeof(Particle), VK_VERTEX_INPUT_RATE_VERTEX}
    };
    attribs = std::vector<VkVertexInputAttributeDescription>{
            {0, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetOf(Particle, position)},
            {1, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetOf(Particle, color)},
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
    grid.xforms.model = glm::scale(glm::mat4{1}, {1/swapChain.aspectRatio(), 1, 1});
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
    VkDeviceSize offset = 0;
//    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, render.pipeline);
//    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, render.layout, 0, 1, &render.descriptorSet, 0, nullptr);
//    vkCmdBindVertexBuffers(commandBuffer, 0, 1, grid.vertexBuffer, &offset);
//    vkCmdDraw(commandBuffer, grid.vertexBuffer.size/sizeof(glm::vec3), 1, 0, 0);

    // render particles
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, particles.pipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, render.layout, 0, 1, &render.descriptorSet, 0, nullptr);
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, particles.buffers[0], &offset);
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
//    runPhysics();
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

void SPHFluidSimulation::initGrid() {
    std::vector<glm::vec3> vertices;
    float n = std::sqrtf(grid.numCells);
    for(int i = 0; i < int(n) + 1; i++){
        float y = 2.0f * float(i)/n - 1;
        float x = 2.0f * float(i)/n - 1;

        // rows
        vertices.emplace_back(-1, y, 0);
        vertices.emplace_back(1, y, 0);

        // columns
        vertices.emplace_back(x, -1, 0);
        vertices.emplace_back(x, 1, 0);
    }
    
    grid.vertexBuffer = device.createDeviceLocalBuffer(vertices.data(), sizeof(glm::vec3) * vertices.size(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    grid.xforms.model = glm::scale(glm::mat4{1}, {1/swapChain.aspectRatio(), 1, 1});
    grid.xforms.projection = vkn::ortho(-1.01, 1.01, -1.01, 1.01, -1, 1);

    render.mvpBuffer = device.createCpuVisibleBuffer(&grid.xforms, sizeof(grid.xforms), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
}

void SPHFluidSimulation::createParticles() {
    std::random_device rnd;
    std::default_random_engine engine{ rnd() };
//    std::normal_distribution<float> dist{0, 0.3};
    std::uniform_real_distribution<float> dist{-0.9, 0.9};
    auto rngPosition = [&]{
        return glm::vec4(dist(engine), dist(engine), 0, 1);
    };

    auto rngVelocity = [&]{
        static std::default_random_engine engine{ rnd() };
        static std::uniform_real_distribution<float> dist{-1, 1};
        static std::uniform_real_distribution<float> dist1{0, 2};

        return glm::vec3{dist(engine), dist1(engine), 0};
    };

    auto rngColor = [&]{
        static std::default_random_engine engine{ rnd() };
        static std::uniform_real_distribution<float> dist{0, 1};

        return glm::vec4{dist(engine), dist(engine), dist(engine), 1};
    };

    std::vector<Particle> vertices;
//    for(auto i = 0; i < particles.constants.numParticles; i++){
//        Particle p{};
//        p.position = rngPosition();
////        p.position = glm::vec4(0, -1 ,0, 1);
//        p.color = rngColor();
//        p.velocity = rngVelocity();
//        p.invMass = 0.1;
//        vertices.push_back(p);
//    }

    float gridSpacing = float(grid.size)/std::sqrt(float(grid.numCells));
//    glm::vec3 origin = glm::vec3(1, 2, 0) * gridSpacing;
//    origin.x += gridSpacing * 0.8f;
//    origin.y += gridSpacing * 0.2f;

    std::vector<glm::vec3> expectedNeighbourList;
    auto rng = rngFunc(0.0f, 1.0f, 1 << 20);
    for(int i = 0; i < 100; i++){
        glm::vec3 point{ rng(), rng(), 0};
        expectedNeighbourList.push_back(point);
    }
//    auto rng = canonicalRng(1 << 20);
//    for(int i = 0; i < 1; i++) {
//        glm::vec2 uv { rng(), rng()};
//        auto neighbour = glm::vec3(origin.xy() + gridSpacing * 0.45f * sampling::uniformSampleDisk(uv), 0);
//        expectedNeighbourList.push_back(neighbour);
//    }
//
//    for(int i = 0; i < 100; i++){
//        float theta = float(i)/100.0f * glm::two_pi<float>();
//        auto position = glm::vec3(origin.xy() +  gridSpacing * glm::vec2(std::cos(theta), std::sin(theta)), 0);
//        expectedNeighbourList.push_back(position);
//    }

    for(auto& position : expectedNeighbourList){
        Particle p{};
        p.position = glm::vec4( 2.0f * position - 1.0f , 1);
        p.position.z = 0;
        p.color = rngColor();
        p.velocity = rngVelocity();
        p.invMass = 0.1;
        vertices.push_back(p);
    }
    vertices.front().color = glm::vec4(1);
    vertices.front().color = glm::vec4(1);

    particles.buffers[0] = device.createBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_ONLY, sizeof(Particle) * MAX_PARTICLES);
    particles.buffers[1] = device.createBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_ONLY, sizeof(Particle) * MAX_PARTICLES);
}

void SPHFluidSimulation::runPhysics() {
    int inIndex = currentFrame % 2;
    static std::array<VkDescriptorSet, 2> sets{};
    sets[0] = particles.descriptorSets[inIndex];
    sets[1] = particles.descriptorSets[1 - inIndex];

    device.graphicsCommandPool().oneTimeCommand([&](VkCommandBuffer commandBuffer){
        particles.constants.gravity = glm::vec3(0, -0.98, 0);
    //    particles.constants.numParticles = std::min(1 << frameCount, 1024);
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, compute.pipeline);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, compute.layout, 0, COUNT(sets), sets.data(), 0, nullptr);
        vkCmdPushConstants(commandBuffer, compute.layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(particles.constants), &particles.constants);
        vkCmdDispatch(commandBuffer, (particles.constants.numParticles - 1)/1024 + 1, 1, 1);
    });
}

void SPHFluidSimulation::initGridBuilder() {
    float n = std::sqrtf(float(grid.numCells));
    gridBuilder.constants.resolution = glm::uvec3(n, n, 1);
    gridBuilder.constants.gridSpacing = grid.size/n;
    gridBuilder.constants.numParticles = particles.constants.numParticles;

    // allocate buffer for bucketSize, nextBucketIndex and single value for buckets for pass 0
    VkDeviceSize size = 2 * grid.numCells * sizeof(int) + sizeof(int);
    gridBuilder.buffer = device.createBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, size);
}

void SPHFluidSimulation::createGridBuilderDescriptorSetLayout() {
    std::vector<VkDescriptorSetLayoutBinding> bindings(3);
    bindings[0].binding = 0;
    bindings[0].descriptorCount = 1;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    bindings[1].binding = 1;
    bindings[1].descriptorCount = 1;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    bindings[2].binding = 2;
    bindings[2].descriptorCount = 1;
    bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[2].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

   gridBuilder.descriptorSetLayout = device.createDescriptorSetLayout(bindings);
}

void SPHFluidSimulation::createGridBuilderDescriptorSet(VkDeviceSize offset, VkDeviceSize range) {
    gridBuilder.descriptorSet = descriptorPool.allocate({ gridBuilder.descriptorSetLayout}).front();
    auto writes = initializers::writeDescriptorSets<3>();


    VkDeviceSize size = grid.numCells * sizeof(uint32_t);
    VkDescriptorBufferInfo  bucketSizeInfo{ gridBuilder.buffer, 0, size };
    writes[0].dstSet = gridBuilder.descriptorSet;
    writes[0].dstBinding = 1;
    writes[0].descriptorCount = 1;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[0].pBufferInfo = &bucketSizeInfo;

    VkDescriptorBufferInfo  nextBucketIndexInfo{ gridBuilder.buffer, size, size };
    writes[1].dstSet = gridBuilder.descriptorSet;
    writes[1].dstBinding = 2;
    writes[1].descriptorCount = 1;
    writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[1].pBufferInfo = &nextBucketIndexInfo;

    VkDescriptorBufferInfo bucketInfo{ gridBuilder.buffer, 2 * size, range };
    writes[2].dstSet = gridBuilder.descriptorSet;
    writes[2].dstBinding = 0;
    writes[2].descriptorCount = 1;
    writes[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[2].pBufferInfo = &bucketInfo;

    device.updateDescriptorSets(writes);
}

void SPHFluidSimulation::createGridBuilderPipeline() {
    auto gridBuilderModule = VulkanShaderModule{"../../data/shaders/sph/point_hash_grid_builder.comp.spv", device};
    auto stage = initializers::shaderStage({gridBuilderModule, VK_SHADER_STAGE_COMPUTE_BIT});
    auto offset = alignedSize(sizeof(Camera) + sizeof(particles.constants), 16);
    std::vector<VkPushConstantRange> ranges{{VK_SHADER_STAGE_COMPUTE_BIT, offset, sizeof(gridBuilder.constants)}};
    gridBuilder.layout = device.createPipelineLayout({ particles.descriptorSetLayout, gridBuilder.descriptorSetLayout }, ranges);

    auto createInfo = initializers::computePipelineCreateInfo();
    createInfo.stage = stage;
    createInfo.layout = gridBuilder.layout;

    gridBuilder.pipeline = device.createComputePipeline(createInfo, pipelineCache);
}


void SPHFluidSimulation::buildPointHashGrid() {
    GeneratePointHashGrid(0);
    VkDeviceSize size = 0;
    gridBuilder.buffer.map<int>([&](auto itr){
        VkDeviceSize end =  grid.numCells;
        for(int i = 0; i < end; i++ ){
            auto gridSize = *(itr + i);
            size += gridSize;
        }
    });
    gridBuilder.buffer = device.createBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, size + 2 * sizeof(int) * grid.numCells);
    createGridBuilderDescriptorSet(0, size);
    GeneratePointHashGrid(1);
}

void SPHFluidSimulation::GeneratePointHashGrid(int pass) {
    device.graphicsCommandPool().oneTimeCommand([&](auto commandBuffer){
        uint32_t offset = alignedSize(sizeof(Camera) + sizeof(particles.constants), 16);
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, gridBuilder.pipeline);

        gridBuilder.constants.pass = pass;
        vkCmdPushConstants(commandBuffer, gridBuilder.layout, VK_SHADER_STAGE_COMPUTE_BIT, offset,
                           sizeof(gridBuilder.constants), &gridBuilder.constants);

        static std::array<VkDescriptorSet, 2> sets;
        sets[0] = particles.descriptorSets[currentFrame%2];
        sets[1] = gridBuilder.descriptorSet;

        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, gridBuilder.layout, 0, COUNT(sets), sets.data(), 0, nullptr);
        vkCmdDispatch(commandBuffer, particles.constants.numParticles/1024 + 1, 1, 1);
    });
}

void SPHFluidSimulation::createEmitter() {
    emitter = VolumeParticleEmitter{&device, &descriptorPool, &particles.descriptorSetLayout, sdf.sdf, 0.02f};
    emitter.init();
}

void SPHFluidSimulation::createSdf() {
    sdf = SdfCompute{ &device, &descriptorPool, BoundingBox{glm::vec3(-0.5, 0, -0.5), glm::vec3(0.5, 2, 0.5) },
                      "../../data/sph/water_drop.comp.spv", glm::ivec3{128}};
}

void SPHFluidSimulation::initCamera() {
    OrbitingCameraSettings settings{};
    settings.offsetDistance = 5.0f;
    settings.rotationSpeed = 0.1f;
    settings.orbitMinZoom = -5.0f;
    settings.fieldOfView = 45.0f;
    settings.modelHeight = 0;
    settings.aspectRatio = static_cast<float>(swapChain.extent.width)/static_cast<float>(swapChain.extent.height);
    camera = std::make_unique<OrbitingCameraController>(device, swapChain.imageCount(), currentImageIndex, dynamic_cast<InputManager&>(*this), settings);
}

void SPHFluidSimulation::addParticleBufferBarrier(VkCommandBuffer commandBuffer, int index) {
    addBufferMemoryBarriers(commandBuffer, { &particles.buffers[index] }
    , VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
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