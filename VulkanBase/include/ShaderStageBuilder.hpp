#pragma once

#include "GraphicsPipelineBuilder.hpp"
#include "VulkanShaderModule.h"

class ShaderStageBuilder : public GraphicsPipelineBuilder{
public:
   explicit ShaderStageBuilder(VulkanDevice* device, GraphicsPipelineBuilder* parent);

   [[nodiscard]]
   ShaderStageBuilder& addVertexShader(const std::string& path);

   [[nodiscard]]
   ShaderStageBuilder& addFragmentShader(const std::string& path);

   [[nodiscard]]
   ShaderStageBuilder& addGeometryShader(const std::string& path);

   [[nodiscard]]
   ShaderStageBuilder& addTessellationEvaluationShader(const std::string& path);

   [[nodiscard]]
   ShaderStageBuilder& addTessellationControlShader(const std::string& path);

   void clear();

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