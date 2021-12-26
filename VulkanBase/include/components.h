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

    struct Instance{
        uint32_t id;
        entt::entity base_entity;
    };


    struct Position{
        glm::vec3 value{0};
    };

    struct Scale{
        glm::vec3 value{1};
    };

    struct Rotation{
        glm::quat value{1, 0, 0, 0};
    };

    struct Transform{
        glm::mat4 value{1};
        Transform* parent{nullptr};
    };

    struct Camera{
        ::Camera* camera{ nullptr };
        bool main{ true };
    };

    struct Pipeline{
        VkPipeline pipeline{ VK_NULL_HANDLE };
        VkPipelineLayout layout{ VK_NULL_HANDLE };
        uint32_t subpass{ 0 };
        std::vector<byte_string> ranges;
        std::vector<VkDescriptorSet> descriptorSets;
    };

    struct Pipelines{
        std::vector<Pipeline> pipelines;

        void add(const Pipeline& pipeline){
            pipelines.push_back(pipeline);
        }

        std::vector<Pipeline>::iterator begin() {
            return pipelines.begin();
        }

        std::vector<Pipeline>::iterator end()  {
            return pipelines.end();
        }

        [[nodiscard]]
        std::vector<Pipeline>::const_iterator begin() const {
            return pipelines.cbegin();
        }

        [[nodiscard]]
        std::vector<Pipeline>::const_iterator end() const {
            return pipelines.cend();
        }

        void clear() {
            pipelines.clear();
        }
    };

    struct Render{
        std::vector<VulkanBuffer> vertexBuffers;
        VulkanBuffer indexBuffer;
        std::vector<vkn::Primitive> primitives;
        uint32_t indexCount{0};
        uint32_t instanceCount{1};
    };
}

template<>
struct entt::component_traits<component::Transform> : entt::basic_component_traits{
    using in_place_delete = std::true_type;
};