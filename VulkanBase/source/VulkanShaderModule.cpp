#include "common.h"
#include "VulkanBaseApp.h"
#include "VulkanShaderModule.h"

VulkanShaderModule::VulkanShaderModule(const std::string& path, VkDevice device)
        :device(device) {
    auto data = loadFile(path);
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = data.size();
    createInfo.pCode = reinterpret_cast<uint32_t*>(data.data());

    REPORT_ERROR(vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule), "Failed to create shader module: " + path);
}

VulkanShaderModule::VulkanShaderModule(const std::vector<uint32_t>& data, VkDevice device) : device(device) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = data.size() * sizeof(uint32_t);
    createInfo.pCode = data.data();

    REPORT_ERROR(vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule), "Failed to create shader module");
}


VulkanShaderModule::~VulkanShaderModule() {
    vkDestroyShaderModule(device, shaderModule, nullptr);
}