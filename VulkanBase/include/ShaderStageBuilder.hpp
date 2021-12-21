#pragma once

#include "common.h"
#include "GraphicsPipelineBuilder.hpp"
#include "VulkanShaderModule.h"

class ShaderStageBuilder : public GraphicsPipelineBuilder{
public:
   explicit ShaderStageBuilder(VulkanDevice* device, GraphicsPipelineBuilder* parent);

   [[nodiscard]]
   ShaderStageBuilder& vertexShader(const std::string& path);

   [[nodiscard]]
   ShaderStageBuilder& vertexShader(const byte_string& data);

   [[nodiscard]]
   ShaderStageBuilder& vertexShader(const std::vector<uint32_t>& data);

   [[nodiscard]]
   ShaderStageBuilder& fragmentShader(const std::string& path);

   [[nodiscard]]
   ShaderStageBuilder& fragmentShader(const byte_string& path);

   [[nodiscard]]
   ShaderStageBuilder& geometryShader(const std::string& path);

   [[nodiscard]]
   ShaderStageBuilder& tessellationEvaluationShader(const std::string& path);

   [[nodiscard]]
   ShaderStageBuilder& tessellationControlShader(const std::string& path);

   ShaderStageBuilder& clear();

   void validate() const;

   [[nodiscard]]
   std::vector<VkPipelineShaderStageCreateInfo>& buildShaderStage();

private:
    VulkanShaderModule _vertexModule;
    VulkanShaderModule _fragmentModule;
    VulkanShaderModule _geometryModule;
    VulkanShaderModule _tessEvalModule;
    VulkanShaderModule _tessControlModule;
    std::vector<ShaderInfo> _stages;
    std::vector<VkPipelineShaderStageCreateInfo> _vkStages;
};