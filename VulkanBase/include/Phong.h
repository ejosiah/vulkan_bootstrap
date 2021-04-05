#pragma once
#include "common.h"
#include "VulkanDevice.h"
#include "VulkanModel.h"
#include "Texture.h"
#include "Mesh.h"

namespace phong{

    enum class LightType : int{
        POINT = 0,
        DIRECTIONAL = 1,
        SPOT = 2
    };

    struct LightSource{
        glm::vec3 position;
        glm::vec3 intensity;
        glm::vec3 spotDirection;
        float spotAngle;
        float spotExponent;
        float kc = 1;
        float ki = 0;
        float kq = 0;

        static constexpr VkPushConstantRange pushConstant(){
            return {VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(LightSource)};
        }
    };

    struct Material {

        Material() = default;

        Material(
            const VulkanDevice &device,
            const VulkanDescriptorPool& pool,
            std::unique_ptr<Texture> diffuseMap,
            std::unique_ptr<Texture> specularMap,
            std::unique_ptr<Texture> normalMap,
            std::unique_ptr<Texture> ambientMap,
            std::unique_ptr<Texture> emission,
            std::unique_ptr<Texture> shine
        );


        std::unique_ptr<Texture> diffuseMap;
        std::unique_ptr<Texture> specularMap;
        std::unique_ptr<Texture> normalMap;
        std::unique_ptr<Texture> ambientMap;
        std::unique_ptr<Texture> emission;
        std::unique_ptr<Texture> shine;
        VulkanDescriptorSetLayout setLayout;

        VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
    };

    struct Mesh : public vkn::Primitive{
        std::string name;
        Material material;

        Mesh();

        Mesh(const VulkanDevice &device,
             const VulkanDescriptorPool& pool,
             const VulkanBuffer& vertexBuffer,
             const VulkanBuffer& indexBuffer,
             const mesh::Mesh& mesh,
             const glm::mat4& model = {});

    };

    std::vector<Mesh> load(std::string_view path, const VulkanDevice &device, const VulkanDescriptorPool& pool);

}