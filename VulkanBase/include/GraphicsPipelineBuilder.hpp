#pragma once

#include "common.h"
#include "Builder.hpp"
#include "VulkanDevice.h"
#include "VulkanRAII.h"

class GraphicsPipelineBuilder : public Builder {
public:
    explicit GraphicsPipelineBuilder(VulkanDevice* device);

    explicit GraphicsPipelineBuilder(VulkanDevice* device, GraphicsPipelineBuilder* parent);

    GraphicsPipelineBuilder() = default;

    virtual ~GraphicsPipelineBuilder();

    virtual ShaderStageBuilder& shaderStage();

    virtual VertexInputStateBuilder& vertexInputState();

//    template<typename BuilderType>
//    BuilderType& allowDerivatives() {
//        if(!parent()){
//            _flags |= VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT;
//        }else{
//            parent()->_flags |= VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT;
//        }
//        return dynamic_cast<BuilderType&>(*this);
//    }
//
//    template<typename BuilderType>
//    BuilderType& setDerivatives() {
//        if(!parent()){
//            _flags |= VK_PIPELINE_CREATE_DERIVATIVE_BIT;
//        }else{
//            parent()->_flags |= VK_PIPELINE_CREATE_DERIVATIVE_BIT;
//        }
//        return dynamic_cast<BuilderType&>(*this);
//    }


    template<typename BuilderType>
    BuilderType& subpass(uint32_t value) {
        if(!parent()){
            _subpass = value;
        }else{
            parent()->_subpass = value;
        }
        return dynamic_cast<BuilderType&>(*this);
    }

    template<typename BuilderType>
    BuilderType& layout(VkPipelineLayout  aLayout) {
        if(!parent()){
            _pipelineLayout = aLayout;
        }else{
            parent()->_pipelineLayout = aLayout;
        }
        return dynamic_cast<BuilderType&>(*this);
    }

    template<typename BuilderType>
    BuilderType& renderPass(VkRenderPass  aRenderPass) {
        if(!parent()){
            _renderPass = aRenderPass;
        }else{
            parent()->_renderPass = aRenderPass;
        }
        return dynamic_cast<BuilderType&>(*this);
    }

    [[nodiscard]]
    GraphicsPipelineBuilder *parent() override;

    VulkanPipeline build();

    VkGraphicsPipelineCreateInfo createInfo();

protected:
    VkPipelineCache _pipelineCache = VK_NULL_HANDLE;
    VkPipelineCreateFlags _flags = 0;
    VkRenderPass _renderPass = VK_NULL_HANDLE;
    VkPipelineLayout _pipelineLayout = VK_NULL_HANDLE;
    uint32_t _subpass = 0;

private:
    ShaderStageBuilder* _shaderStageBuilder = nullptr;
    VertexInputStateBuilder* _vertexInputStateBuilder = nullptr;
};

#include "ShaderStageBuilder.hpp"
#include "VertexInputStateBuilder.hpp"