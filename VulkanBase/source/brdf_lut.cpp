#include "Texture.h"
#include "VulkanShaderModule.h"

namespace textures{


    struct Pipeline{
        VulkanPipeline pipeline;
        VulkanPipelineLayout layout;
    };

    Texture createTexture1(const VulkanDevice& device){
        Texture texture;
        VkImageCreateInfo createInfo = initializers::imageCreateInfo(VK_IMAGE_TYPE_2D
                , VK_FORMAT_R32G32B32A32_SFLOAT
                , VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT, 512, 512);
        texture.image = device.createImage(createInfo);

        texture.image.transitionLayout(device.graphicsCommandPool(), VK_IMAGE_LAYOUT_GENERAL, DEFAULT_SUB_RANGE
                                       , 0, VK_ACCESS_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

        texture.imageView = texture.image.createView(VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_VIEW_TYPE_2D, DEFAULT_SUB_RANGE);

        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

        texture.sampler = device.createSampler(samplerInfo);
        return texture;
    }

    VulkanDescriptorPool createDescriptorPool1(const VulkanDevice &device) {
        constexpr uint32_t maxSets = 1;
        std::array<VkDescriptorPoolSize, 1> poolSizes{
                {
                        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1},
                }
        };
        return device.createDescriptorPool(maxSets, poolSizes, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT);
    }

    VulkanDescriptorSetLayout createDescriptorSetLayout1(const VulkanDevice& device){
        return
            device.descriptorSetLayoutBuilder()
                .binding(0)
                    .descriptorType(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
                    .descriptorCount(1)
                    .shaderStages(VK_SHADER_STAGE_COMPUTE_BIT)
                .createLayout();
    }

    void updateDescriptorSet1(const VulkanDevice& device, const Texture& texture, VkDescriptorSet descriptorSet){
        auto writes = initializers::writeDescriptorSets<1>();
        auto& write = writes[0];

        write.dstSet = descriptorSet;
        write.dstBinding = 0;
        write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        write.descriptorCount = 1;
        VkDescriptorImageInfo imageInfo{VK_NULL_HANDLE, texture.imageView, VK_IMAGE_LAYOUT_GENERAL};
        write.pImageInfo = &imageInfo;

        device.updateDescriptorSets(writes);
    }

    Pipeline createPipeline1(const VulkanDevice& device, const VulkanDescriptorSetLayout& setLayout){
        auto module = VulkanShaderModule{ "../../data/shaders/pbr/integrate_brdf.comp.spv", device};
        auto stage = initializers::shaderStage({ module, VK_SHADER_STAGE_COMPUTE_BIT});

        Pipeline compute;

        std::vector<VkDescriptorSetLayout> setLayouts{ setLayout};
        compute.layout = device.createPipelineLayout( setLayouts );

        auto computeCreateInfo = initializers::computePipelineCreateInfo();
        computeCreateInfo.stage = stage;
        computeCreateInfo.layout = compute.layout;

        compute.pipeline = device.createComputePipeline(computeCreateInfo);

        return compute;
    }

    Texture brdf_lut(const VulkanDevice& device){
        auto texture = createTexture1(device);
        auto descriptorPool = createDescriptorPool1(device);
        auto setLayout = createDescriptorSetLayout1(device);
        auto descriptorSet = descriptorPool.allocate( {setLayout }).front();
        updateDescriptorSet1(device, texture, descriptorSet);

        auto pipeline = createPipeline1(device, setLayout);

        device.graphicsCommandPool().oneTimeCommand([&](auto commandBuffer){
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.pipeline);
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.layout, 0, 1, &descriptorSet, 0, VK_NULL_HANDLE);
            vkCmdDispatch(commandBuffer, 1, 512, 512);
            texture.image.transitionLayout(commandBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, DEFAULT_SUB_RANGE
                                           , VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT
                                           , VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
        });

        return texture;
    }
}