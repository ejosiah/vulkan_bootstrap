#include "DescriptorSetBuilder.hpp"

DescriptorSetLayoutBuilder VulkanDevice::descriptorSetLayoutBuilder() {
    return DescriptorSetLayoutBuilder{ *this };
}