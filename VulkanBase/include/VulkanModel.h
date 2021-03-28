#pragma once

#include "common.h"
#include "VulkanDevice.h"
#include "Texture.h"

struct PhongMaterial{
    PhongMaterial(
        const VulkanDevice& device,
        const VulkanDescriptorPool pool,
        const VkDescriptorSetLayout& setLayout,
        std::unique_ptr<Texture> diffuseMap,
        std::unique_ptr<Texture> specularMap,
        std::unique_ptr<Texture> normalMap,
        std::unique_ptr<Texture> ambientMap,
        std::unique_ptr<Texture> emission,
        std::unique_ptr<Texture> shine
        )
    : diffuseMap(std::move(diffuseMap))
    , specularMap(std::move(specularMap))
    , normalMap(std::move(normalMap))
    , ambientMap(std::move(ambientMap))
    , emission(std::move(emission))
    , shine(std::move(shine))
    {
        descriptorSet = pool.allocate({ setLayout }).front();
        // TODO write descriptorSet

        std::array<VkDescriptorImageInfo, 6> imageInfos{};
        imageInfos[0].imageView = diffuseMap->imageView;
        imageInfos[0].sampler = diffuseMap->sampler;
        imageInfos[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        imageInfos[1].imageView = specularMap->imageView;
        imageInfos[1].sampler = specularMap->sampler;
        imageInfos[1].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        imageInfos[2].imageView = normalMap->imageView;
        imageInfos[2].sampler = normalMap->sampler;
        imageInfos[2].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        imageInfos[3].imageView = ambientMap->imageView;
        imageInfos[3].sampler = ambientMap->sampler;
        imageInfos[3].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        imageInfos[4].imageView = emission->imageView;
        imageInfos[4].sampler = emission->sampler;
        imageInfos[4].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        imageInfos[5].imageView = shine->imageView;
        imageInfos[5].sampler = shine->sampler;
        imageInfos[5].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        std::vector<VkWriteDescriptorSet> writes(6, initializers::writeDescriptorSet(descriptorSet));

        for(auto i = 0; i < imageInfos.size(); i++) {
            writes[0].dstBinding = i;
            writes[0].dstArrayElement = 0;
            writes[0].descriptorCount = 1;
            writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            writes[0].pImageInfo = &imageInfos[i];
        }

        vkUpdateDescriptorSets(device, COUNT(writes), writes.data(), 0, VK_NULL_HANDLE);

    }



    std::unique_ptr<Texture> diffuseMap;
    std::unique_ptr<Texture> specularMap;
    std::unique_ptr<Texture> normalMap;
    std::unique_ptr<Texture> ambientMap;
    std::unique_ptr<Texture> emission;
    std::unique_ptr<Texture> shine;

    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
};

struct PBRMaterial;

template<typename Material>
struct Primitive{
    Primitive(uint32_t firstIndex, uint32_t vertexCount, uint32_t firstVertex = 0, uint32_t indexCount = 0,  Material* material = nullptr)
            :firstIndex(firstIndex)
            , indexCount(indexCount)
            , firstVertex(0)
            , vertexCount(0)
            , material(material)
    {};



    struct {
        glm::vec3 min;
        glm::vec3 max;
    } bounds;

    void setBounds(glm::vec3 min, glm::vec3 max){
        bounds.min = min;
        bounds.max = max;
    }

    uint32_t firstIndex;
    uint32_t indexCount;
    uint32_t firstVertex;
    uint32_t vertexCount;
    Material material;
};
