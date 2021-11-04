
#include "MultisampleStateBuilder.hpp"

MultisampleStateBuilder::MultisampleStateBuilder(VulkanDevice *device, GraphicsPipelineBuilder *parent)
: GraphicsPipelineBuilder(device, parent)
, _info(initializers::multisampleState())
{

}

VkPipelineMultisampleStateCreateInfo &MultisampleStateBuilder::buildMultisampleState() {
    return _info;
}
