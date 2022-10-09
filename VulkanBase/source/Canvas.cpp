#include <Vertex.h>
#include "Canvas.hpp"

Canvas::Canvas(const VulkanBaseApp* application, VkImageUsageFlags usage, VkFormat fmt)
    : app{ application }
    , usageFlags{ usage| VK_IMAGE_USAGE_SAMPLED_BIT }
    , format { fmt }
{

}

Canvas& Canvas::init() {
    createBuffer();
    createImageStorage();
    createDescriptorSetLayout();
    createDescriptorPool();
    createDescriptorSet();
    createPipeline();

    return *this;
}

void Canvas::recreate() {
    dispose(pipeline);
    descriptorPool.free(descriptorSet);
    disposeImage();

    createImageStorage();
    createDescriptorSet();
    createPipeline();
}

void Canvas::disposeImage(){
    dispose(image);
    dispose(imageView);
    dispose(sampler);
}

void Canvas::draw(VkCommandBuffer commandBuffer) {
    assert(pipeline);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, VK_NULL_HANDLE);
    std::array<VkDeviceSize, 1> offsets = {0u};
    std::array<VkBuffer, 2> buffers{ buffer, colorBuffer};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers.data() , offsets.data());
    vkCmdDraw(commandBuffer, 4, 1, 0, 0);
}

void Canvas::createDescriptorSetLayout() {
    std::array<VkDescriptorSetLayoutBinding, 1> binding{};
    binding[0].binding = 0;
    binding[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    binding[0].descriptorCount = 1;
    binding[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    descriptorSetLayout = app->device.createDescriptorSetLayout(binding);
}

void Canvas::createDescriptorPool() {
    std::array<VkDescriptorPoolSize, 1> poolSizes{};
    poolSizes[0].descriptorCount = 1;
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

    descriptorPool = app->device.createDescriptorPool(1, poolSizes, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT);
}

void Canvas::createDescriptorSet() {
    descriptorSet = descriptorPool.allocate({ descriptorSetLayout }).front();


    std::array<VkDescriptorImageInfo, 1> imageInfo{};
    imageInfo[0].imageView = imageView;
    imageInfo[0].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    imageInfo[0].sampler = sampler;

    auto writes = initializers::writeDescriptorSets<1>();
    writes[0].dstSet = descriptorSet;
    writes[0].dstBinding = 0;
    writes[0].dstArrayElement = 0;
    writes[0].descriptorCount = 1;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[0].pImageInfo = imageInfo.data();

    app->device.updateDescriptorSets(writes);
}

void Canvas::createPipeline() {
    auto vertexShaderModule = VulkanShaderModule{"../../data/shaders/quad.vert.spv", app->device };
    auto fragmentShaderModule = VulkanShaderModule{"../../data/shaders/quad.frag.spv", app->device };

    auto stages = initializers::vertexShaderStages({
                                                             { vertexShaderModule, VK_SHADER_STAGE_VERTEX_BIT}
                                                           , {fragmentShaderModule, VK_SHADER_STAGE_FRAGMENT_BIT}
                                                   });

    auto bindings = ClipSpace::bindingDescription();

    auto attributes = ClipSpace::attributeDescriptions();

    auto vertexInputState = initializers::vertexInputState(bindings, attributes);

    auto inputAssemblyState = initializers::inputAssemblyState(ClipSpace::Quad::topology);

    auto viewport = initializers::viewport(app->swapChain.extent);
    auto scissor = initializers::scissor(app->swapChain.extent);

    auto viewportState = initializers::viewportState( viewport, scissor);

    auto rasterState = initializers::rasterizationState();
    rasterState.polygonMode = VK_POLYGON_MODE_FILL;
    rasterState.cullMode = VK_CULL_MODE_NONE;
    rasterState.frontFace = ClipSpace::Quad::frontFace;

    auto multisampleState = initializers::multisampleState();
    multisampleState.rasterizationSamples = app->settings.msaaSamples;

    auto depthStencilState = initializers::depthStencilState();

    auto colorBlendAttachment = std::vector<VkPipelineColorBlendAttachmentState>(1);
    colorBlendAttachment[0].colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    auto colorBlendState = initializers::colorBlendState(colorBlendAttachment);

    dispose(pipelineLayout);
    pipelineLayout = app->device.createPipelineLayout({ descriptorSetLayout});


    VkGraphicsPipelineCreateInfo createInfo = initializers::graphicsPipelineCreateInfo();
    createInfo.stageCount = COUNT(stages);
    createInfo.pStages = stages.data();
    createInfo.pVertexInputState = &vertexInputState;
    createInfo.pInputAssemblyState = &inputAssemblyState;
    createInfo.pViewportState = &viewportState;
    createInfo.pRasterizationState = &rasterState;
    createInfo.pMultisampleState = &multisampleState;
    createInfo.pDepthStencilState = &depthStencilState;
    createInfo.pColorBlendState = &colorBlendState;
    createInfo.layout = pipelineLayout;
    createInfo.renderPass = app->renderPass;
    createInfo.subpass = 0;

    pipeline = app->device.createGraphicsPipeline(createInfo);
}

void Canvas::createBuffer() {
    VkDeviceSize size = sizeof(glm::vec2) * ClipSpace::Quad::positions.size();
    auto data = ClipSpace::Quad::positions;
    buffer = app->device.createDeviceLocalBuffer(data.data(), size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);

    std::vector<glm::vec4> colors{
            {1, 0, 0, 1},
            {0, 1, 0, 1},
            {0, 0, 1, 1},
            {1, 1, 0, 1}
    };
    size = sizeof(glm::vec4) * colors.size();
    colorBuffer = app->device.createDeviceLocalBuffer(colors.data(), size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
}

void Canvas::createImageStorage() {
    auto imageInfo = initializers::imageCreateInfo(
            VK_IMAGE_TYPE_2D,
            format,
            usageFlags,
            app->swapChain.extent.width,
            app->swapChain.extent.height
    );
    image = app->device.createImage(imageInfo);

    VkImageSubresourceRange subresourceRange{};
    subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceRange.baseMipLevel = 0;
    subresourceRange.levelCount = 1;
    subresourceRange.baseArrayLayer = 0;
    subresourceRange.layerCount = 1;
    imageView = image.createView(format, VK_IMAGE_VIEW_TYPE_2D, subresourceRange);
    app->device.graphicsCommandPool().oneTimeCommand([&](auto commandBuffer) {
        auto barrier = initializers::ImageMemoryBarrier();
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0,
                             0, nullptr, 0, nullptr, 1, &barrier);

    });

    VkSamplerCreateInfo samplerCreateInfo = initializers::samplerCreateInfo();
    samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
    samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
    samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    
    sampler = app->device.createSampler(samplerCreateInfo);
    
}

