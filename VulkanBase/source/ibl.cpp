#include "Texture.h"
#include "VulkanShaderModule.h"
#include <array>
#include <vector>

class IblCreator{
public:
    static constexpr uint32_t mipLevels = 5;
    Texture irradianceMap;
    Texture specularMap;
    std::array<VulkanImageView, mipLevels> specularMipViews;

    IblCreator(const VulkanDevice& dev, const Texture& eMap, VkImageLayout fLayout)
    : device{dev}
    , envMap{eMap}
    , finalLayout{fLayout}
    {

        createDescriptorPool();
        createDescriptorSetLayouts();

        createTextures();
        updateDescriptorSets();
        createPipelines();
    }

    void createTextures(){
        assert(envMap.width != 0 && envMap.height != 0);
        irradianceMap.width = 512 ;
        irradianceMap.height = 512 ;
        specularMap.width = 1024;
        specularMap.height = 1024;
        create(irradianceMap);
        createSpecular(specularMap);
    }

    void create(Texture& texture){
        VkImageCreateInfo createInfo = initializers::imageCreateInfo(VK_IMAGE_TYPE_2D
                , VK_FORMAT_R32G32B32A32_SFLOAT
                , VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, texture.width, texture.height);
        texture.image = device.createImage(createInfo);

        texture.image.transitionLayout(device.graphicsCommandPool(), VK_IMAGE_LAYOUT_GENERAL, DEFAULT_SUB_RANGE
                , 0, VK_ACCESS_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

        texture.imageView = texture.image.createView(VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_VIEW_TYPE_2D, DEFAULT_SUB_RANGE);

        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

        texture.sampler = device.createSampler(samplerInfo);
    }

    void createSpecular(Texture& texture){
        VkImageCreateInfo createInfo = initializers::imageCreateInfo(VK_IMAGE_TYPE_2D
                , VK_FORMAT_R32G32B32A32_SFLOAT
                , VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, texture.width, texture.height);
        createInfo.mipLevels = mipLevels;
        texture.image = device.createImage(createInfo);

        auto subRange = initializers::imageSubresourceRange();
        subRange.levelCount = mipLevels;

        texture.image.transitionLayout(device.graphicsCommandPool(), VK_IMAGE_LAYOUT_GENERAL, subRange
                , 0, VK_ACCESS_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

        for(auto i = 0; i < mipLevels; i++){
            subRange.baseMipLevel = i;
            subRange.levelCount = mipLevels - i;
            auto imageView = texture.image.createView(VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_VIEW_TYPE_2D, subRange);
            specularMipViews[i] = std::move(imageView);
        }

        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.maxLod = mipLevels;

        texture.sampler = device.createSampler(samplerInfo);
    }

    void createDescriptorPool(){
        constexpr uint32_t maxSets = 50;
        std::array<VkDescriptorPoolSize, 2> poolSizes{
                {
                        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, mipLevels * maxSets},
                        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2 * maxSets},
                }
        };
        descriptorPool = device.createDescriptorPool(maxSets, poolSizes, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT);
    }

    void createDescriptorSetLayouts(){
        envMapSetLayout =
            device.descriptorSetLayoutBuilder()
                .binding(0)
                    .descriptorType(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
                    .descriptorCount(1)
                    .shaderStages(VK_SHADER_STAGE_COMPUTE_BIT)
                .createLayout();

        setLayout =
            device.descriptorSetLayoutBuilder()
                .binding(0)
                    .descriptorType(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
                    .descriptorCount(1)
                    .shaderStages(VK_SHADER_STAGE_COMPUTE_BIT)
                .createLayout();
    }

    void updateDescriptorSets(){
        auto sets = descriptorPool.allocate( { envMapSetLayout, setLayout, setLayout, setLayout, setLayout, setLayout, setLayout });
        envMapDescriptorSet = sets[0];
        irradianceCompute.descriptorSet = sets[1];

        for(int i = 0; i < mipLevels; i++){
            specular.descriptorSet[i] = sets[i + 2];
        }

        auto writes = initializers::writeDescriptorSets<2>();

        writes[0].dstSet = envMapDescriptorSet;
        writes[0].dstBinding = 0;
        writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writes[0].descriptorCount = 1;
        VkDescriptorImageInfo envMapInfo{ envMap.sampler, envMap.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
        writes[0].pImageInfo = &envMapInfo;

        writes[1].dstSet = irradianceCompute.descriptorSet;
        writes[1].dstBinding = 0;
        writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        writes[1].descriptorCount = 1;
        VkDescriptorImageInfo irrInfo{ VK_NULL_HANDLE, irradianceMap.imageView, VK_IMAGE_LAYOUT_GENERAL};
        writes[1].pImageInfo = &irrInfo;

        device.updateDescriptorSets(writes);

        specular.baseDescriptorSet = descriptorPool.allocate({envMapSetLayout, envMapSetLayout, envMapSetLayout, envMapSetLayout});

        // specular
        writes = initializers::writeDescriptorSets<9>();

        std::array<VkDescriptorImageInfo, 4> baseInfo{};
        for(int i = 0; i < 4; i++) {
            writes[i].dstSet = specular.baseDescriptorSet[i];
            writes[i].dstBinding = 0;
            writes[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            writes[i].descriptorCount = 1;
            baseInfo[i] = VkDescriptorImageInfo {envMap.sampler, specularMipViews[i], VK_IMAGE_LAYOUT_GENERAL};
            writes[i].pImageInfo = &baseInfo[i];
        }

        std::array<VkDescriptorImageInfo, mipLevels> specInfo{};
        for(int i = 0; i < mipLevels; i++) {
            writes[i + 4].dstSet = specular.descriptorSet[i];
            writes[i + 4].dstBinding = 0;
            writes[i + 4].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            writes[i + 4].descriptorCount = 1;
            specInfo[i] = VkDescriptorImageInfo {VK_NULL_HANDLE, specularMipViews[i], VK_IMAGE_LAYOUT_GENERAL};
            writes[i + 4].pImageInfo = &specInfo[i];
        }

        device.updateDescriptorSets(writes);
    }

    void createPipelines(){
        auto module = VulkanShaderModule{ "../../data/shaders/pbr/irradiance_map.comp.spv", device};
        auto stage = initializers::shaderStage({ module, VK_SHADER_STAGE_COMPUTE_BIT});

        std::vector<VkDescriptorSetLayout> setLayouts{ envMapSetLayout, setLayout };
        irradianceCompute.layout = device.createPipelineLayout( setLayouts );

        auto computeCreateInfo = initializers::computePipelineCreateInfo();
        computeCreateInfo.stage = stage;
        computeCreateInfo.layout = irradianceCompute.layout;

        irradianceCompute.pipeline = device.createComputePipeline(computeCreateInfo);

        // create specular map compute pipeline
        module = VulkanShaderModule{ "../../data/shaders/pbr/specular_map.comp.spv", device};
        stage = initializers::shaderStage({ module, VK_SHADER_STAGE_COMPUTE_BIT});
        specular.layout = device.createPipelineLayout( setLayouts, { {VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(float)} });

        computeCreateInfo.stage = stage;
        computeCreateInfo.layout = specular.layout;
        specular.pipeline = device.createComputePipeline(computeCreateInfo);
    }

    void computeIrradianceMap(){
        device.graphicsCommandPool().oneTimeCommand([&](auto commandBuffer){
            std::array<VkDescriptorSet, 2> sets{envMapDescriptorSet, irradianceCompute.descriptorSet};
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, irradianceCompute.pipeline);
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, irradianceCompute.layout
                                    , 0, COUNT(sets), sets.data(), 0, VK_NULL_HANDLE);

            auto size = 512;
            vkCmdDispatch(commandBuffer, 1, size, size);
            irradianceMap.image.transitionLayout(commandBuffer, finalLayout, DEFAULT_SUB_RANGE
                    , VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT
                    , VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
        });
    }

    void computeSpecularMap(){
        device.graphicsCommandPool().oneTimeCommand([&](auto commandBuffer){
            uint32_t width = specularMap.width;
            uint32_t height = specularMap.height;

            std::array<VkDescriptorSet, 2> sets{envMapDescriptorSet, specular.descriptorSet[0]};
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, specular.pipeline);
            vkCmdPushConstants(commandBuffer, specular.layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(float), &specular.roughness);
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, specular.layout
                    , 0, COUNT(sets), sets.data(), 0, VK_NULL_HANDLE);

            vkCmdDispatch(commandBuffer, 1, width, height);

            VkImageMemoryBarrier barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.image = specularMap.image;
            barrier.subresourceRange = DEFAULT_SUB_RANGE;
            barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            for(auto mip = 1u; mip < mipLevels; mip++ ){
                width = specularMap.width >> mip;
                height = specularMap.height >> mip;
                specular.roughness = static_cast<float>(mip) / static_cast<float>(mipLevels - 1);
                barrier.subresourceRange.baseMipLevel = mip - 1;

                vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT
                                     , 0, 0, VK_NULL_HANDLE, 0, VK_NULL_HANDLE, 1, &barrier);

                sets[0] = specular.baseDescriptorSet[mip - 1];
                sets[1] = specular.descriptorSet[mip];

                spdlog::info("generating specular map, mip level {}, roughness {}, size {}", mip, specular.roughness, width);
                vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, specular.pipeline);
                vkCmdPushConstants(commandBuffer, specular.layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(float), &specular.roughness);
                vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, specular.layout
                        , 0, COUNT(sets), sets.data(), 0, VK_NULL_HANDLE);

                vkCmdDispatch(commandBuffer, 1, width, height);
            }

            auto subResource = DEFAULT_SUB_RANGE;
            subResource.levelCount = mipLevels;
            specularMap.image.transitionLayout(commandBuffer, finalLayout, subResource
                    , VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT
                    , VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
        });
        specularMap.imageView = std::move(specularMipViews.front());
    }

    void compute(){
        computeIrradianceMap();
        computeSpecularMap();
    }


private:
    const VulkanDevice& device;
    const Texture& envMap;
    VulkanDescriptorPool descriptorPool;
    VulkanDescriptorSetLayout envMapSetLayout;
    VkDescriptorSet envMapDescriptorSet{VK_NULL_HANDLE};
    VulkanDescriptorSetLayout setLayout;
    VkImageLayout finalLayout;
    struct {
        VulkanPipelineLayout layout;
        VulkanPipeline pipeline;
        VkDescriptorSet descriptorSet{VK_NULL_HANDLE};
    } irradianceCompute;

    struct{
        VulkanPipelineLayout layout;
        VulkanPipeline pipeline;
        std::vector<VkDescriptorSet> baseDescriptorSet{};
        std::array<VkDescriptorSet, mipLevels> descriptorSet{};
        float roughness{0};
    } specular;
};

namespace textures {

    void ibl(const VulkanDevice &device, const Texture &envMap, Texture &irradianceMap, Texture &specularMap, VkImageLayout finalLayout) {
        IblCreator ibl{device, envMap, finalLayout};
        ibl.compute();
        irradianceMap = std::move(ibl.irradianceMap);
        specularMap = std::move(ibl.specularMap);
    }

}
