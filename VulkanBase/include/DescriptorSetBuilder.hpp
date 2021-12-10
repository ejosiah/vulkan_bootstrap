#pragma once

#include "VulkanDevice.h"

class DescriptorSetLayoutBuilder {
public:
    explicit DescriptorSetLayoutBuilder(const VulkanDevice& device) : device(device){}

    class DescriptorSetLayoutBindingBuilder{
    public:
        explicit DescriptorSetLayoutBindingBuilder(
                const VulkanDevice& device,
                std::vector<VkDescriptorSetLayoutBinding>& bindings, 
                uint32_t bindingValue
        )
        : device(device)
        , bindings(bindings)
        {
            _binding.binding = bindingValue;
        };


        DescriptorSetLayoutBindingBuilder binding(uint32_t value) const {
            assertBinding();
            bindings.push_back(_binding);
            return DescriptorSetLayoutBindingBuilder{ device, bindings, value};
        }

        const DescriptorSetLayoutBindingBuilder& descriptorCount(uint32_t count) const{
            _binding.descriptorCount = count;
            return *this;
        }

        const DescriptorSetLayoutBindingBuilder& descriptorType(VkDescriptorType type) const{
            _binding.descriptorType = type;
            return *this;
        }

        const DescriptorSetLayoutBindingBuilder& shaderStages(VkShaderStageFlags flags) const{
            _binding.stageFlags = flags;
            return *this;
        }

        const DescriptorSetLayoutBindingBuilder& immutableSamplers(const VkSampler* samplers) const{
            _binding.pImmutableSamplers = samplers;
            return *this;
        }
        
        [[nodiscard]] 
        VulkanDescriptorSetLayout createLayout(VkDescriptorSetLayoutCreateFlags flags = 0) const {
            assertBinding();
            bindings.push_back(_binding);
            return device.createDescriptorSetLayout(bindings, flags);
        }

        void assertBinding() const {
            assert(_binding.binding >= 0 && _binding.descriptorCount >= 1);
        }

    private:
        mutable VkDescriptorSetLayoutBinding _binding{};
        std::vector<VkDescriptorSetLayoutBinding>& bindings;
        const VulkanDevice& device;
    };

    DescriptorSetLayoutBindingBuilder binding(uint32_t value) const {
        return DescriptorSetLayoutBindingBuilder{ device, bindings, value};
    }

private:
    mutable std::vector<VkDescriptorSetLayoutBinding> bindings;
    const VulkanDevice& device;
};