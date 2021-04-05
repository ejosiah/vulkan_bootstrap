#include "Phong.h"

phong::Material::Material(
        const VulkanDevice &device,
        const VulkanDescriptorPool& pool,
        std::unique_ptr<Texture> diffuseMap,
        std::unique_ptr<Texture> specularMap,
        std::unique_ptr<Texture> normalMap,
        std::unique_ptr<Texture> ambientMap,
        std::unique_ptr<Texture> emission,
        std::unique_ptr<Texture> shine)
        : diffuseMap(std::move(diffuseMap))
        , specularMap(std::move(specularMap))
        , normalMap(std::move(normalMap))
        , ambientMap(std::move(ambientMap))
        , emission(std::move(emission))
        , shine(std::move(shine))
{
    // TODO create setLayout

    descriptorSet = pool.allocate({setLayout}).front();

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

    for (auto i = 0; i < imageInfos.size(); i++) {
        writes[0].dstBinding = i;
        writes[0].dstArrayElement = 0;
        writes[0].descriptorCount = 1;
        writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writes[0].pImageInfo = &imageInfos[i];
    }

    vkUpdateDescriptorSets(device, COUNT(writes), writes.data(), 0, VK_NULL_HANDLE);
}

phong::Mesh::Mesh():vkn::Primitive() {

}

phong::Mesh::Mesh(const VulkanDevice &device, const VulkanDescriptorPool &pool, const VulkanBuffer &vertexBuffer,
                  const VulkanBuffer &indexBuffer, const mesh::Mesh &mesh, const glm::mat4 &model)
                  :vkn::Primitive()
{

}

