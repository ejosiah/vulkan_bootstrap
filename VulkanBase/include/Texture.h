#pragma once

#include "common.h"
#include "VulkanImage.h"
#include "VulkanRAII.h"



struct Texture{
    VulkanImage image;
    VulkanImageView imageView;
    VulkanSampler sampler;
};