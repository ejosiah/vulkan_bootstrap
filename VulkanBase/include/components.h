#pragma once
#include "common.h"
#include <entt/entt.hpp>
#include "camera_base.h"
#include "VulkanModel.h"

namespace component{
    struct Name{
        std::string value;
    };

    struct Parent{
        entt::entity entity{ entt::null };
    };

    struct Transform{
        Transform* parent{nullptr};
        glm::vec3 translation{ glm::vec3(0) };
        glm::vec3 scale{ glm::vec3(1) };
        glm::quat rotation{ 1, 0, 0, 0};
    };

    struct Camera{
        ::Camera* camera{ nullptr };
    };

    struct Render{
        VkPipeline pipeline;
        VkPipelineLayout layout;
        std::vector<byte_string> ranges;
        std::vector<VkDescriptorSet> descriptorSets;
        std::vector<vkn::Primitive> primitives;

        template<typename T>
        T& range(int index){
            assert(index < ranges.size());
            return *reinterpret_cast<T*>(ranges[index].data());
        }
    };
}

template<>
struct entt::component_traits<component::Transform> : entt::basic_component_traits{
    using in_place_delete = std::true_type;
};