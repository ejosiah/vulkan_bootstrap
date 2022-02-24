#include "VulkanDevice.h"
#include "VulkanRAII.h"
#include "Texture.h"
#include "Vertex.h"
#include "VulkanInitializers.h"

namespace textures {

    struct Pipeline {
        VulkanPipeline pipeline;
        VulkanPipelineLayout layout;
    };

    Texture createOctahedralMap(const VulkanDevice& device, uint32_t size);

    VulkanDescriptorPool createDescriptorPool(const VulkanDevice &device);

    VulkanRenderPass createRenderPass(const VulkanDevice &device, const Texture &texture, uint32_t size);

    VulkanFramebuffer createFrameBuffer(const VulkanDevice &device, const VulkanRenderPass& renderPass, const Texture &texture,  uint32_t size);

    VulkanDescriptorSetLayout createSetLayout(const VulkanDevice &device);

    VulkanBuffer createScreenQuad(const VulkanDevice& device);

    void updateDescriptorSet(const VulkanDevice& device, const Texture& texture, VkDescriptorSet descriptorSet);

    Pipeline createPipeline(const VulkanDevice& device, const VulkanRenderPass &renderPass, const VulkanDescriptorSetLayout &setLayout,
                            const Texture &texture, VkExtent2D extent);


    Texture equirectangularToOctahedralMap(const VulkanDevice &device, const Texture &equirectangularTexture, uint32_t size) {

        VkExtent2D extent = {size, size};
        Texture octahedralTexture = createOctahedralMap(device, size);
        VulkanRenderPass renderPass = createRenderPass(device, octahedralTexture, size);
        VulkanFramebuffer framebuffer = createFrameBuffer(device, renderPass, octahedralTexture, size);

        VulkanDescriptorPool descriptorPool = createDescriptorPool(device);
        VulkanDescriptorSetLayout setLayout = createSetLayout(device);
        VkDescriptorSet descriptorSet = descriptorPool.allocate({ setLayout }).front();
        updateDescriptorSet(device, equirectangularTexture, descriptorSet);

        VulkanBuffer screenQuad = createScreenQuad(device);
        Pipeline pipeline = createPipeline(device, renderPass, setLayout, equirectangularTexture, extent);

        device.graphicsCommandPool().oneTimeCommand([&](auto commandBuffer){
            VkCommandBufferBeginInfo beginInfo = initializers::commandBufferBeginInfo();
            vkBeginCommandBuffer(commandBuffer, &beginInfo);

            static std::array<VkClearValue, 2> clearValues;
            clearValues[0].color = {0, 0, 1, 1};
            clearValues[1].depthStencil = {1.0, 0u};

            VkRenderPassBeginInfo rPassInfo = initializers::renderPassBeginInfo();
            rPassInfo.clearValueCount = COUNT(clearValues);
            rPassInfo.pClearValues = clearValues.data();
            rPassInfo.framebuffer = framebuffer;
            rPassInfo.renderArea.offset = {0u, 0u};
            rPassInfo.renderArea.extent = extent;
            rPassInfo.renderPass = renderPass;

            vkCmdBeginRenderPass(commandBuffer, &rPassInfo, VK_SUBPASS_CONTENTS_INLINE);

            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline);
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.layout, 0, 1, &descriptorSet, 0, VK_NULL_HANDLE);
            VkDeviceSize offset = 0;
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, screenQuad, &offset);
            vkCmdDraw(commandBuffer, ClipSpace::Quad::positions.size(), 1, 0, 0);

            vkCmdEndRenderPass(commandBuffer);

            vkEndCommandBuffer(commandBuffer);
        });

        return octahedralTexture;
    }

    Texture createOctahedralMap(const VulkanDevice &device, uint32_t size) {

        Texture texture;
        auto format = VK_FORMAT_R32G32B32A32_SFLOAT;
        VkImageCreateInfo imageInfo = initializers::imageCreateInfo(VK_IMAGE_TYPE_2D, format
                                                                    , VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
                                                                    , size, size);
        texture.image = device.createImage(imageInfo, VMA_MEMORY_USAGE_GPU_ONLY);

        auto subResource = initializers::imageSubresourceRange();
        texture.imageView = texture.image.createView(format, VK_IMAGE_VIEW_TYPE_2D, subResource);
        return texture;
    }

    VulkanDescriptorPool createDescriptorPool(const VulkanDevice &device) {
        constexpr uint32_t maxSets = 1;
        std::array<VkDescriptorPoolSize, 16> poolSizes{
                {
                        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1},
                }
        };
        return device.createDescriptorPool(maxSets, poolSizes, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT);
    }

    VulkanRenderPass createRenderPass(const VulkanDevice &device, const Texture &texture, uint32_t size) {

        std::vector<VkAttachmentDescription> attachments{
                {
                    0,
                    VK_FORMAT_R32G32B32A32_SFLOAT,
                    VK_SAMPLE_COUNT_1_BIT,
                    VK_ATTACHMENT_LOAD_OP_CLEAR,
                    VK_ATTACHMENT_STORE_OP_STORE,
                    VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                    VK_ATTACHMENT_STORE_OP_DONT_CARE,
                    VK_IMAGE_LAYOUT_UNDEFINED,
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                }
        };

        std::vector<SubpassDescription> subpasses(1);

        subpasses[0].colorAttachments.push_back({0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});

        std::vector<VkSubpassDependency> dependencies(2);

        dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[0].dstSubpass = 0;
        dependencies[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[0].dstSubpass = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dependencies[0].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
        dependencies[0].dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        dependencies[1].srcSubpass = 0;
        dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[1].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dependencies[1].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[1].srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        dependencies[1].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
        dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        return device.createRenderPass(attachments, subpasses, dependencies);
    }

    VulkanFramebuffer createFrameBuffer(const VulkanDevice &device, const VulkanRenderPass& renderPass, const Texture &texture, uint32_t size) {
        std::vector<VkImageView> attachments{ texture.imageView };
        return device.createFramebuffer(renderPass, attachments, size, size);
    }

    VulkanDescriptorSetLayout createSetLayout(const VulkanDevice &device) {
        return
            device.descriptorSetLayoutBuilder()
                .binding(0)
                    .descriptorType(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
                    .descriptorCount(1)
                    .shaderStages(VK_SHADER_STAGE_FRAGMENT_BIT)
            .createLayout();
    }

    VulkanBuffer createScreenQuad(const VulkanDevice &device) {
        auto quad = ClipSpace::Quad::positions;
        std::vector<glm::vec2> positions;
        for(int i = 0; i < quad.size(); i += 2){
            positions.push_back(quad[i]);
        }
        return device.createDeviceLocalBuffer(positions.data(), BYTE_SIZE(positions), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    }

    void updateDescriptorSet(const VulkanDevice& device, const Texture &texture, VkDescriptorSet descriptorSet) {
        auto writes = initializers::writeDescriptorSets<1>();
        auto& write = writes[0];

        write.dstSet = descriptorSet;
        write.dstBinding = 0;
        write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write.descriptorCount = 1;
        VkDescriptorImageInfo imageInfo{texture.sampler, texture.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
        write.pImageInfo = &imageInfo;

        device.updateDescriptorSets(writes);
    }

    Pipeline createPipeline(const VulkanDevice &device, const VulkanRenderPass &renderPass,
                            const VulkanDescriptorSetLayout &setLayout, const Texture &texture, VkExtent2D extent) {
        return Pipeline();
    }
}