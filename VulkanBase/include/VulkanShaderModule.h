#pragma once

#include "common.h"

struct VulkanShaderModule{

    VulkanShaderModule(const std::string& path, VkDevice device);

    VulkanShaderModule(const std::vector<uint32_t>& data, VkDevice device);

    ~VulkanShaderModule();

    VkShaderModule shaderModule = VK_NULL_HANDLE;

    inline operator VkShaderModule() const {
        return shaderModule;
    }
private:
    VkDevice device = VK_NULL_HANDLE;
};
