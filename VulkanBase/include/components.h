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

    struct Transform{
        glm::vec3 translation{ glm::vec3(0) };
        glm::quat rotation{1, 0, 0, 0};
        glm::vec3 scale{ glm::vec3(1) };
        Transform* parent{nullptr};

        [[nodiscard]]
        glm::mat4 get() const {
            glm::mat4 rotate = glm::mat4(rotation);
            auto localTransform = glm::translate(glm::mat4(1), translation) * rotate * glm::scale(glm::mat4(1), scale);
            if(parent){
                return parent->get() * localTransform;
            }
            return localTransform;
        }
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

    struct Render{
        std::vector<Pipeline> pipelines;
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