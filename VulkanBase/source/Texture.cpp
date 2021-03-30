#include "Texture.h"
#include  <stb_image.h>

void textures::fromFile(const VulkanDevice &device, Texture &texture, std::string_view path) {
    int texWidth, texHeight, texChannels;
    stbi_set_flip_vertically_on_load(1);
    stbi_uc* pixels = stbi_load(path.data(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    if(!pixels){
        throw std::runtime_error{"failed to load texture image!"};
    }
    create(device, texture, VK_IMAGE_TYPE_2D, VK_FORMAT_R8G8B8A8_SRGB, pixels, {texWidth, texHeight, 1u});
    stbi_image_free(pixels);
}

void textures::create(const VulkanDevice &device, Texture &texture, VkImageType imageType, VkFormat format, void *data,
                      Dimension<uint32_t> dimensions, VkSamplerAddressMode addressMode) {
    VkDeviceSize imageSize = dimensions.x * dimensions.y * dimensions.z * nunChannels(format);

    VulkanBuffer stagingBuffer = device.createBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY, imageSize);
    stagingBuffer.copy(data, imageSize);

    VkImageCreateInfo imageCreateInfo{};
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    imageCreateInfo.extent = { static_cast<uint32_t>(dimensions.x), static_cast<uint32_t>(dimensions.y), dimensions.z};
    imageCreateInfo.mipLevels = 1;
    imageCreateInfo.arrayLayers = 1;
    imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCreateInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    auto& commandPool = device.commandPoolFor(*device.queueFamilyIndex.graphics);

    texture.image = device.createImage(imageCreateInfo, VMA_MEMORY_USAGE_GPU_ONLY);
    texture.image.transitionLayout(commandPool, device.queues.graphics, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    commandPool.oneTimeCommand(device.queues.graphics, [&](auto cmdBuffer) {
        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;

        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = {0, 0, 0};
        region.imageExtent = {static_cast<uint32_t>(dimensions.x), static_cast<uint32_t>(dimensions.y), dimensions.z};

        vkCmdCopyBufferToImage(cmdBuffer, stagingBuffer, texture.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                               &region);
    });

    texture.image.transitionLayout(commandPool, device.queues.graphics, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    VkImageSubresourceRange subresourceRange;
    subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceRange.baseMipLevel = 0;
    subresourceRange.levelCount = 1;
    subresourceRange.baseArrayLayer = 0;
    subresourceRange.layerCount = 1;

    texture.imageView = texture.image.createView(VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_VIEW_TYPE_2D, subresourceRange);

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = addressMode;
    samplerInfo.addressModeV = addressMode;
    samplerInfo.addressModeW = addressMode;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 16;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    texture.sampler = device.createSampler(samplerInfo);
}

void textures::checkerboard(const VulkanDevice &device, Texture &texture, const glm::vec3 &colorA, const glm::vec3 &colorB) {
    auto data = new unsigned char[256 * 256 * 4];
    for(int i = 0; i < 256; i++){
        for(int j = 0; j < 256; j++){
            auto color = (((i / 8) % 2) && ((j / 8) % 2)) || (!((i / 8) % 2) && !((j / 8) % 2)) ? colorB : colorA;
            auto idx = (i * 256 + j) * 4;
            data[idx + 0]  = static_cast<unsigned char>(color.r * 255);
            data[idx + 1]  = static_cast<unsigned char>(color.b * 255);
            data[idx + 2]  = static_cast<unsigned char>(color.g * 255);
            data[idx + 3] = 255;
        }
    }
    create(device, texture, VK_IMAGE_TYPE_2D, VK_FORMAT_R8G8B8A8_SRGB, data, {256, 256, 1}, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
}

uint32_t textures::nunChannels(VkFormat format) {
    switch (format) {
        case VK_FORMAT_R8_SRGB:
            return 1;
        case VK_FORMAT_R8G8_SRGB:
            return 2;
        case VK_FORMAT_R8G8B8_SRGB:
        case VK_FORMAT_B8G8R8_SRGB:
            return 3;
        case VK_FORMAT_R8G8B8A8_SRGB:
        case VK_FORMAT_B8G8R8A8_SRGB:
            return 4;
        default:
            return 0;
    }
}

