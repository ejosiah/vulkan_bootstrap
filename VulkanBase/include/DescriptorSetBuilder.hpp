#pragma once

class DescriptorSetLayoutBuilder {
public:
    explicit DescriptorSetLayoutBuilder(const VulkanDevice& device) : device(device){}

    class DescriptorSetLayoutBindingBuilder{
    public:
        explicit DescriptorSetLayoutBindingBuilder(
                const VulkanDevice& device,
                std::vector<VkDescriptorSetLayoutBinding>& bindings,
                std::string name,
                uint32_t bindingValue
        )
        : device(device)
        , bindings(bindings)
        , _name{name}
        {
            _binding.binding = bindingValue;
        };


        DescriptorSetLayoutBindingBuilder binding(uint32_t value) const {
            assertBinding();
            bindings.push_back(_binding);
            return DescriptorSetLayoutBindingBuilder{ device, bindings, _name, value};
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
            auto setLayout = std::move(device.createDescriptorSetLayout(bindings, flags));
            if(!_name.empty()){
                device.setName<VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT>(_name, setLayout.handle);
            }
            return setLayout;
        }

        void assertBinding() const {
            assert(_binding.binding >= 0 && _binding.descriptorCount >= 1);
        }

    private:
        mutable VkDescriptorSetLayoutBinding _binding{};
        std::vector<VkDescriptorSetLayoutBinding>& bindings;
        mutable std::string _name;
        const VulkanDevice& device;
    };

    DescriptorSetLayoutBuilder& name(const std::string& name) {
        _name = name;
        return *this;
    }

    DescriptorSetLayoutBindingBuilder binding(uint32_t value) const {
        return DescriptorSetLayoutBindingBuilder{ device, bindings, _name, value};
    }

private:
    mutable std::vector<VkDescriptorSetLayoutBinding> bindings;
    mutable std::string _name;
    const VulkanDevice& device;
};