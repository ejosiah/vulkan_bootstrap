#include "fluid_sim_gpu.h"

#include <utility>

FluidSim::FluidSim(VulkanDevice *device, FileManager fileMgr, VulkanBuffer u0, VulkanBuffer v0,
                   VulkanBuffer u, VulkanBuffer v, int N, float dt, float dissipation)
        : ComputePipelines{ device }
        , _fileManager{ std::move(fileMgr) }
{
    _u[0] = u0;
    _v[0] = v0;
    _u[1] = u;
    _v[1] = v;
    _constants.N = N;
    _constants.dt = dt;
    _constants.dissipation = dissipation;
    sc[0] = sc[1] = N;
}

void FluidSim::init() {
    createSupportBuffers();
    initTextureSampler();
    createDescriptorPool();
    createDescriptorSets();
    updateDescriptorSets();
    createPipelines();
}

void FluidSim::createSupportBuffers() {
    VkDeviceSize size = (_constants.N + 2) * (_constants.N + 2) * sizeof(float);
    _divBuffer = device->createBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, size, "divergence");
    _pressure = device->createBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, size, "pressure");
    _gu = device->createBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, size, "pressure_gradient_u");
    _gv = device->createBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, size, "pressure_gradient_v");
}

void FluidSim::initTextureSampler() {
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

    textureSampler = device->createSampler(samplerInfo);
}

void FluidSim::createDescriptorPool() {
    constexpr uint32_t maxSets = 100;
    std::array<VkDescriptorPoolSize, 6> poolSizes{
            {
                    {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 100 * maxSets},
                    {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 100 * maxSets},
                    { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 100 * maxSets },
                    { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 100 * maxSets },
                    { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 100 * maxSets },
                    { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 100 * maxSets },
            }
    };
    _descriptorPool = device->createDescriptorPool(maxSets, poolSizes, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT);

}

void FluidSim::createDescriptorSets() {
    _velocitySetLayout =
        device->descriptorSetLayoutBuilder()
            .binding(0)
                .descriptorType(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
                .descriptorCount(1)
                .shaderStages(VK_SHADER_STAGE_COMPUTE_BIT)
            .binding(1)
                .descriptorType(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
                .descriptorCount(1)
                .shaderStages(VK_SHADER_STAGE_COMPUTE_BIT)
                .createLayout();

    _quantitySetLayout =
        device->descriptorSetLayoutBuilder()
            .binding(0)
                .descriptorType(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
                .descriptorCount(1)
                .shaderStages(VK_SHADER_STAGE_COMPUTE_BIT)
            .createLayout();

    _scalaFieldSetLayout =
        device->descriptorSetLayoutBuilder()
            .binding(0)
                .descriptorType(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
                .descriptorCount(1)
                .shaderStages(VK_SHADER_STAGE_COMPUTE_BIT)
            .createLayout();


    auto sets = _descriptorPool.allocate({
                             _velocitySetLayout, _velocitySetLayout, _scalaFieldSetLayout, _scalaFieldSetLayout,
                             _velocitySetLayout, _quantitySetLayout, _quantitySetLayout, _quantitySetLayout,
                             _quantitySetLayout
                           });

    _velocityDescriptorSet[0] = sets[0];
    _velocityDescriptorSet[1] = sets[1];

    _divergenceDescriptorSet = sets[2];
    _pressureDescriptorSet = sets[3];
    _pressureGradientDescriptorSet = sets[4];

    _quantityDescriptorSets[VELOCITY_FIELD_U][0] = sets[5];
    _quantityDescriptorSets[VELOCITY_FIELD_U][1] = sets[6];
    _quantityDescriptorSets[VELOCITY_FIELD_V][0] = sets[7];
    _quantityDescriptorSets[VELOCITY_FIELD_V][1] = sets[8];
    setVectorFieldQuantity();

}

void FluidSim::setVectorFieldQuantity() {
    _quantityBuffer[VELOCITY_FIELD_U][0] = _u[0];
    _quantityBuffer[VELOCITY_FIELD_U][1] = _u[1];

    _quantityBuffer[VELOCITY_FIELD_V][0] = _v[0];
    _quantityBuffer[VELOCITY_FIELD_V][1] = _v[1];

    _quantityIn[VELOCITY_FIELD_U] = 0;
    _quantityOut[VELOCITY_FIELD_U] = 1;
    _quantityIn[VELOCITY_FIELD_V] = 0;
    _quantityOut[VELOCITY_FIELD_V] = 1;
}

void FluidSim::set(Texture &color) {
    _outputColor = &color;
    textures::create(*device, scratchColors[0], VK_IMAGE_TYPE_2D, VK_FORMAT_R8G8B8A8_UNORM, {color.width, color.height, 1}, VK_SAMPLER_ADDRESS_MODE_REPEAT);
    textures::create(*device, scratchColors[1], VK_IMAGE_TYPE_2D, VK_FORMAT_R8G8B8A8_UNORM, {color.width, color.height, 1}, VK_SAMPLER_ADDRESS_MODE_REPEAT);

    device->graphicsCommandPool().oneTimeCommand([&](VkCommandBuffer commandBuffer){
        VkImageSubresourceRange subresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        textures::copy(commandBuffer, color, scratchColors[0]);
        scratchColors[0].image.transitionLayout(commandBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, subresourceRange, 0, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_NONE, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
        scratchColors[1].image.transitionLayout(commandBuffer, VK_IMAGE_LAYOUT_GENERAL, subresourceRange, 0, VK_ACCESS_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_NONE, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
    });
}

void FluidSim::updateDescriptorSets() {
    auto writes = initializers::writeDescriptorSets<12>();
    writes[0].dstSet = _velocityDescriptorSet[0];
    writes[0].dstBinding = 0;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[0].descriptorCount = 1;
    VkDescriptorBufferInfo u0Info{_u[0], 0, VK_WHOLE_SIZE};
    writes[0].pBufferInfo = &u0Info;

    writes[1].dstSet = _velocityDescriptorSet[0];
    writes[1].dstBinding = 1;
    writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[1].descriptorCount = 1;
    VkDescriptorBufferInfo v0Info{_v[0], 0, VK_WHOLE_SIZE};
    writes[1].pBufferInfo = &v0Info;

    writes[2].dstSet = _velocityDescriptorSet[1];
    writes[2].dstBinding = 0;
    writes[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[2].descriptorCount = 1;
    VkDescriptorBufferInfo uInfo{_u[1], 0, VK_WHOLE_SIZE};
    writes[2].pBufferInfo = &uInfo;

    writes[3].dstSet = _velocityDescriptorSet[1];
    writes[3].dstBinding = 1;
    writes[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[3].descriptorCount = 1;
    VkDescriptorBufferInfo vInfo{_v[1], 0, VK_WHOLE_SIZE};
    writes[3].pBufferInfo = &vInfo;
    
    writes[4].dstSet = _divergenceDescriptorSet;
    writes[4].dstBinding = 0;
    writes[4].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[4].descriptorCount = 1;
    VkDescriptorBufferInfo divInfo{_divBuffer, 0, VK_WHOLE_SIZE};
    writes[4].pBufferInfo = &divInfo;


    writes[5].dstSet = _pressureGradientDescriptorSet;
    writes[5].dstBinding = 0;
    writes[5].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[5].descriptorCount = 1;
    VkDescriptorBufferInfo guInfo{_gu, 0, VK_WHOLE_SIZE};
    writes[5].pBufferInfo = &guInfo;

    writes[6].dstSet = _pressureGradientDescriptorSet;
    writes[6].dstBinding = 1;
    writes[6].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[6].descriptorCount = 1;
    VkDescriptorBufferInfo gvInfo{_gv, 0, VK_WHOLE_SIZE};
    writes[6].pBufferInfo = &gvInfo;

    writes[7].dstSet = _pressureDescriptorSet;
    writes[7].dstBinding = 0;
    writes[7].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[7].descriptorCount = 1;
    VkDescriptorBufferInfo pInfo{_pressure, 0, VK_WHOLE_SIZE};
    writes[7].pBufferInfo = &pInfo;


    writes[8].dstSet = _quantityDescriptorSets[VELOCITY_FIELD_U][0];
    writes[8].dstBinding = 0;
    writes[8].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[8].descriptorCount = 1;
    VkDescriptorBufferInfo u0qInfo{_u[0], 0, VK_WHOLE_SIZE};
    writes[8].pBufferInfo = &u0qInfo;

    writes[9].dstSet = _quantityDescriptorSets[VELOCITY_FIELD_V][0];
    writes[9].dstBinding = 0;
    writes[9].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[9].descriptorCount = 1;
    VkDescriptorBufferInfo v0qInfo{_v[0], 0, VK_WHOLE_SIZE};
    writes[9].pBufferInfo = &v0qInfo;

    writes[10].dstSet = _quantityDescriptorSets[VELOCITY_FIELD_U][1];
    writes[10].dstBinding = 0;
    writes[10].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[10].descriptorCount = 1;
    VkDescriptorBufferInfo uqInfo{_u[1], 0, VK_WHOLE_SIZE};
    writes[10].pBufferInfo = &uqInfo;

    writes[11].dstSet = _quantityDescriptorSets[VELOCITY_FIELD_V][1];
    writes[11].dstBinding = 0;
    writes[11].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[11].descriptorCount = 1;
    VkDescriptorBufferInfo vqInfo{_v[1], 0, VK_WHOLE_SIZE};
    writes[11].pBufferInfo = &vqInfo;

    device->updateDescriptorSets(writes);
}

void FluidSim::run(float speed) {

}

void FluidSim::velocityStep(VkCommandBuffer commandBuffer, float dt, float viscosity) {
    int iterations = 80;
//    diffuseVectorField(commandBuffer, iterations);
//    swapVectorFieldBuffers();
//    project(commandBuffer, iterations); // u -> u0
//    swapVectorFieldBuffers();
    advectVectorField(commandBuffer);
    swapVectorFieldBuffers();
    project(commandBuffer, iterations); // u -> u0
    swapVectorFieldBuffers();
}

void FluidSim::quantityStep(VkCommandBuffer commandBuffer, const Quantity &quantity, float dt, float dissipation) {}

void
FluidSim::resize(VulkanBuffer u0, VulkanBuffer v0, VulkanBuffer u, VulkanBuffer v, int N) {
    _u[0] = std::move(u0);
    _v[0] = std::move(v0);
    _u[1] = std::move(u);
    _v[1] = std::move(v);
    _constants.N = N;
    sc[0] = N;
    sc[1] = N;
    createSupportBuffers();
    setVectorFieldQuantity();
    updateDescriptorSets();
    createPipelines();
}

void FluidSim::advectVectorField(VkCommandBuffer commandBuffer) {
    advect(commandBuffer, VELOCITY_FIELD_U, HORIZONTAL_COMPONENT_BOUNDARY); // u0 -> u
    advect(commandBuffer, VELOCITY_FIELD_V, VERTICAL_COMPONENT_BOUNDARY);
}

void FluidSim::advect(VkCommandBuffer commandBuffer, const Quantity &quantity, int boundary) {
    static std::array<VkDescriptorSet, 3> sets;
    sets[0] = _velocityDescriptorSet[v_in];
    sets[1] = inputDescriptorSet(quantity);
    sets[2] = outputDescriptorSet(quantity);
    const auto& buffer = outputBuffer(quantity);

    uint32_t workGroups = glm::max(_constants.N/32, 1);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline("advect"));
    vkCmdPushConstants(commandBuffer, layout("advect"), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(_constants), &_constants);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, layout("advect"), 0, COUNT(sets), sets.data(), 0, VK_NULL_HANDLE);
    vkCmdDispatch(commandBuffer, workGroups, workGroups, 1);

    barrier(commandBuffer, buffer);
    setBoundary(commandBuffer, quantity, boundary);
}

void FluidSim::calculateDivergence(VkCommandBuffer commandBuffer) {
    const int N = _constants.N;
    static std::array<VkDescriptorSet, 2> sets;
    sets[0] = _velocityDescriptorSet[v_in];
    sets[1] = _divergenceDescriptorSet;
    uint32_t workGroups = glm::max(N/32, 1);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline("divergence"));
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, layout("divergence"), 0, COUNT(sets), sets.data(), 0, VK_NULL_HANDLE);
    vkCmdPushConstants(commandBuffer, layout("divergence"), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(int), &N);
    vkCmdDispatch(commandBuffer, workGroups, workGroups, 1);
    barrier(commandBuffer, _divBuffer);
    setBoundary(commandBuffer, _divergenceDescriptorSet, _divBuffer, SCALAR_FIELD_BOUNDARY);
    barrier(commandBuffer, _divBuffer);
}

void FluidSim::diffuseVectorField(VkCommandBuffer commandBuffer, int iterations) {
    diffuse(commandBuffer, VELOCITY_FIELD_U, HORIZONTAL_COMPONENT_BOUNDARY, iterations); // u0 -> u
    diffuse(commandBuffer, VELOCITY_FIELD_V, VERTICAL_COMPONENT_BOUNDARY, iterations);
}

void FluidSim::diffuse(VkCommandBuffer commandBuffer, const Quantity& quantity, int boundary, int iterations) {
    auto in = _quantityIn[quantity];
    auto out = _quantityOut[quantity];
    const auto& oldQuantityDescriptorSet = descriptorSet(quantity)[in];
    const auto& newQuantityDescriptorSet = descriptorSet(quantity)[out];
    const auto& buffer = _quantityBuffer[quantity][out];

    const int N = _constants.N;
    static std::array<VkDescriptorSet, 2> sets;
    sets[0] = newQuantityDescriptorSet;
    sets[1] = oldQuantityDescriptorSet;
    uint32_t workGroups = glm::max(N/32, 1);

    auto dx = 1.f/float(N);
    jacobiConstants.alpha = (dx * dx)/(_constants.dissipation * _constants.dt);
    jacobiConstants.rBeta = 1.0f/(4.0f + jacobiConstants.alpha);
    jacobiConstants.N = N;
    for(int i = 0; i < iterations; i++){
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline("jacobi"));
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, layout("jacobi"), 0, COUNT(sets), sets.data(), 0, VK_NULL_HANDLE);
        vkCmdPushConstants(commandBuffer, layout("jacobi"), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(jacobiConstants), &jacobiConstants);
        vkCmdDispatch(commandBuffer, workGroups, workGroups, 1);

        barrier(commandBuffer, buffer);
        setBoundary(commandBuffer, quantity, boundary);
        barrier(commandBuffer, buffer);
    }
}

void FluidSim::project(VkCommandBuffer commandBuffer, int iterations) {
    clearSupportBuffers(commandBuffer);
    calculateDivergence(commandBuffer);
    solvePressureEquation(commandBuffer, iterations);
    computePressureGradient(commandBuffer);
    computeDivergenceFreeField(commandBuffer);
    setBoundary(commandBuffer, VELOCITY_FIELD_U, HORIZONTAL_COMPONENT_BOUNDARY);
    setBoundary(commandBuffer, VELOCITY_FIELD_V, VERTICAL_COMPONENT_BOUNDARY);
    barrier(commandBuffer, {_u[v_out], _v[v_out]});
}

void FluidSim::clearSupportBuffers(VkCommandBuffer commandBuffer) {
    vkCmdFillBuffer(commandBuffer, _pressure, 0, VK_WHOLE_SIZE, 0);
    vkCmdFillBuffer(commandBuffer, _divBuffer, 0, VK_WHOLE_SIZE, 0);
    barrier(commandBuffer, {_pressure, _divBuffer});
}

void FluidSim::swapVectorFieldBuffers() {
    std::swap(v_in, v_out);
    std::swap(_quantityIn[VELOCITY_FIELD_U], _quantityOut[VELOCITY_FIELD_U]);
    std::swap(_quantityIn[VELOCITY_FIELD_V], _quantityOut[VELOCITY_FIELD_V]);
}

void FluidSim::computePressureGradient(VkCommandBuffer commandBuffer) {
    const int N = _constants.N;
    static std::array<VkDescriptorSet, 2> sets;
    sets[0] = _pressureDescriptorSet;
    sets[1] = _pressureGradientDescriptorSet;
    uint32_t workGroups = glm::max(N/32, 1);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline("gradient"));
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, layout("gradient"), 0, COUNT(sets), sets.data(), 0, VK_NULL_HANDLE);
    vkCmdPushConstants(commandBuffer, layout("divergence"), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(int), &N);
    vkCmdDispatch(commandBuffer, workGroups, workGroups, 1);
    barrier(commandBuffer, {_gu, _gv});
}

void FluidSim::solvePressureEquation(VkCommandBuffer commandBuffer, int iterations) {
    const int N = _constants.N;
    static std::array<VkDescriptorSet, 2> sets;
    sets[0] = _pressureDescriptorSet;
    sets[1] = _divergenceDescriptorSet;
    uint32_t workGroups = glm::max(N/32, 1);

    auto dx = 1.f/float(N);
    jacobiConstants.alpha = -(dx * dx);
    jacobiConstants.rBeta = 0.25f;
    jacobiConstants.N = N;
    for(int i = 0; i < iterations; i++){
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline("jacobi"));
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, layout("jacobi"), 0, COUNT(sets), sets.data(), 0, VK_NULL_HANDLE);
        vkCmdPushConstants(commandBuffer, layout("jacobi"), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(jacobiConstants), &jacobiConstants);
        vkCmdDispatch(commandBuffer, workGroups, workGroups, 1);

        barrier(commandBuffer, _pressure);
        setBoundary(commandBuffer, _pressureDescriptorSet, _pressure, SCALAR_FIELD_BOUNDARY);
        barrier(commandBuffer, _pressure);
    }
}

void FluidSim::computeDivergenceFreeField(VkCommandBuffer commandBuffer) {
    const int N = _constants.N;
    uint32_t workGroups = glm::max(_constants.N/32, 1);
    static std::array<VkDescriptorSet, 3> sets;
    sets[0] = _velocityDescriptorSet[v_in];
    sets[1] = _velocityDescriptorSet[v_out];
    sets[2] = _pressureGradientDescriptorSet;

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline("divergence_free_field"));
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, layout("divergence_free_field"), 0, COUNT(sets), sets.data(), 0, VK_NULL_HANDLE);
    vkCmdPushConstants(commandBuffer, layout("divergence_free_field"), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(int), &N);
    vkCmdDispatch(commandBuffer, workGroups, workGroups, 1);
    barrier(commandBuffer, {_u[v_out], _v[v_out]});
}

void FluidSim::setBoundary(VkCommandBuffer commandBuffer, const Quantity &quantity, int boundary) {
    auto descriptorSet = outputDescriptorSet(quantity);
    const auto buffer = outputBuffer(quantity);

    setBoundary(commandBuffer, descriptorSet, buffer, boundary);
}

void FluidSim::setBoundary(VkCommandBuffer commandBuffer, VkDescriptorSet descriptorSet, VulkanBuffer buffer, int boundary) {
    _boundaryConstants.N = _constants.N;
    _boundaryConstants.boundary = boundary;
    uint32_t workGroups = glm::max(_constants.N/32, 1);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline("boundary"));
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, layout("boundary"), 0, 1, &descriptorSet, 0, VK_NULL_HANDLE);
    vkCmdPushConstants(commandBuffer, layout("boundary"), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(_boundaryConstants), &_boundaryConstants);
    vkCmdDispatch(commandBuffer, workGroups, 1, 1);

    VkBufferMemoryBarrier barrier{ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER, nullptr, VK_ACCESS_SHADER_WRITE_BIT
                                   ,(VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT), 0
                                   , 0,  buffer, 0, VK_WHOLE_SIZE};

    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0
                         , VK_NULL_HANDLE, 1, &barrier, 0, VK_NULL_HANDLE);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline("boundary_1"));
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, layout("boundary_1"), 0, 1, &descriptorSet, 0, VK_NULL_HANDLE);
    vkCmdPushConstants(commandBuffer, layout("boundary_1"), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(_boundaryConstants), &_boundaryConstants);
    vkCmdDispatch(commandBuffer, 1, 1, 1);
}

std::vector<PipelineMetaData> FluidSim::pipelineMetaData() {
    return {{
                    "advect",
                    resource("advect.comp.spv"),
                    { &_velocitySetLayout, &_quantitySetLayout, &_quantitySetLayout},
                    { {VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(_constants)}},
                    {
                            {
                                    {0, 0, sizeof(int)},
                                    {1, sizeof(int), sizeof(int)}
                            },
                            sc.data(),
                            BYTE_SIZE(sc)
                    }
            },
            {
                "divergence",
                    resource("divergence.comp.spv"),
                    { &_velocitySetLayout, &_scalaFieldSetLayout},
                    {{VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(int)}}
            },
            {
                "gradient",
                    resource("gradient.comp.spv"),
                    {&_scalaFieldSetLayout, &_velocitySetLayout},
                    {{VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(int)}}
            },
            {
                    "divergence_free_field",
                    resource("divergence_free_field.comp.spv"),
                    {&_velocitySetLayout, &_velocitySetLayout, &_velocitySetLayout},
                    {{VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(int)}}
            },
            {
                "boundary",
                    resource("boundary.comp.spv"),
                    {&_quantitySetLayout},
                    {{VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(_boundaryConstants)}}
            },
            {
                "boundary_1",
                    resource("boundary_1.comp.spv"),
                    {&_quantitySetLayout},
                    {{VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(_boundaryConstants)}}
            },
            {
                "jacobi",
                    resource("jacobi.comp.spv"),
                    {&_quantitySetLayout, &_quantitySetLayout},
                    {{VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(jacobiConstants)}}
            }
    };
}

std::string FluidSim::resource(const std::string &path) {
    return _fileManager.getFullPath(path)->string();
}

void FluidSim::dissipation(float value) {
    _constants.dissipation = value;
}

void FluidSim::barrier(VkCommandBuffer commandBuffer, const VulkanBuffer& buffer) {
    VkBufferMemoryBarrier barrier{ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER, nullptr, VK_ACCESS_SHADER_WRITE_BIT
            ,(VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT), 0
            , 0,  buffer, 0, VK_WHOLE_SIZE};

    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0
            , VK_NULL_HANDLE, 1, &barrier, 0, VK_NULL_HANDLE);
}

void FluidSim::barrier(VkCommandBuffer commandBuffer, const std::vector<VulkanBuffer> &buffers) {
    std::vector<VkBufferMemoryBarrier> barriers;
    for(const auto& buffer : buffers){
        VkBufferMemoryBarrier barrier{ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER, nullptr, VK_ACCESS_SHADER_WRITE_BIT
                ,(VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT), 0
                , 0,  buffer, 0, VK_WHOLE_SIZE};
        barriers.push_back(barrier);
    }

    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0
            , VK_NULL_HANDLE, COUNT(barriers), barriers.data(), 0, VK_NULL_HANDLE);
}

void FluidSim::setQuantity(const Quantity &quantity, VulkanBuffer q0, VulkanBuffer q) {
    auto sets = _descriptorPool.allocate({ _quantitySetLayout, _quantitySetLayout });
    std::array<VkDescriptorSet, 2> descriptorSets{ sets[0], sets[1]};
    std::array<VulkanBuffer, 2> buffers{ std::move(q0), std::move(q)};
    _quantityDescriptorSets[quantity] = descriptorSets;
    _quantityBuffer[quantity] = buffers;
    _quantityIn[quantity] = 0;
    _quantityOut[quantity] = 1;
    
    auto writes = initializers::writeDescriptorSets<2>();
    
    writes[0].dstSet = _quantityDescriptorSets[quantity][0];
    writes[0].dstBinding = 0;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[0].descriptorCount = 1;
    VkDescriptorBufferInfo q0Info{_quantityBuffer[quantity][0], 0, VK_WHOLE_SIZE};
    writes[0].pBufferInfo = &q0Info;

    writes[1].dstSet = _quantityDescriptorSets[quantity][1];
    writes[1].dstBinding = 0;
    writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[1].descriptorCount = 1;
    VkDescriptorBufferInfo qInfo{_quantityBuffer[quantity][1], 0, VK_WHOLE_SIZE};
    writes[1].pBufferInfo = &qInfo;

    device->updateDescriptorSets(writes);
}

std::array<VkDescriptorSet, 2> &FluidSim::descriptorSet(const Quantity &quantity) {
    assert(_quantityDescriptorSets.find(quantity) != _quantityDescriptorSets.end());
    return _quantityDescriptorSets[quantity];
}

const VulkanBuffer &FluidSim::inputBuffer(const Quantity &quantity) {
    assert(_quantityBuffer.find(quantity) != _quantityBuffer.end());
    return _quantityBuffer[quantity][_quantityIn[quantity]];
}

const VulkanBuffer &FluidSim::outputBuffer(const Quantity &quantity) {
    assert(_quantityBuffer.find(quantity) != _quantityBuffer.end());
    return _quantityBuffer[quantity][_quantityOut[quantity]];
}

VkDescriptorSet FluidSim::inputDescriptorSet(const Quantity &quantity) {
    return descriptorSet(quantity)[_quantityIn[quantity]];
}

VkDescriptorSet FluidSim::outputDescriptorSet(const Quantity &quantity) {
    return descriptorSet(quantity)[_quantityOut[quantity]];
}


