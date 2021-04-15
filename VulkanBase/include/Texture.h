#pragma once

#include "common.h"
#include "VulkanImage.h"
#include "VulkanDevice.h"



struct Texture{
    VulkanImage image;
    VulkanImageView imageView;
    VulkanSampler sampler;
};

template<typename T>
using Dimension3D = glm::vec<3, T, glm::defaultp>;

template<typename T>
using Dimension2D = glm::vec<2, T, glm::defaultp>;

namespace textures{


    void create(const VulkanDevice& device, Texture& texture, VkImageType imageType, VkFormat format, void* data
                , Dimension3D<uint32_t> dimensions, VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT
                , uint32_t sizeMultiplier = 1);

    void fromFile(const VulkanDevice& device, Texture& texture, std::string_view path, bool flipUv = false, VkFormat format = VK_FORMAT_R8G8B8A8_SRGB);

    void checkerboard(const VulkanDevice& device, Texture& texture, const glm::vec3& colorA = glm::vec3(1), const glm::vec3& colorB = glm::vec3(0));

    void normalMap(const VulkanDevice& device, Texture& texture, const Dimension2D<uint32_t>& dimensions);

    void color(const VulkanDevice& device, Texture& texture, const glm::vec3& color, const Dimension2D<uint32_t>& dimensions);
}