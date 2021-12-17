#pragma once
#include "common.h"
#include <entt/entt.hpp>
#include "camera_base.h"
#include "VulkanModel.h"

namespace component{
    struct Tag{
        std::string value;
    };

    struct Parent{
        entt::entity entity{ entt::null };
    };

    struct Transform{
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