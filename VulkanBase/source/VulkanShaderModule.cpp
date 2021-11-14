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

VulkanShaderModule::VulkanShaderModule(const byte_string& data, VkDevice device) : device(device) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = data.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(data.data());

    REPORT_ERROR(vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule), "Failed to create shader module");
}



VulkanShaderModule::~VulkanShaderModule() {
    if(shaderModule) {
        vkDestroyShaderModule(device, shaderModule, nullptr);
    }
}

VulkanShaderModule::VulkanShaderModule(VulkanShaderModule &&source) noexcept {
    operator=(static_cast<VulkanShaderModule&&>(source));
}

VulkanShaderModule &VulkanShaderModule::operator=(VulkanShaderModule &&source) noexcept {
    this->~VulkanShaderModule();
    this->shaderModule = source.shaderModule;
    this->device = source.device;

    source.shaderModule = VK_NULL_HANDLE;
    source.device = VK_NULL_HANDLE;
    return *this;
}
