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
using Dimension = glm::vec<3, T, glm::defaultp>;

namespace textures{


    void create(const VulkanDevice& device, Texture& texture, VkImageType imageType, VkFormat format, void* data, Dimension<uint32_t> dimensions, VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT);

    void fromFile(const VulkanDevice& device, Texture& texture, std::string_view path);

    void checkerboard(const VulkanDevice& device, Texture& texture, const glm::vec3& colorA = glm::vec3(1), const glm::vec3& colorB = glm::vec3(0));

    uint32_t nunChannels(VkFormat format);
}