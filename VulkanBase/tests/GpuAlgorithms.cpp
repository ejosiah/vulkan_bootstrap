#include "VulkanFixture.hpp"
#include "gpu/algorithm.h"
#include "algorithm"

class GpuAlgorithms : public VulkanFixture{
protected:
    void postVulkanInit() override {
        gpu::init(device, _fileManager);
    }

    void TearDown() override {
        gpu::shutdown();
    }
};

TEST_F(GpuAlgorithms, averageLargeValues){

    std::vector<float> data(1 << 20);
    auto rng = rngFunc<float>(0.0f, 100.0f, 1 << 200);
    std::generate(begin(data), end(data), [&]{ return rng(); });

    auto sum = std::accumulate(begin(data), end(data), 0.0f);
    auto expected = sum/static_cast<float>(data.size());
    VulkanBuffer buffer = device.createDeviceLocalBuffer(data.data(), BYTE_SIZE(data), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    auto actual = gpu::average(buffer);

    ASSERT_NEAR(expected, actual, 0.01);
}

TEST_F(GpuAlgorithms, averageValuesNearZero){
    std::vector<float> data(1 << 20);
    auto rng = rngFunc<float>(-0.1f, 0.1f, 1 << 20);
    std::generate(begin(data), end(data), [&]{ return rng(); });

    auto sum = std::accumulate(begin(data), end(data), 0.0f);
    auto expected = sum/static_cast<float>(data.size());
    VulkanBuffer buffer = device.createDeviceLocalBuffer(data.data(), BYTE_SIZE(data), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    auto actual = gpu::average(buffer);

    ASSERT_NEAR(expected, actual, 0.00001);
}

TEST_F(GpuAlgorithms, averageWithExternalDescriptor){
    std::vector<float> data(1 << 20);
    auto rng = rngFunc<int>(0, 10, 1 << 20);
    std::generate(begin(data), end(data), [&]{ return float(rng()); });

    VulkanDescriptorSetLayout setLayout =
            device.descriptorSetLayoutBuilder()
                    .binding(0)
                    .descriptorType(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
                    .descriptorCount(1)
                    .shaderStages(VK_SHADER_STAGE_COMPUTE_BIT)
                    .binding(1)
                    .descriptorType(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
                    .descriptorCount(1)
                    .shaderStages(VK_SHADER_STAGE_COMPUTE_BIT)
                    .createLayout();
    VkDescriptorSet descriptorSet = descriptorPool.allocate( { setLayout }).front();

    VulkanBuffer input = device.createCpuVisibleBuffer(data.data(), BYTE_SIZE(data), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    VulkanBuffer output = device.createBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, sizeof(float));

    auto writes = initializers::writeDescriptorSets<2>();

    writes[0].dstSet = descriptorSet;
    writes[0].dstBinding = 0;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[0].descriptorCount = 1;
    VkDescriptorBufferInfo inInfo{ input, 0, VK_WHOLE_SIZE};
    writes[0].pBufferInfo = &inInfo;

    writes[1].dstSet = descriptorSet;
    writes[1].dstBinding = 1;
    writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[1].descriptorCount = 1;
    VkDescriptorBufferInfo outInfo{ output, 0, VK_WHOLE_SIZE};
    writes[1].pBufferInfo = &outInfo;

    device.updateDescriptorSets(writes);

    device.computeCommandPool().oneTimeCommand([&](auto commandBuffer){
        gpu::average(commandBuffer, descriptorSet , input);
    });

    auto actual = *reinterpret_cast<float*>(output.map());
    fmt::print("actual: {}\n", actual);
    float expected = std::accumulate(begin(data), end(data), 0.0f)/data.size();
    output.unmap();
    ASSERT_NEAR(expected, actual, 0.001);

}

TEST_F(GpuAlgorithms, reduceAdd){
    std::vector<float> data(1 << 20);
    auto rng = rngFunc<int>(0, 10, 1 << 20);
    std::generate(begin(data), end(data), [&]{ return float(rng()); });

    VulkanDescriptorSetLayout setLayout =
        device.descriptorSetLayoutBuilder()
            .binding(0)
                .descriptorType(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
                .descriptorCount(1)
                .shaderStages(VK_SHADER_STAGE_COMPUTE_BIT)
            .binding(1)
                .descriptorType(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
                .descriptorCount(1)
                .shaderStages(VK_SHADER_STAGE_COMPUTE_BIT)
            .createLayout();
    VkDescriptorSet descriptorSet = descriptorPool.allocate( { setLayout }).front();
    
    VulkanBuffer input = device.createCpuVisibleBuffer(data.data(), BYTE_SIZE(data), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    VulkanBuffer output = device.createBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, sizeof(float));
    
    auto writes = initializers::writeDescriptorSets<2>();
    
    writes[0].dstSet = descriptorSet;
    writes[0].dstBinding = 0;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[0].descriptorCount = 1;
    VkDescriptorBufferInfo inInfo{ input, 0, VK_WHOLE_SIZE};
    writes[0].pBufferInfo = &inInfo;

    writes[1].dstSet = descriptorSet;
    writes[1].dstBinding = 1;
    writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[1].descriptorCount = 1;
    VkDescriptorBufferInfo outInfo{ output, 0, VK_WHOLE_SIZE};
    writes[1].pBufferInfo = &outInfo;

    device.updateDescriptorSets(writes);

    device.computeCommandPool().oneTimeCommand([&](auto commandBuffer){
        gpu::reduce(commandBuffer, descriptorSet , input);
    });

    auto actual = *reinterpret_cast<float*>(output.map());
    fmt::print("actual: {}\n", actual);
    float expected = std::accumulate(begin(data), end(data), 0.0f);
    output.unmap();
    ASSERT_NEAR(expected, actual, 0.001);
    
}