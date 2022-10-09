#include "Texture.h"
#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include  <stb_image.h>
#endif
#include <fmt/format.h>
#include <glm/glm.hpp>

uint32_t nunChannels(VkFormat format) {
    switch (format) {
        case VK_FORMAT_R8_SRGB:
        case VK_FORMAT_R32_SFLOAT:
            return 1;
        case VK_FORMAT_R8G8_SRGB:
        case VK_FORMAT_R8G8_UNORM:
        case VK_FORMAT_R32G32_SFLOAT:
            return 2;
        case VK_FORMAT_R8G8B8_SRGB:
        case VK_FORMAT_B8G8R8_SRGB:
        case VK_FORMAT_R8G8B8_UNORM:
        case VK_FORMAT_R16G16B16_SFLOAT:
        case VK_FORMAT_R32G32B32_SFLOAT:
            return 3;
        case VK_FORMAT_R8G8B8A8_SRGB:
        case VK_FORMAT_B8G8R8A8_SRGB:
        case VK_FORMAT_R8G8B8A8_UNORM:
        case VK_FORMAT_R16G16B16A16_SFLOAT:
        case VK_FORMAT_R32G32B32A32_SFLOAT:
            return 4;
        default:
            throw std::runtime_error{fmt::format("format: {}, not implemented", format)};
    }
}

constexpr VkFormat getFormat(uint32_t numChannels){
    switch(numChannels){
        case 1:
            return VK_FORMAT_R8_SRGB;
        case 2:
            return VK_FORMAT_R8G8_SRGB;
        case 3:
            return VK_FORMAT_R8G8B8_SRGB;
        case 4:
            return VK_FORMAT_R8G8B8A8_SRGB;
        default:
            return VK_FORMAT_UNDEFINED;
    }
}


RawImage textures::loadImage(std::string_view path, bool flipUv) {
    int texWidth, texHeight,  texChannels;
    stbi_set_flip_vertically_on_load(flipUv ? 1 : 0);
    stbi_uc* pixels = stbi_load(path.data(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    return RawImage{
        std::unique_ptr<stbi_uc, stbi_image_deleter>(pixels),
                static_cast<uint32_t>(texWidth),
                static_cast<uint32_t>(texHeight),
                texChannels
    };
}

void textures::fromFile(const VulkanDevice &device, Texture &texture, std::string_view path, bool flipUv, VkFormat format) {
    int texWidth, texHeight, texChannels;
    stbi_set_flip_vertically_on_load(flipUv ? 1 : 0);
    stbi_uc* pixels = stbi_load(path.data(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    texture.width = texWidth;
    texture.height = texHeight;
    if(!pixels){
        throw std::runtime_error{fmt::format("failed to load texture image {}!", path)};
    }
    create(device, texture, VK_IMAGE_TYPE_2D, format, pixels, {texWidth, texHeight, 1u});
    stbi_image_free(pixels);
}

Texture textures::equirectangularToOctahedralMap(const VulkanDevice& device, const std::string& path, uint32_t size, VkImageLayout finalLayout){
    Texture equiTexture;
    equiTexture.width = equiTexture.height = size;
    textures::hdr(device, equiTexture, path);
    auto octahedralMap = equirectangularToOctahedralMap(device, equiTexture, size, finalLayout);
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

    octahedralMap.sampler = device.createSampler(samplerInfo);

    return octahedralMap;
}


void textures::hdr(const VulkanDevice &device, Texture &texture, std::string_view path) {
    int texWidth, texHeight, texChannels;
    float* data = stbi_loadf(path.data(), &texWidth, &texHeight, &texChannels, 0);
    texture.width = texWidth;
    texture.height = texHeight;
    if(!data){
        throw std::runtime_error{"failed to load texture image!"};
    }
    int desiredChannels = 4;
    std::vector<float> pixels;
    int size = texWidth * texHeight * texChannels;
    for(auto i = 0; i < size; i+= texChannels){
        float r = data[i];
        float g = data[i + 1];
        float b = data[i + 2];
        pixels.push_back(r);
        pixels.push_back(g);
        pixels.push_back(b);
        pixels.push_back(1);
    }
    stbi_image_free(data);

    create(device, texture, VK_IMAGE_TYPE_2D, VK_FORMAT_R32G32B32A32_SFLOAT, reinterpret_cast<char*>(pixels.data())
           , {texWidth, texHeight, 1u}, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, sizeof(float));
}

void textures::create(const VulkanDevice &device, Texture &texture, VkImageType imageType, VkFormat format, void *data,
                      Dimension3D<uint32_t> dimensions, VkSamplerAddressMode addressMode, uint32_t sizeMultiplier, VkImageTiling tiling) {
    VkDeviceSize imageSize = dimensions.x * dimensions.y * dimensions.z * nunChannels(format) * sizeMultiplier;

    VulkanBuffer stagingBuffer = device.createBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY, imageSize);
    stagingBuffer.copy(data, imageSize);

     VkImageCreateInfo imageCreateInfo{};
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.imageType = imageType;
    imageCreateInfo.format = format;
    imageCreateInfo.extent = { static_cast<uint32_t>(dimensions.x), static_cast<uint32_t>(dimensions.y), dimensions.z};
    imageCreateInfo.mipLevels = 1;
    imageCreateInfo.arrayLayers = 1;
    imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCreateInfo.tiling = tiling;
    imageCreateInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    auto& commandPool = device.commandPoolFor(*device.queueFamilyIndex.graphics);

    texture.image = device.createImage(imageCreateInfo, VMA_MEMORY_USAGE_GPU_ONLY);
    texture.image.size = imageSize;
    texture.width = dimensions.x;
    texture.height = dimensions.y;
    texture.depth = dimensions.z;
    texture.image.transitionLayout(commandPool, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    commandPool.oneTimeCommand([&](auto cmdBuffer) {
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

    texture.image.transitionLayout(commandPool, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    VkImageSubresourceRange subresourceRange;
    subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceRange.baseMipLevel = 0;
    subresourceRange.levelCount = 1;
    subresourceRange.baseArrayLayer = 0;
    subresourceRange.layerCount = 1;

    auto imageViewType = VK_IMAGE_VIEW_TYPE_2D;
    imageViewType = imageType == VK_IMAGE_TYPE_3D ? VK_IMAGE_VIEW_TYPE_3D : imageViewType;
    texture.imageView = texture.image.createView(format, imageViewType, subresourceRange);  // FIXME derive image view type

    if(!texture.sampler.handle) {
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = addressMode;
        samplerInfo.addressModeV = addressMode;
        samplerInfo.addressModeW = addressMode;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

        texture.sampler = device.createSampler(samplerInfo);
    }
}

void textures::create(const VulkanDevice &device, Texture &texture, VkImageType imageType, VkFormat format,
                      Dimension3D<uint32_t> dimensions, VkSamplerAddressMode addressMode, uint32_t sizeMultiplier,
                      VkImageTiling tiling) {

    VkDeviceSize imageSize = dimensions.x * dimensions.y * dimensions.z * nunChannels(format) * sizeMultiplier;

    VkImageCreateInfo imageCreateInfo{};
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.imageType = imageType;
    imageCreateInfo.format = format;
    imageCreateInfo.extent = { static_cast<uint32_t>(dimensions.x), static_cast<uint32_t>(dimensions.y), dimensions.z};
    imageCreateInfo.mipLevels = 1;
    imageCreateInfo.arrayLayers = 1;
    imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCreateInfo.tiling = tiling;
    imageCreateInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    auto& commandPool = device.commandPoolFor(*device.queueFamilyIndex.graphics);

    texture.image = device.createImage(imageCreateInfo, VMA_MEMORY_USAGE_GPU_ONLY);
    texture.image.size = imageSize;
    texture.width = dimensions.x;
    texture.height = dimensions.y;
    texture.depth = dimensions.z;
    texture.image.transitionLayout(commandPool, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    texture.image.transitionLayout(commandPool, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    VkImageSubresourceRange subresourceRange;
    subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceRange.baseMipLevel = 0;
    subresourceRange.levelCount = 1;
    subresourceRange.baseArrayLayer = 0;
    subresourceRange.layerCount = 1;

    auto imageViewType = VK_IMAGE_VIEW_TYPE_2D;
    imageViewType = imageType == VK_IMAGE_TYPE_3D ? VK_IMAGE_VIEW_TYPE_3D : imageViewType;
    texture.imageView = texture.image.createView(format, imageViewType, subresourceRange);  // FIXME derive image view type

    if(!texture.sampler.handle) {
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = addressMode;
        samplerInfo.addressModeV = addressMode;
        samplerInfo.addressModeW = addressMode;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

        texture.sampler = device.createSampler(samplerInfo);
    }

}

void textures::allocate(const VulkanDevice& device, Texture &texture, VkImageType imageType, VkFormat format, VkDeviceSize size,
                        Dimension3D<uint32_t> dimensions, VkImageTiling tiling) {

    VkImageCreateInfo imageCreateInfo{};
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.imageType = imageType;
    imageCreateInfo.format = format;
    imageCreateInfo.extent = { static_cast<uint32_t>(dimensions.x), static_cast<uint32_t>(dimensions.y), dimensions.z};
    imageCreateInfo.mipLevels = 1;
    imageCreateInfo.arrayLayers = 1;
    imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCreateInfo.tiling = tiling;
    imageCreateInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    auto& commandPool = device.commandPoolFor(*device.queueFamilyIndex.graphics);

    texture.image = device.createImage(imageCreateInfo, VMA_MEMORY_USAGE_GPU_ONLY);
    texture.image.size = size;
    texture.width = dimensions.x;
    texture.height = dimensions.y;
    texture.depth = dimensions.z;
    texture.image.transitionLayout(commandPool, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    VkImageSubresourceRange subresourceRange;
    subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceRange.baseMipLevel = 0;
    subresourceRange.levelCount = 1;
    subresourceRange.baseArrayLayer = 0;
    subresourceRange.layerCount = 1;

    auto imageViewType = VK_IMAGE_VIEW_TYPE_2D;
    imageViewType = imageType == VK_IMAGE_TYPE_3D ? VK_IMAGE_VIEW_TYPE_3D : imageViewType;
    texture.imageView = texture.image.createView(format, imageViewType, subresourceRange);

}


void textures::checkerboard(const VulkanDevice &device, Texture &texture, const glm::vec3 &colorA, const glm::vec3 &colorB) {
    texture.width = texture.height = 256;
    auto data = new unsigned char[256 * 256 * 4];
    checkerboard(data, {256, 256 });
    create(device, texture, VK_IMAGE_TYPE_2D, VK_FORMAT_R8G8B8A8_UNORM, data, {256, 256, 1}, VK_SAMPLER_ADDRESS_MODE_REPEAT);
    delete[] data;
}

void textures::normalMap(const VulkanDevice &device, Texture &texture, const Dimension2D<uint32_t>& dimensions) {
    color(device, texture, glm::vec3{0.5, 0.5, 1}, dimensions);
}



void textures::color(const VulkanDevice &device, Texture &texture, const glm::vec3 &color, const Dimension2D<uint32_t>& dimensions) {
    auto data = new unsigned char[dimensions.x * dimensions.y * 4];
    textures::color(data, dimensions, color);
    create(device, texture, VK_IMAGE_TYPE_2D, VK_FORMAT_R8G8B8A8_UNORM, data, Dimension3D<uint32_t>{dimensions, 1u}, VK_SAMPLER_ADDRESS_MODE_REPEAT);
    delete[] data;
}


void textures::checkerboard(unsigned char* data, const Dimension2D<uint32_t>& dimensions, const glm::vec3& colorA, const glm::vec3& colorB){
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
}

void textures::checkerboard1(unsigned char* data, const Dimension2D<uint32_t>& dimensions, const glm::vec3& colorA, const glm::vec3& colorB, float repeat){
    for(int i = 0; i < dimensions.y; i++){
        for(int j = 0; j < dimensions.x; j++){
            float x = glm::floor(static_cast<float>(j)/static_cast<float>(dimensions.x) * repeat);
            float y = glm::floor(static_cast<float>(i)/static_cast<float>(dimensions.y) * repeat);

            float t = glm::step(glm::mod(x + y, 2.f), 0.f);
            auto color = glm::mix(colorA, colorB, t);
            auto idx = (i * dimensions.x + j) * 4;
            data[idx + 0]  = static_cast<unsigned char>(color.r * 255);
            data[idx + 1]  = static_cast<unsigned char>(color.b * 255);
            data[idx + 2]  = static_cast<unsigned char>(color.g * 255);
            data[idx + 3] = 255;
        }
    }
}

void textures::color(unsigned char* data, const Dimension2D<uint32_t>& dimensions, const glm::vec3& color){
    for(auto i = 0; i < dimensions.y; i++){
        for(auto j = 0; j < dimensions.x; j++){
            auto idx = (i * dimensions.x + j) * 4;
            data[idx + 0]  = static_cast<unsigned char>(color.r * 255);
            data[idx + 1]  = static_cast<unsigned char>(color.g * 255);
            data[idx + 2]  = static_cast<unsigned char>(color.b * 255);
            data[idx + 3] = 255;
        }
    }
}

void textures::normalMap(unsigned char* data, const Dimension2D<uint32_t>& dimensions){
    color(data, dimensions, glm::vec3{0.5, 0.5, 1});
}

void textures::uv(const VulkanDevice &device, Texture &texture, const Dimension2D<uint32_t> &dimensions) {
    auto data = uvData(dimensions);
    create(device, texture, VK_IMAGE_TYPE_2D, VK_FORMAT_R8G8_UNORM, data.data(), Dimension3D<uint32_t>{dimensions, 1u}, VK_SAMPLER_ADDRESS_MODE_REPEAT);
}

std::vector<unsigned char> textures::uvData(const Dimension2D<uint32_t> &dimensions) {
    std::vector<unsigned char> data(dimensions.x * dimensions.y * 2);
    glm::vec2 size{dimensions};
    for(int i = 0; i < dimensions.y; i++){
        for(int j = 0; j < dimensions.x; j++){
            glm::vec2 coord{i, j};
            auto uv = coord/size;
            auto id = (i * dimensions.x + j) * 2;
            data[id + 0] = static_cast<unsigned char>(uv.x * 255);
            data[id + 1] = static_cast<unsigned char>(uv.y * 255);
        }
    }
    return data;
}

void textures::transfer(VkCommandBuffer commandBuffer, const VulkanBuffer& srcBuffer, VulkanImage& dstImage,
                        Dimension2D<uint32_t> dimension2D, VkImageLayout sourceLayout) {

    VkImageSubresourceRange subresourceRange;
    subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceRange.baseMipLevel = 0;
    subresourceRange.levelCount = 1;
    subresourceRange.baseArrayLayer = 0;
    subresourceRange.layerCount = 1;

    dstImage.transitionLayout(commandBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, subresourceRange
                              , VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_TRANSFER_WRITE_BIT
                              , VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {static_cast<uint32_t>(dimension2D.x), static_cast<uint32_t>(dimension2D.y), 1};

    vkCmdCopyBufferToImage(commandBuffer, srcBuffer, dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
                           , 1,&region);

    dstImage.transitionLayout(commandBuffer, sourceLayout, subresourceRange, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
}

void textures::copy(VkCommandBuffer commandBuffer, Texture &srcTexture, Texture &dstTexture) {

    VkImageSubresourceRange subresourceRange;
    subresourceRange.aspectMask = srcTexture.aspectMask;
    subresourceRange.baseMipLevel = 0;
    subresourceRange.levelCount = 1;
    subresourceRange.baseArrayLayer = 0;
    subresourceRange.layerCount = 1;

    auto dstOldLayout = dstTexture.image.currentLayout;
    if(dstOldLayout != VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL){
        dstTexture.image.transitionLayout(commandBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, subresourceRange
            , 0, VK_ACCESS_TRANSFER_WRITE_BIT
            , VK_PIPELINE_STAGE_NONE, VK_PIPELINE_STAGE_TRANSFER_BIT);
    }

    auto srcOldLayout = srcTexture.image.currentLayout;
    if(srcOldLayout != VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL){
        srcTexture.image.transitionLayout(commandBuffer, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, subresourceRange
                , 0, VK_ACCESS_TRANSFER_READ_BIT
                , VK_PIPELINE_STAGE_NONE, VK_PIPELINE_STAGE_TRANSFER_BIT);
    }

    VkImageSubresourceLayers imageSubresource{srcTexture.aspectMask, 0, 0, 1};

    VkImageCopy region{};
    region.srcSubresource = imageSubresource;
    region.srcOffset = {0, 0, 0};
    region.dstSubresource = imageSubresource;
    region.dstOffset = {0, 0, 0};
    region.extent = {srcTexture.width, srcTexture.height, 1};

    vkCmdCopyImage(commandBuffer, srcTexture.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dstTexture.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    if(dstOldLayout != VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL){
        dstTexture.image.transitionLayout(commandBuffer, dstOldLayout, subresourceRange
                ,  VK_ACCESS_TRANSFER_WRITE_BIT, 0
                ,  VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_NONE);
    }

    if(srcOldLayout != VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL){
        srcTexture.image.transitionLayout(commandBuffer, srcOldLayout, subresourceRange
                ,  VK_ACCESS_TRANSFER_WRITE_BIT, 0
                ,  VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_NONE);
    }
}

void textures::generate(const VulkanDevice &device, Texture &texture, uint32_t width, uint32_t height,
                       ColorGen&& generator, VkFormat format) {
    std::vector<unsigned char> canvas(width * height * 4);

    for(auto i = 0; i < height; i++){
        for(auto j = 0; j < width; j++){
            auto idx = (i * width + j) * 4;
            auto color = generator(j, i, width, height);
            canvas[idx + 0]  = static_cast<unsigned char>(color.r * 255);
            canvas[idx + 1]  = static_cast<unsigned char>(color.g * 255);
            canvas[idx + 2]  = static_cast<unsigned char>(color.b * 255);
            canvas[idx + 3] = 255;
        }
    }
    create(device, texture, VK_IMAGE_TYPE_2D, format, canvas.data(), {width, height, 1u});
}