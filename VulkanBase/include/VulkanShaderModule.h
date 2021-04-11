#pragma once

#include "common.h"

struct ShaderModule{

    ShaderModule(const std::string& path, VkDevice device);

    ShaderModule(const std::vector<uint32_t>& data, VkDevice device);

    ~ShaderModule();

    VkShaderModule shaderModule = VK_NULL_HANDLE;

    inline operator VkShaderModule() const {
        return shaderModule;
    }
private:
    VkDevice device = VK_NULL_HANDLE;
};
