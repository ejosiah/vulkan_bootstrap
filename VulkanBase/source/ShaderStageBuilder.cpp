#include "ShaderStageBuilder.hpp"
#include "VulkanInitializers.h"

ShaderStageBuilder::ShaderStageBuilder(VulkanDevice *device, GraphicsPipelineBuilder *parent)
: GraphicsPipelineBuilder(device, parent)
{

}

ShaderStageBuilder &ShaderStageBuilder::addVertexShader(const std::string &path) {
    _vertexModule = VulkanShaderModule{ path, device()};
    return *this;
}

ShaderStageBuilder &ShaderStageBuilder::addFragmentShader(const std::string &path) {
    _fragmentModule = VulkanShaderModule{ path, device()};
    return *this;
}

ShaderStageBuilder &ShaderStageBuilder::addGeometryShader(const std::string &path) {
    _geometryModule = VulkanShaderModule{ path, device()};
    return *this;
}

ShaderStageBuilder &ShaderStageBuilder::addTessellationEvaluationShader(const std::string &path) {
    _tessEvalModule = VulkanShaderModule{ path, device()};
    return *this;
}

ShaderStageBuilder &ShaderStageBuilder::addTessellationControlShader(const std::string &path) {
    _tessControlModule = VulkanShaderModule{ path, device() };
    return *this;;
}

void ShaderStageBuilder::validate() const {
    if(!_vertexModule){
        throw std::runtime_error{"at least vertex shader should be provided"};
    }
    if(_tessControlModule && !_tessEvalModule){
        throw std::runtime_error{"tessellation eval shader required if tessellation control shader provided"};
    }
}

std::vector<VkPipelineShaderStageCreateInfo>& ShaderStageBuilder::buildShaderStage()  {
    validate();

    _stages.push_back({_vertexModule, VK_SHADER_STAGE_VERTEX_BIT});

    if(_fragmentModule){
        _stages.push_back({ _fragmentModule, VK_SHADER_STAGE_FRAGMENT_BIT});
    }
    if(_geometryModule){
        _stages.push_back({ _geometryModule, VK_SHADER_STAGE_GEOMETRY_BIT });
    }

    if(_tessEvalModule){
        _stages.push_back({ _tessEvalModule, VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT});
    }

    if(_tessControlModule){
        _stages.push_back({ _tessControlModule, VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT});
    }

    _vkStages = initializers::vertexShaderStages(_stages);

    return _vkStages;
}
