#pragma once

#include "GraphicsPipelineBuilder.hpp"

class MultisampleStateBuilder : public GraphicsPipelineBuilder{
public:
    MultisampleStateBuilder(VulkanDevice* device, GraphicsPipelineBuilder* parent);

    MultisampleStateBuilder& rasterizationSamples(VkSampleCountFlagBits flags);

    MultisampleStateBuilder& enableSampleShading();

    MultisampleStateBuilder& disableSampleShading();

    MultisampleStateBuilder& minSampleShading(float value);

    MultisampleStateBuilder& sampleMask(const VkSampleMask* mask);

    MultisampleStateBuilder& enableAlphaToCoverage();

    MultisampleStateBuilder& disableAlphaToCoverage();

    MultisampleStateBuilder& enableAlphaToOne();

    VkPipelineMultisampleStateCreateInfo& buildMultisampleState();

private:
    VkPipelineMultisampleStateCreateInfo _info;
};