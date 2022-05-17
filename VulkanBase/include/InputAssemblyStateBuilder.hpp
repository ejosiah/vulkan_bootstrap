#pragma once

#include "GraphicsPipelineBuilder.hpp"

class InputAssemblyStateBuilder : public GraphicsPipelineBuilder{
public:
    InputAssemblyStateBuilder(VulkanDevice* device, GraphicsPipelineBuilder* parent);

    InputAssemblyStateBuilder& enablePrimitiveRestart();

    InputAssemblyStateBuilder& disablePrimitiveRestart();

    InputAssemblyStateBuilder& points();

    InputAssemblyStateBuilder& lines();

    InputAssemblyStateBuilder& lineStrip();

    InputAssemblyStateBuilder& triangles();

    InputAssemblyStateBuilder& triangleStrip();

    InputAssemblyStateBuilder& patches();

    InputAssemblyStateBuilder& linesWithAdjacency();

    InputAssemblyStateBuilder& lineStripWithAdjacency();

    InputAssemblyStateBuilder& trianglesWithAdjacency();

    InputAssemblyStateBuilder& triangleStripWithAdjacency();

    VkPipelineInputAssemblyStateCreateInfo& buildInputAssemblyState();

private:
    VkPrimitiveTopology  _topology{ VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST};
    VkBool32  _primitiveRestartEnable{ VK_FALSE };
    VkPipelineInputAssemblyStateCreateInfo _info;

};