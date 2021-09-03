#pragma once

#include "VulkanDevice.h"

class DescriptorSetLayoutBuilder {
public:
    explicit DescriptorSetLayoutBuilder(VulkanDevice& device) : device(device){}

    class DescriptorSetLayoutBindingBuilder{
    public:
        explicit DescriptorSetLayoutBindingBuilder(
                VulkanDevice& device, 
                std::vector<VkDescriptorSetLayoutBinding>& bindings, 
                uint32_t bindingValue
        )
        : device(device)
        , bindings(bindings)
        {
            _binding.binding = bindingValue;
        };


        DescriptorSetLayoutBindingBuilder binding(uint32_t value){
            assertBinding();
            bindings.push_back(_binding);
            return DescriptorSetLayoutBindingBuilder{ device, bindings, value};
        }

        DescriptorSetLayoutBindingBuilder& descriptorCount(uint32_t count){
            _binding.descriptorCount = count;
            return *this;
        }

        DescriptorSetLayoutBindingBuilder& descriptorType(VkDescriptorType type){
            _binding.descriptorType = type;
            return *this;
        }

        DescriptorSetLayoutBindingBuilder& shaderStages(VkShaderStageFlags flags){
            _binding.stageFlags = flags;
            return *this;
        }

        DescriptorSetLayoutBindingBuilder& immutableSamplers(const VkSampler* samplers){
            _binding.pImmutableSamplers = samplers;
            return *this;
        }
        
        [[nodiscard]] 
        VulkanDescriptorSetLayout createLayout(VkDescriptorSetLayoutCreateFlags flags = 0) const{
            assertBinding();
            bindings.push_back(_binding);
            return device.createDescriptorSetLayout(bindings, flags);
        }

        void assertBinding() const {
            assert(_binding.binding >= 0 && _binding.descriptorCount >= 1);
        }

    private:
        VkDescriptorSetLayoutBinding _binding{};
        std::vector<VkDescriptorSetLayoutBinding>& bindings;
        VulkanDevice& device;
    };

    DescriptorSetLayoutBindingBuilder binding(uint32_t value){
        return DescriptorSetLayoutBindingBuilder{ device, bindings, value};
    }

private:
    std::vector<VkDescriptorSetLayoutBinding> bindings;
    VulkanDevice& device;
};