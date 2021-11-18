#include "Phong.h"

void phong::Material::init(const mesh::Mesh& mesh,  VulkanDevice& device, const VulkanDescriptorPool& descriptorPool
                           , const VulkanDescriptorSetLayout& descriptorSetLayout, VkBufferUsageFlags usageFlags){

    descriptorSet = descriptorPool.allocate({descriptorSetLayout}).front();
    device.setName<VK_OBJECT_TYPE_DESCRIPTOR_SET>(mesh.name, descriptorSet);
    std::vector<VkWriteDescriptorSet> writes;

    auto initTexture = [&](Texture& texture, VkDescriptorImageInfo& info, const std::string& path, uint32_t binding){
        if(!path.empty()){
            textures::fromFile(device, texture, path, true);
            info.imageView = texture.imageView;
            info.sampler = texture.sampler;
            info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            VkWriteDescriptorSet write = initializers::writeDescriptorSet();
            write.dstSet = descriptorSet;
            write.dstBinding = binding;
            write.descriptorCount = 1;
            write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            write.pImageInfo = &info;
            writes.push_back(write);
        }
    };

    textures.ambientMap = std::make_unique<Texture>();
    VkDescriptorImageInfo ambInfo{};
    initTexture(*textures.ambientMap, ambInfo, mesh.textureMaterial.ambientMap, 1);


    textures.diffuseMap = std::make_unique<Texture>();
    VkDescriptorImageInfo diffInfo{};
    initTexture(*textures.diffuseMap, diffInfo, mesh.textureMaterial.diffuseMap, 2);

    textures.specularMap = std::make_unique<Texture>();
    VkDescriptorImageInfo specInfo{};
    initTexture(*textures.specularMap, specInfo, mesh.textureMaterial.specularMap, 3);

    textures.normalMap = std::make_unique<Texture>();
    VkDescriptorImageInfo normInfo{};
    initTexture(*textures.normalMap, normInfo, mesh.textureMaterial.normalMap, 4);

    textures.ambientOcclusionMap = std::make_unique<Texture>();
    VkDescriptorImageInfo aoInfo{};
    initTexture(*textures.ambientOcclusionMap, aoInfo, mesh.textureMaterial.ambientOcclusionMap, 5);

    uint32_t size = sizeof(mesh.material) - offsetof(mesh::Material, diffuse);
    std::vector<char> materialData(size);
    std::memcpy(materialData.data(), &mesh.material.diffuse, size);

    materialBuffer = device.createDeviceLocalBuffer(materialData.data(), size, usageFlags | VK_BUFFER_USAGE_TRANSFER_SRC_BIT);

    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = materialBuffer;
    bufferInfo.offset = materialOffset;
    bufferInfo.range = VK_WHOLE_SIZE;
    VkWriteDescriptorSet write = initializers::writeDescriptorSet();
    write.dstSet = descriptorSet;
    write.dstBinding = 0;
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    write.pBufferInfo = &bufferInfo;
    writes.push_back(write);

    device.updateDescriptorSets(writes);

}



