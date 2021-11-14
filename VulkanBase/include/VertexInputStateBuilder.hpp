#pragma once
#include "GraphicsPipelineBuilder.hpp"

class VertexInputStateBuilder : public GraphicsPipelineBuilder{
public:
    explicit VertexInputStateBuilder(VulkanDevice* device, GraphicsPipelineBuilder* parent);

    VertexInputStateBuilder& addVertexBindingDescription(uint32_t binding, uint32_t stride, VkVertexInputRate inputRate);

    template<typename BindingDescriptions = std::vector<VkVertexInputBindingDescription>>
    inline VertexInputStateBuilder& addVertexBindingDescriptions(const BindingDescriptions& bindings){
        for(const auto& binding : bindings){
            addVertexBindingDescription(binding.binding, binding.stride, binding.inputRate);
        }
        return *this;
    }

    VertexInputStateBuilder& addVertexAttributeDescription(uint32_t location, uint32_t binding, VkFormat format, uint32_t offset);

    template<typename AttributeDescriptions = std::vector<VkVertexInputAttributeDescription>>
    inline VertexInputStateBuilder& addVertexAttributeDescriptions(const AttributeDescriptions& attributes){
        for(const auto& attribute : attributes){
            addVertexAttributeDescription(attribute.location, attribute.binding, attribute.format, attribute.offset);
        }
        return *this;
    }

    void validate() const;

    void clear();

    VkPipelineVertexInputStateCreateInfo& buildVertexInputState();

private:
    std::vector<VkVertexInputBindingDescription> _bindings;
    std::vector<VkVertexInputAttributeDescription> _attributes;
    VkPipelineVertexInputStateCreateInfo _info;
};