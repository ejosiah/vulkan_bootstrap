#pragma once

#include "VulkanDevice.h"

class GraphicsPipelineBuilder{
public:
    explicit GraphicsPipelineBuilder(VulkanDevice& device)
    :device{device}
    {
        vertexInputState.parent = this;
        inputAssemblyState.parent = this;
    }


    GraphicsPipelineBuilder& addShader(ShaderInfo&& shaderInfo){
        shaders.push_back(shaderInfo);
        return *this;
    }

    struct SubBuilder{
        GraphicsPipelineBuilder* parent = nullptr;

        [[nodiscard]]
        GraphicsPipelineBuilder& commit() const{
            return *parent;
        }
    };

    struct VertexInputStateBuilder : SubBuilder {
        std::vector<VkVertexInputBindingDescription> vertexBindingDescriptions;
        std::vector<VkVertexInputAttributeDescription> vertexAttributeDescriptions;

        VertexInputStateBuilder& addVertexBinding(VkVertexInputBindingDescription vertexBindingDesc){
            vertexBindingDescriptions.push_back(vertexBindingDesc);
            return *this;
        }

        VertexInputStateBuilder& addVertexAttribute(VkVertexInputAttributeDescription vertexAttrib){
            vertexAttributeDescriptions.push_back(vertexAttrib);
            return *this;
        }

    } vertexInputState;

    struct InputAssemblyStateBuilder : SubBuilder{
        VkPipelineInputAssemblyStateCreateInfo value = initializers::inputAssemblyState();

        InputAssemblyStateBuilder& topology(VkPrimitiveTopology topology){
            value.topology = topology;
            return *this;
        }

        InputAssemblyStateBuilder& enablePrimitiveRestart(){
            value.primitiveRestartEnable = true;
            return *this;
        }

        InputAssemblyStateBuilder& disablePrimitiveRestart(){
            value.primitiveRestartEnable = false;
            return *this;
        }

    } inputAssemblyState;

    struct ViewportStateBuilder : SubBuilder{
        std::vector<VkViewport> viewports;
        std::vector<VkRect2D> scissors;

        ViewportStateBuilder& addViewport(VkViewport viewport){
            viewports.push_back(viewport);
            return *this;
        }

        ViewportStateBuilder& addScissor(VkRect2D scissor){
            scissors.push_back(scissor);
            return *this;
        }

    } viewportState;

private:
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
    std::vector<ShaderInfo> shaders;
    VulkanDevice& device;
};

GraphicsPipelineBuilder VulkanDevice::graphicsPipelineBuilder(){
    return GraphicsPipelineBuilder{ *this };
}