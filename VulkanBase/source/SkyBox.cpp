#include "common.h"
#include "SkyBox.hpp"
#include "primitives.h"
#include "VulkanDevice.h"
#include "GraphicsPipelineBuilder.hpp"
#include  <stb_image.h>
#include "DescriptorSetBuilder.hpp"

static constexpr uint32_t __glsl_skybox_vert_spv[] = {
        0x07230203,0x00010000,0x000D000A,0x00000049,0x00000000,0x00020011,0x00000001,0x0006000B,
        0x00000001,0x4C534C47,0x6474732E,0x3035342E,0x00000000,0x0003000E,0x00000000,0x00000001,
        0x0008000F,0x00000000,0x00000004,0x6E69616D,0x00000000,0x00000009,0x0000000B,0x0000003A,
        0x00030003,0x00000002,0x000001CC,0x000A0004,0x475F4C47,0x4C474F4F,0x70635F45,0x74735F70,
        0x5F656C79,0x656E696C,0x7269645F,0x69746365,0x00006576,0x00080004,0x475F4C47,0x4C474F4F,
        0x6E695F45,0x64756C63,0x69645F65,0x74636572,0x00657669,0x00040005,0x00000004,0x6E69616D,
        0x00000000,0x00040005,0x00000009,0x43786574,0x0064726F,0x00050005,0x0000000B,0x69736F70,
        0x6E6F6974,0x00000000,0x00040005,0x00000010,0x6569566D,0x00000077,0x00030005,0x00000011,
        0x0050564D,0x00050006,0x00000011,0x00000000,0x65646F6D,0x0000006C,0x00050006,0x00000011,
        0x00000001,0x77656976,0x00000000,0x00060006,0x00000011,0x00000002,0x6A6F7270,0x69746365,
        0x00006E6F,0x00030005,0x00000013,0x00000000,0x00060005,0x00000038,0x505F6C67,0x65567265,
        0x78657472,0x00000000,0x00060006,0x00000038,0x00000000,0x505F6C67,0x7469736F,0x006E6F69,
        0x00070006,0x00000038,0x00000001,0x505F6C67,0x746E696F,0x657A6953,0x00000000,0x00070006,
        0x00000038,0x00000002,0x435F6C67,0x4470696C,0x61747369,0x0065636E,0x00070006,0x00000038,
        0x00000003,0x435F6C67,0x446C6C75,0x61747369,0x0065636E,0x00030005,0x0000003A,0x00000000,
        0x00040047,0x00000009,0x0000001E,0x00000000,0x00040047,0x0000000B,0x0000001E,0x00000000,
        0x00040048,0x00000011,0x00000000,0x00000005,0x00050048,0x00000011,0x00000000,0x00000023,
        0x00000000,0x00050048,0x00000011,0x00000000,0x00000007,0x00000010,0x00040048,0x00000011,
        0x00000001,0x00000005,0x00050048,0x00000011,0x00000001,0x00000023,0x00000040,0x00050048,
        0x00000011,0x00000001,0x00000007,0x00000010,0x00040048,0x00000011,0x00000002,0x00000005,
        0x00050048,0x00000011,0x00000002,0x00000023,0x00000080,0x00050048,0x00000011,0x00000002,
        0x00000007,0x00000010,0x00030047,0x00000011,0x00000002,0x00050048,0x00000038,0x00000000,
        0x0000000B,0x00000000,0x00050048,0x00000038,0x00000001,0x0000000B,0x00000001,0x00050048,
        0x00000038,0x00000002,0x0000000B,0x00000003,0x00050048,0x00000038,0x00000003,0x0000000B,
        0x00000004,0x00030047,0x00000038,0x00000002,0x00020013,0x00000002,0x00030021,0x00000003,
        0x00000002,0x00030016,0x00000006,0x00000020,0x00040017,0x00000007,0x00000006,0x00000003,
        0x00040020,0x00000008,0x00000003,0x00000007,0x0004003B,0x00000008,0x00000009,0x00000003,
        0x00040020,0x0000000A,0x00000001,0x00000007,0x0004003B,0x0000000A,0x0000000B,0x00000001,
        0x00040017,0x0000000D,0x00000006,0x00000004,0x00040018,0x0000000E,0x0000000D,0x00000004,
        0x00040020,0x0000000F,0x00000007,0x0000000E,0x0005001E,0x00000011,0x0000000E,0x0000000E,
        0x0000000E,0x00040020,0x00000012,0x00000009,0x00000011,0x0004003B,0x00000012,0x00000013,
        0x00000009,0x00040015,0x00000014,0x00000020,0x00000001,0x0004002B,0x00000014,0x00000015,
        0x00000001,0x00040020,0x00000016,0x00000009,0x0000000E,0x0004002B,0x00000014,0x00000019,
        0x00000000,0x00040018,0x0000001D,0x00000007,0x00000003,0x0004002B,0x00000006,0x00000025,
        0x3F800000,0x0004002B,0x00000006,0x00000026,0x00000000,0x00040015,0x00000035,0x00000020,
        0x00000000,0x0004002B,0x00000035,0x00000036,0x00000001,0x0004001C,0x00000037,0x00000006,
        0x00000036,0x0006001E,0x00000038,0x0000000D,0x00000006,0x00000037,0x00000037,0x00040020,
        0x00000039,0x00000003,0x00000038,0x0004003B,0x00000039,0x0000003A,0x00000003,0x0004002B,
        0x00000014,0x0000003B,0x00000002,0x00040020,0x00000047,0x00000003,0x0000000D,0x00050036,
        0x00000002,0x00000004,0x00000000,0x00000003,0x000200F8,0x00000005,0x0004003B,0x0000000F,
        0x00000010,0x00000007,0x0004003D,0x00000007,0x0000000C,0x0000000B,0x0003003E,0x00000009,
        0x0000000C,0x00050041,0x00000016,0x00000017,0x00000013,0x00000015,0x0004003D,0x0000000E,
        0x00000018,0x00000017,0x00050041,0x00000016,0x0000001A,0x00000013,0x00000019,0x0004003D,
        0x0000000E,0x0000001B,0x0000001A,0x00050092,0x0000000E,0x0000001C,0x00000018,0x0000001B,
        0x00050051,0x0000000D,0x0000001E,0x0000001C,0x00000000,0x0008004F,0x00000007,0x0000001F,
        0x0000001E,0x0000001E,0x00000000,0x00000001,0x00000002,0x00050051,0x0000000D,0x00000020,
        0x0000001C,0x00000001,0x0008004F,0x00000007,0x00000021,0x00000020,0x00000020,0x00000000,
        0x00000001,0x00000002,0x00050051,0x0000000D,0x00000022,0x0000001C,0x00000002,0x0008004F,
        0x00000007,0x00000023,0x00000022,0x00000022,0x00000000,0x00000001,0x00000002,0x00060050,
        0x0000001D,0x00000024,0x0000001F,0x00000021,0x00000023,0x00060051,0x00000006,0x00000027,
        0x00000024,0x00000000,0x00000000,0x00060051,0x00000006,0x00000028,0x00000024,0x00000000,
        0x00000001,0x00060051,0x00000006,0x00000029,0x00000024,0x00000000,0x00000002,0x00060051,
        0x00000006,0x0000002A,0x00000024,0x00000001,0x00000000,0x00060051,0x00000006,0x0000002B,
        0x00000024,0x00000001,0x00000001,0x00060051,0x00000006,0x0000002C,0x00000024,0x00000001,
        0x00000002,0x00060051,0x00000006,0x0000002D,0x00000024,0x00000002,0x00000000,0x00060051,
        0x00000006,0x0000002E,0x00000024,0x00000002,0x00000001,0x00060051,0x00000006,0x0000002F,
        0x00000024,0x00000002,0x00000002,0x00070050,0x0000000D,0x00000030,0x00000027,0x00000028,
        0x00000029,0x00000026,0x00070050,0x0000000D,0x00000031,0x0000002A,0x0000002B,0x0000002C,
        0x00000026,0x00070050,0x0000000D,0x00000032,0x0000002D,0x0000002E,0x0000002F,0x00000026,
        0x00070050,0x0000000D,0x00000033,0x00000026,0x00000026,0x00000026,0x00000025,0x00070050,
        0x0000000E,0x00000034,0x00000030,0x00000031,0x00000032,0x00000033,0x0003003E,0x00000010,
        0x00000034,0x00050041,0x00000016,0x0000003C,0x00000013,0x0000003B,0x0004003D,0x0000000E,
        0x0000003D,0x0000003C,0x0004003D,0x0000000E,0x0000003E,0x00000010,0x00050092,0x0000000E,
        0x0000003F,0x0000003D,0x0000003E,0x0004003D,0x00000007,0x00000040,0x0000000B,0x00050051,
        0x00000006,0x00000041,0x00000040,0x00000000,0x00050051,0x00000006,0x00000042,0x00000040,
        0x00000001,0x00050051,0x00000006,0x00000043,0x00000040,0x00000002,0x00070050,0x0000000D,
        0x00000044,0x00000041,0x00000042,0x00000043,0x00000025,0x00050091,0x0000000D,0x00000045,
        0x0000003F,0x00000044,0x0009004F,0x0000000D,0x00000046,0x00000045,0x00000045,0x00000000,
        0x00000001,0x00000003,0x00000003,0x00050041,0x00000047,0x00000048,0x0000003A,0x00000019,
        0x0003003E,0x00000048,0x00000046,0x000100FD,0x00010038
};

static constexpr uint32_t __glsl_skybox_frag_spv[]{
        0x07230203,0x00010000,0x000D000A,0x00000014,0x00000000,0x00020011,0x00000001,0x0006000B,
        0x00000001,0x4C534C47,0x6474732E,0x3035342E,0x00000000,0x0003000E,0x00000000,0x00000001,
        0x0007000F,0x00000004,0x00000004,0x6E69616D,0x00000000,0x00000009,0x00000011,0x00030010,
        0x00000004,0x00000007,0x00030003,0x00000002,0x000001CC,0x000A0004,0x475F4C47,0x4C474F4F,
        0x70635F45,0x74735F70,0x5F656C79,0x656E696C,0x7269645F,0x69746365,0x00006576,0x00080004,
        0x475F4C47,0x4C474F4F,0x6E695F45,0x64756C63,0x69645F65,0x74636572,0x00657669,0x00040005,
        0x00000004,0x6E69616D,0x00000000,0x00050005,0x00000009,0x67617266,0x6F6C6F43,0x00000072,
        0x00040005,0x0000000D,0x62796B73,0x0000786F,0x00050005,0x00000011,0x43786574,0x64726F6F,
        0x00000000,0x00040047,0x00000009,0x0000001E,0x00000000,0x00040047,0x0000000D,0x00000022,
        0x00000000,0x00040047,0x0000000D,0x00000021,0x00000000,0x00040047,0x00000011,0x0000001E,
        0x00000000,0x00020013,0x00000002,0x00030021,0x00000003,0x00000002,0x00030016,0x00000006,
        0x00000020,0x00040017,0x00000007,0x00000006,0x00000004,0x00040020,0x00000008,0x00000003,
        0x00000007,0x0004003B,0x00000008,0x00000009,0x00000003,0x00090019,0x0000000A,0x00000006,
        0x00000003,0x00000000,0x00000000,0x00000000,0x00000001,0x00000000,0x0003001B,0x0000000B,
        0x0000000A,0x00040020,0x0000000C,0x00000000,0x0000000B,0x0004003B,0x0000000C,0x0000000D,
        0x00000000,0x00040017,0x0000000F,0x00000006,0x00000003,0x00040020,0x00000010,0x00000001,
        0x0000000F,0x0004003B,0x00000010,0x00000011,0x00000001,0x00050036,0x00000002,0x00000004,
        0x00000000,0x00000003,0x000200F8,0x00000005,0x0004003D,0x0000000B,0x0000000E,0x0000000D,
        0x0004003D,0x0000000F,0x00000012,0x00000011,0x00050057,0x00000007,0x00000013,0x0000000E,
        0x00000012,0x0003003E,0x00000009,0x00000013,0x000100FD,0x00010038
};

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
    std::vector<uint32_t> vertex{std::begin(__glsl_skybox_vert_spv), std::end(__glsl_skybox_vert_spv)};
    std::vector<uint32_t> fragment(std::begin(__glsl_skybox_frag_spv), std::end(__glsl_skybox_frag_spv));
    g_pipeline =
        device().graphicsPipelineBuilder()
            .shaderStage()
                .vertexShader(vertex)
                .fragmentShader(fragment)
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

