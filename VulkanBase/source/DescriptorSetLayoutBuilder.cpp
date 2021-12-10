
#include <VulkanDevice.h>

#include "DescriptorSetBuilder.hpp"

DescriptorSetLayoutBuilder VulkanDevice::descriptorSetLayoutBuilder() const {
    return DescriptorSetLayoutBuilder{ *this };
}