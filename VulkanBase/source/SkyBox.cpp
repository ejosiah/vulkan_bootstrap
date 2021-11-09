#include "common.h"
#include "SkyBox.hpp"
#include "primitives.h"
#include "VulkanDevice.h"
#include "GraphicsPipelineBuilder.hpp"
#include  <stb_image.h>
#include "DescriptorSetBuilder.hpp"

VulkanDevice* SkyBox::g_device = nullptr;
VulkanPipeline SkyBox::g_pipeline;
VulkanPipelineLayout SkyBox::g_layout;
VulkanDescriptorSetLayout SkyBox::g_descriptorSetLayout;
VulkanDescriptorPool SkyBox::g_descriptorPool;
VulkanBaseApp* SkyBox::g_app{nullptr };


void SkyBox::init(VulkanBaseApp* app) {
    g_app = app;
    g_device = &app->device;
    g_device->registerDisposeListener([&](const auto& _){
        SkyBox::destroy();
    });

    createDescriptorPool();
    createDescriptorSetLayout();
    createPipeline();
}

void SkyBox::create(SkyBox &skyBox, const std::string& directory, const std::array<std::string, 6>& faceNames) {
    auto cube = primitives::cube({1, 0, 0, 1});
    std::vector<glm::vec3> vertices;
    for(const auto& vertex : cube.vertices){
        vertices.push_back(vertex.position.xyz());
    }
    skyBox.cube.vertices = device().createDeviceLocalBuffer(vertices.data(), BYTE_SIZE(vertices)
                                                          , VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    skyBox.cube.indices = device().createDeviceLocalBuffer(cube.indices.data(), BYTE_SIZE(cube.indices)
                                                      , VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    skyBox.pipeline = &g_pipeline;
    skyBox.layout = &g_layout;

    load(skyBox.texture, directory, faceNames);
    updateDescriptorSet(skyBox);
}

void SkyBox::destroy() {
    dispose(g_pipeline);
    dispose(g_layout);
    dispose(g_descriptorPool);
    dispose(g_descriptorSetLayout);
}

void SkyBox::createPipeline() {
    g_pipeline =
        device().graphicsPipelineBuilder()
            .shaderStage()
                .vertexShader("../../data/shaders/skybox/skybox.vert.spv")
                .fragmentShader("../../data/shaders/skybox/skybox.frag.spv")
            .vertexInputState()
                .addVertexBindingDescription(0, sizeof(glm::vec3), VK_VERTEX_INPUT_RATE_VERTEX)
                .addVertexAttributeDescription(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0)
            .inputAssemblyState()
                .triangles()
            .viewportState()
                .viewport()
                    .origin(0, 0)
                    .dimension(g_app->swapChain.extent)
                    .minDepth(0)
                    .maxDepth(1)
                .scissor()
                    .offset(0, 0)
                    .extent(g_app->swapChain.extent)
                .add()
            .rasterizationState()
                .cullFrontFace()
                .frontFaceCounterClockwise()
                .polygonModeFill()
            .multisampleState()
                .rasterizationSamples(g_app->settings.msaaSamples)
            .depthStencilState()
                .enableDepthTest()
                .enableDepthWrite()
                .compareOpLessOrEqual()
                .minDepthBounds(0)
                .maxDepthBounds(1)
            .colorBlendState()
                .attachment()
                .add()
            .layout()
                .addDescriptorSetLayout(g_descriptorSetLayout)
                .addPushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Camera))
            .renderPass(g_app->renderPass)
            .subpass(0)
            .name("skybox")
        .build(g_layout);
}

VulkanDevice &SkyBox::device() {
    return *g_device;
}

void SkyBox::load(Texture &skyboxTexture, const std::string& directory, const std::array<std::string, 6>& faceNames) {
    int texWidth = 0;
    int texHeight = 0;
    int prevWidth = 0;
    int prevHeight = 0;
    int texChannels;

    std::vector<stbi_uc*> faces;
    for(const auto& faceName : faceNames){
        auto path = fmt::format("{}\\{}", directory, faceName);
        if(prevWidth != texWidth || prevHeight != texHeight){
            throw std::runtime_error{"sky box faces must have the same width / height"};
        }
        auto pixels = stbi_load(path.data(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        faces.push_back(pixels);
        prevWidth = texWidth;
        prevHeight = texHeight;
    }

    VkDeviceSize faceSize = texWidth * texHeight * STBI_rgb_alpha;
    VkDeviceSize imageSize = faceSize * 6;
    VulkanBuffer stagingBuffer = device().createBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY, imageSize);

    for(auto i = 0; i < 6; i++){
        stagingBuffer.copy(faces[i], faceSize, faceSize * i);
    }

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    imageInfo.extent = { uint32_t(texWidth), uint32_t(texHeight), 1u};
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 6;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    auto& commandPool = device().graphicsCommandPool();
    skyboxTexture.image = device().createImage(imageInfo, VMA_MEMORY_USAGE_GPU_ONLY);

    VkImageSubresourceRange subresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 6};
    skyboxTexture.image.transitionLayout(commandPool, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, subresourceRange);

    std::array<VkBufferImageCopy, 6> copyRegions{};

    for(auto face = 0; face < 6; face++){
        VkBufferImageCopy region{};
        region.bufferOffset = face * faceSize;

        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = face;
        region.imageSubresource.layerCount = 1;
        region.imageExtent.width = texWidth;
        region.imageExtent.height = texHeight;
        region.imageExtent.depth = 1;

        copyRegions[face] = region;
    }

    device().graphicsCommandPool().oneTimeCommand([&](auto commandBuffer){
        vkCmdCopyBufferToImage(commandBuffer, stagingBuffer, skyboxTexture.image
                               , VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
                               ,COUNT(copyRegions), copyRegions.data());
    });


    skyboxTexture.image.transitionLayout(commandPool, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, subresourceRange);

    VkImageViewCreateInfo viewInfo{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    viewInfo.image = skyboxTexture.image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
    viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    viewInfo.subresourceRange = subresourceRange;

    VkImageView view;
    ASSERT(vkCreateImageView(device(), &viewInfo, nullptr, &view));
    skyboxTexture.imageView = VulkanImageView{ device(), view };

    VkSamplerCreateInfo  sampler = initializers::samplerCreateInfo();
    sampler.magFilter = VK_FILTER_LINEAR;
    sampler.minFilter = VK_FILTER_LINEAR;
    sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler.addressModeV = sampler.addressModeU;
    sampler.addressModeW = sampler.addressModeU;
    sampler.mipLodBias = 0.0f;
    sampler.compareOp = VK_COMPARE_OP_NEVER;
    sampler.minLod = 0.0f;
    sampler.maxLod = 1;
    sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    sampler.maxAnisotropy = 1.0f;

    if(g_app->settings.enabledFeatures.samplerAnisotropy){
        sampler.maxAnisotropy = device().getLimits().maxSamplerAnisotropy;
        sampler.anisotropyEnable = VK_TRUE;
    }

    skyboxTexture.sampler = device().createSampler(sampler);

    for(const auto& pixels : faces){
        stbi_image_free(pixels);
    }
}

void SkyBox::createDescriptorSetLayout() {
    g_descriptorSetLayout =
        device().descriptorSetLayoutBuilder()
            .binding(0)
                .descriptorCount(1)
                .descriptorType(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
                .shaderStages(VK_SHADER_STAGE_FRAGMENT_BIT)
            .createLayout();

}

void SkyBox::updateDescriptorSet(SkyBox &skyBox) {
    skyBox.descriptorSet = g_descriptorPool.allocate({ g_descriptorSetLayout}).front();

    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageView = skyBox.texture.imageView;
    imageInfo.sampler = skyBox.texture.sampler;
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    auto writes = initializers::writeDescriptorSets<1>();
    writes[0].dstSet = skyBox.descriptorSet;
    writes[0].dstBinding = 0;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[0].descriptorCount = 1;
    writes[0].pImageInfo = &imageInfo;

    device().updateDescriptorSets(writes);
}

void SkyBox::createDescriptorPool() {
    std::vector<VkDescriptorPoolSize> poolSizes(1);
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[0].descriptorCount = 1;
    g_descriptorPool = device().createDescriptorPool(10, poolSizes);
}

