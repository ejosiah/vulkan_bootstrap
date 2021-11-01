#pragma once

#include "common.h"

struct VulkanShaderModule{

    DISABLE_COPY(VulkanShaderModule);

    VulkanShaderModule() = default;

    VulkanShaderModule(const std::string& path, VkDevice device);

    VulkanShaderModule(const std::vector<uint32_t>& data, VkDevice device);

    VulkanShaderModule(VulkanShaderModule&& source) noexcept;

    VulkanShaderModule& operator=(VulkanShaderModule&& source) noexcept;

    ~VulkanShaderModule();

    VkShaderModule shaderModule = VK_NULL_HANDLE;

    inline operator VkShaderModule() const {
        return shaderModule;
    }

    inline operator bool() const{
        return shaderModule != VK_NULL_HANDLE;
    }

private:
    VkDevice device = VK_NULL_HANDLE;
};
