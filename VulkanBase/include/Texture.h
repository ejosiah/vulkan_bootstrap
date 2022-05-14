#pragma once

#include "common.h"
#include "VulkanImage.h"
#include "VulkanDevice.h"



struct Texture{
    VulkanImage image;
    VulkanImageView imageView;
    VulkanSampler sampler;
    uint32_t width{0};
    uint32_t height{0};
    uint32_t depth{1};
};

template<typename T>
using Dimension3D = glm::vec<3, T, glm::defaultp>;

template<typename T>
using Dimension2D = glm::vec<2, T, glm::defaultp>;

namespace textures{

    void create(const VulkanDevice& device, Texture& texture, VkImageType imageType, VkFormat format, void* data
                , Dimension3D<uint32_t> dimensions, VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT
                , uint32_t sizeMultiplier = 1, VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL);

    void fromFile(const VulkanDevice& device, Texture& texture, std::string_view path, bool flipUv = false, VkFormat format = VK_FORMAT_R8G8B8A8_UNORM);

    void hdr(const VulkanDevice& device, Texture& texture, std::string_view path);

    void checkerboard(const VulkanDevice& device, Texture& texture, const glm::vec3& colorA = glm::vec3(1), const glm::vec3& colorB = glm::vec3(0));

    void normalMap(const VulkanDevice& device, Texture& texture, const Dimension2D<uint32_t>& dimensions);

    void color(const VulkanDevice& device, Texture& texture, const glm::vec3& color, const Dimension2D<uint32_t>& dimensions);

    void checkerboard(unsigned char* data, const Dimension2D<uint32_t>& dimensions, const glm::vec3& colorA = glm::vec3(1), const glm::vec3& colorB = glm::vec3(0));

    void color(unsigned char* data, const Dimension2D<uint32_t>& dimensions, const glm::vec3& color);

    void normalMap(unsigned char* data, const Dimension2D<uint32_t>& dimensions);

    Texture equirectangularToOctahedralMap(const VulkanDevice& device, const std::string& path, uint32_t size, VkImageLayout finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    Texture equirectangularToOctahedralMap(const VulkanDevice& device, const Texture& equirectangularTexture, uint32_t size, VkImageLayout finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    Texture brdf_lut(const VulkanDevice& device);

    void ibl(const VulkanDevice& device, const Texture& envMap, Texture& irradianceMap, Texture& specularMap, VkImageLayout finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}