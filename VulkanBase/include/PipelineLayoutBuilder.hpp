#pragma once

#include "GraphicsPipelineBuilder.hpp"

class PipelineLayoutBuilder : public GraphicsPipelineBuilder{
public:
    PipelineLayoutBuilder(VulkanDevice* device, GraphicsPipelineBuilder* builder);

    PipelineLayoutBuilder& addDescriptorSetLayout(VkDescriptorSetLayout layout);

    template<typename DescriptorSetLayouts = std::vector<VkDescriptorSetLayout>>
    PipelineLayoutBuilder& addDescriptorSetLayouts(const DescriptorSetLayouts& layouts){
        for(auto& layout : layouts){
            addDescriptorSetLayout(layout);
        }
        return *this;
    }

    PipelineLayoutBuilder& addPushConstantRange(VkShaderStageFlags stage, uint32_t offset, uint32_t size);

    PipelineLayoutBuilder& addPushConstantRange(VkPushConstantRange range);

    template<typename Ranges = std::vector<VkPushConstantRange>>
    PipelineLayoutBuilder& addPushConstantRanges(const Ranges& ranges){
        for(auto& range : ranges){
            addPushConstantRange(range.stageFlags, range.offset, range.size);
        }
        return *this;
    }

    PipelineLayoutBuilder& clear();

    PipelineLayoutBuilder& clearRanges();

    PipelineLayoutBuilder& clearLayouts();

    [[nodiscard]]
    VulkanPipelineLayout buildPipelineLayout() const;


private:
    std::vector<VkDescriptorSetLayout> _descriptorSetLayouts;
    std::vector<VkPushConstantRange> _ranges;
};