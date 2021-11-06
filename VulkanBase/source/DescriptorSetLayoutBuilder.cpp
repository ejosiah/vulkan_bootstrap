
#include <VulkanDevice.h>

#include "DescriptorSetBuilder.hpp"

DescriptorSetLayoutBuilder VulkanDevice::descriptorSetLayoutBuilder() {
    return DescriptorSetLayoutBuilder{ *this };
}