#include "VertexInputStateBuilder.hpp"
#include "VulkanInitializers.h"

VertexInputStateBuilder::VertexInputStateBuilder(VulkanDevice *device, GraphicsPipelineBuilder *parent)
        : GraphicsPipelineBuilder(device, parent) {

}

VertexInputStateBuilder &
VertexInputStateBuilder::addVertexBindingDescription(uint32_t binding, uint32_t stride, VkVertexInputRate inputRate) {
    _bindings.push_back({binding, stride, inputRate});
    return *this;
}

VertexInputStateBuilder &
VertexInputStateBuilder::addVertexAttributeDescription(uint32_t location, uint32_t binding, VkFormat format,
                                                       uint32_t offset) {
    _attributes.push_back({location, binding, format, offset});
    return *this;
}

void VertexInputStateBuilder::validate() const {
    if(_bindings.empty()){
        throw std::runtime_error{ "No vertex binding descriptions defined for vertexInputState"};
    }
    if(_attributes.empty()){
        throw std::runtime_error{ "No vertex attributes descriptions defined for vertexInputState"};
    }

    for(const auto& binding : _bindings){
        auto itr = std::find_if(begin(_attributes), end(_attributes), [&binding](const auto& attribute){
            return binding.binding == attribute.binding;
        });
        if(itr == end(_attributes)){
            throw std::runtime_error{fmt::format("No vertex attribute description defined for binding {}", binding.binding)};
        }
    }
}

VkPipelineVertexInputStateCreateInfo VertexInputStateBuilder::buildVertexInputState() {
    validate();
    return initializers::vertexInputState(_bindings, _attributes);
}
