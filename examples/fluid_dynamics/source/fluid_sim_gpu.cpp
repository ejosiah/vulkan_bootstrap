#include "fluid_sim_gpu.h"

#include <utility>

FluidSim::FluidSim(VulkanDevice *device, FileManager fileMgr, VulkanBuffer u0, VulkanBuffer v0,
                   VulkanBuffer u, VulkanBuffer v, VulkanBuffer q0, VulkanBuffer q, int N, float dt,
                   float dissipation)
        : ComputePipelines{ device }
        , _fileManager{ std::move(fileMgr) }
        , _u0{ std::move(u0) }
        , _v0{ std::move(v0) }
        , _u{std::move( u )}
        , _v{std::move( v )}
        , _q0{std::move( q0 )}
        , _q{std::move( q )}
{
    _constants.N = N;
    _constants.dt = dt;
    _constants.dissipation = dissipation;
    VkDeviceSize size = (_constants.N + 2) * (_constants.N + 2) * sizeof(float);
    _divBuffer = device->createBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, size, "divergence");
    _pressure = device->createBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, size, "pressure");
    _gu = device->createBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, size, "pressure_gradient_u");
    _gv = device->createBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, size, "pressure_gradient_v");
    sc[0] = sc[1] = N;
}

void FluidSim::init() {
    initTextureSampler();
    createDescriptorPool();
    createDescriptorSets();
    updateDescriptorSets();
    createPipelines();
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
                             _velocitySetLayout, _velocitySetLayout, _quantitySetLayout, _quantitySetLayout,
                             _scalaFieldSetLayout, _scalaFieldSetLayout, _velocitySetLayout
                           });

    _velocityDescriptorSet[0] = sets[0];
    _velocityDescriptorSet[1] = sets[1];

    _quantityDescriptorSet[0] = sets[2];
    _quantityDescriptorSet[1] = sets[3];

    _divergenceDescriptorSet = sets[4];
    _pressureDescriptorSet = sets[5];
    _pressureGradientDescriptorSet = sets[6];
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
    auto writes = initializers::writeDescriptorSets<7>();
    writes[0].dstSet = _velocityDescriptorSet[0];
    writes[0].dstBinding = 0;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[0].descriptorCount = 1;
    VkDescriptorBufferInfo u0Info{_u0, 0, VK_WHOLE_SIZE};
    writes[0].pBufferInfo = &u0Info;

    writes[1].dstSet = _velocityDescriptorSet[0];
    writes[1].dstBinding = 1;
    writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[1].descriptorCount = 1;
    VkDescriptorBufferInfo v0Info{_v0, 0, VK_WHOLE_SIZE};
    writes[1].pBufferInfo = &v0Info;

    writes[2].dstSet = _quantityDescriptorSet[0];
    writes[2].dstBinding = 0;
    writes[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[2].descriptorCount = 1;
    VkDescriptorBufferInfo q0Info{_q0, 0, VK_WHOLE_SIZE};
    writes[2].pBufferInfo = &q0Info;

    writes[3].dstSet = _velocityDescriptorSet[1];
    writes[3].dstBinding = 0;
    writes[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[3].descriptorCount = 1;
    VkDescriptorBufferInfo uInfo{_u, 0, VK_WHOLE_SIZE};
    writes[3].pBufferInfo = &uInfo;

    writes[4].dstSet = _velocityDescriptorSet[1];
    writes[4].dstBinding = 1;
    writes[4].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[4].descriptorCount = 1;
    VkDescriptorBufferInfo vInfo{_v, 0, VK_WHOLE_SIZE};
    writes[4].pBufferInfo = &vInfo;

    writes[5].dstSet = _quantityDescriptorSet[1];
    writes[5].dstBinding = 0;
    writes[5].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[5].descriptorCount = 1;
    VkDescriptorBufferInfo qInfo{_q, 0, VK_WHOLE_SIZE};
    writes[5].pBufferInfo = &qInfo;
    
    writes[6].dstSet = _divergenceDescriptorSet;
    writes[6].dstBinding = 0;
    writes[6].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[6].descriptorCount = 1;
    VkDescriptorBufferInfo divInfo{_divBuffer, 0, VK_WHOLE_SIZE};
    writes[6].pBufferInfo = &divInfo;

    device->updateDescriptorSets(writes);
}

void FluidSim::run(float speed) {

}

void
FluidSim::resize(VulkanBuffer u0, VulkanBuffer v0, VulkanBuffer u, VulkanBuffer v, VulkanBuffer q0, VulkanBuffer q,
                 int N) {
    _u0 = std::move(u0);
    _v0 = std::move(v0);
    _u = std::move(u);
    _v = std::move(v);
    _q0 = std::move(q0);
    _q = std::move(q);
    _constants.N = N;
    sc[0] = N;
    sc[1] = N;
    updateDescriptorSets();
    createPipelines();
}

void FluidSim::advect(VkCommandBuffer commandBuffer) {
    static std::array<VkDescriptorSet, 3> sets;
    sets[0] = _velocityDescriptorSet[0];
    sets[1] = _quantityDescriptorSet[q_in];
    sets[2] = _quantityDescriptorSet[q_out];
    uint32_t workGroups = glm::max(_constants.N/32, 1);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline("advect"));
    vkCmdPushConstants(commandBuffer, layout("advect"), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(_constants), &_constants);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, layout("advect"), 0, COUNT(sets), sets.data(), 0, VK_NULL_HANDLE);
    vkCmdDispatch(commandBuffer, workGroups, workGroups, 1);
    std::swap(q_in, q_out);
}

void FluidSim::calculateDivergence(VkCommandBuffer commandBuffer) {
    const int N = _constants.N;
    static std::array<VkDescriptorSet, 2> sets;
    sets[0] = _velocityDescriptorSet[0];
    sets[1] = _divergenceDescriptorSet;
    uint32_t workGroups = glm::max(N/32, 1);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline("divergence"));
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, layout("divergence"), 0, COUNT(sets), sets.data(), 0, VK_NULL_HANDLE);
    vkCmdPushConstants(commandBuffer, layout("advect"), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(int), &N);
    vkCmdDispatch(commandBuffer, workGroups, workGroups, 1);
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
            }
    };
}

std::string FluidSim::resource(const std::string &path) {
    return _fileManager.getFullPath(path)->string();
}

void FluidSim::setQuantity(VulkanBuffer q0, VulkanBuffer q) {
    _q0 = std::move(q0);
    _q = std::move(q);
    updateDescriptorSets();
}

void FluidSim::dissipation(float value) {
    _constants.dissipation = value;
}
