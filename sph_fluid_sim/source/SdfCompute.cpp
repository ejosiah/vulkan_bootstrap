#include "SdfCompute.hpp"

SdfCompute::SdfCompute(VulkanDevice* device, VulkanDescriptorPool* pool, BoundingBox domain, std::string sdfPath, glm::uvec3 resolution)
: ComputePipelines{ device }
, pool { pool }
, domain{ domain }
, sdfPath{ std::move(sdfPath) }
, resolution{ resolution }{
    init();
}

void SdfCompute::init() {
    createBuffers();
    createImage();
    createDescriptorSetLayout();
    createDescriptorSet();
    createPipelines();
}

void SdfCompute::createBuffers() {
    buffer = device->createDeviceLocalBuffer(&domain, sizeof(BoundingBox), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
}

std::vector<PipelineMetaData> SdfCompute::pipelineMetaData() {
    return {
            {
                    "water_drop",
                    "../../data/shaders/sph/water_drop.comp.spv",
                    {  &setLayout }
            }
    };
}

void SdfCompute::execute(VkCommandBuffer commandBuffer) {
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline("water_drop"));
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, layout("water_drop"), 0, 1, &descriptorSet, 0,
                            nullptr);
    vkCmdDispatch(commandBuffer, resolution.x/8, resolution.y/8, resolution.z/8);
}

void SdfCompute::createDescriptorSetLayout() {
    std::vector<VkDescriptorSetLayoutBinding> bindings(2);
    bindings[0].binding = 0;
    bindings[0].descriptorCount = 1;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    
    bindings[1].binding = 1;
    bindings[1].descriptorCount = 1;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    
    setLayout = device->createDescriptorSetLayout(bindings);
}

void SdfCompute::createDescriptorSet() {
    descriptorSet = pool->allocate({ setLayout }).front();
    
    auto writes = initializers::writeDescriptorSets<2>( descriptorSet );
    
    VkDescriptorImageInfo imageInfo{ sdf.texture.sampler, sdf.texture.imageView, VK_IMAGE_LAYOUT_GENERAL};
    writes[0].dstBinding = 0;
    writes[0].descriptorCount = 1;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    writes[0].pImageInfo = &imageInfo;

    VkDescriptorBufferInfo bufferInfo{ buffer, 0, VK_WHOLE_SIZE};
    writes[1].dstBinding = 1;
    writes[1].descriptorCount = 1;
    writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[1].pBufferInfo = &bufferInfo;

    device->updateDescriptorSets(writes);
}

void SdfCompute::createImage() {
    auto imageCreateInfo = initializers::imageCreateInfo(
            VK_IMAGE_TYPE_3D
            , VK_FORMAT_R32_SFLOAT
            , VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
            , resolution.x
            , resolution.y
            , resolution.z);
    sdf.texture.image = device->createImage(imageCreateInfo);
    VkImageSubresourceRange subresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
    sdf.texture.image.transitionLayout(device->graphicsCommandPool(), VK_IMAGE_LAYOUT_GENERAL, subresourceRange, 0,
                                       VK_ACCESS_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

    sdf.texture.imageView = sdf.texture.image.createView(VK_FORMAT_R32_SFLOAT, VK_IMAGE_VIEW_TYPE_3D, subresourceRange);

    VkSamplerCreateInfo samplerCreateInfo = initializers::samplerCreateInfo();
    samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerCreateInfo.magFilter = VK_FILTER_NEAREST;
    samplerCreateInfo.minFilter = VK_FILTER_NEAREST;
    samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;


    sdf.texture.sampler = device->createSampler(samplerCreateInfo);
    sdf.domain = domain;
}
