#include "VulkanCube.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include  <stb_image.h>
#endif

VulkanCube::VulkanCube():VulkanBaseApp("VulkanCube", 1080, 720, true){}

void VulkanCube::initApp() {
    createCommandPool();
    createVertexBuffer();
    createIndexBuffer();
    createTextureBuffers();

    createDescriptorSetLayout();
    createDescriptorPool();
    createDescriptorSet();


    createGraphicsPipeline();
    createCommandBuffer();
}

void VulkanCube::onSwapChainDispose() {
    commandPool.free(commandBuffers);
    dispose(pipeline);
    dispose(layout);
    dispose(descriptorPool);
    dispose(descriptorSetLayout);
}

void VulkanCube::onSwapChainRecreation() {
    createDescriptorSet();

    createDescriptorSetLayout();
    createDescriptorPool();
    createDescriptorSet();

    createGraphicsPipeline();
    createCommandBuffer();
}

void VulkanCube::createGraphicsPipeline() {
    assert(renderPass != VK_NULL_HANDLE);
    auto vertexShaderModule = ShaderModule{"../../data/shaders/triangle.vert.spv", device};
    auto fragmentShaderModule = ShaderModule{"../../data/shaders/triangle.frag.spv", device};

    std::vector<VkPipelineShaderStageCreateInfo> stages = initializers::vertexShaderStages(
            {
                    {vertexShaderModule,   VK_SHADER_STAGE_VERTEX_BIT},
                    {fragmentShaderModule, VK_SHADER_STAGE_FRAGMENT_BIT}
            }
    );

    std::vector<VkVertexInputBindingDescription> vertexBindings{Vertex::bindingDisc()};
    std::vector<VkVertexInputAttributeDescription> vertexAttributes = Vertex::attributeDisc();
    VkPipelineVertexInputStateCreateInfo vertexInputState = initializers::vertexInputState(vertexBindings, vertexAttributes);


    VkPipelineInputAssemblyStateCreateInfo InputAssemblyState = initializers::inputAssemblyState();

    VkViewport viewport = initializers::viewport(swapChain.extent);

    VkRect2D scissor{};
    scissor.extent = swapChain.extent;
    scissor.offset = {0, 0};

    VkPipelineViewportStateCreateInfo viewportState = initializers::viewportState(viewport, scissor);

    VkPipelineRasterizationStateCreateInfo rasterizationState = initializers::rasterizationState();
    rasterizationState.rasterizerDiscardEnable = VK_FALSE;
    rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizationState.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizationState.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizationState.depthBiasEnable = VK_FALSE;
    rasterizationState.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo multisampleState = initializers::multisampleState();


    VkPipelineDepthStencilStateCreateInfo depthStencilState = initializers::depthStencilState();


    VkPipelineColorBlendStateCreateInfo blendState = initializers::colorBlendState();

    VkPipelineDynamicStateCreateInfo dynamicState = initializers::dynamicState();

    VkPushConstantRange range;
    range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    range.offset = 0;
    range.size = sizeof(Camera);

    //   layout = device.createPipelineLayout({descriptorSetLayout }, {range });
    layout = device.createPipelineLayout({cameraDescriptorSetLayout, descriptorSetLayout }, {range });

    VkGraphicsPipelineCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    createInfo.stageCount = static_cast<uint32_t>(stages.size());
    createInfo.pStages = stages.data();
    createInfo.pVertexInputState = &vertexInputState;
    createInfo.pInputAssemblyState = &InputAssemblyState;
    createInfo.pTessellationState = nullptr;
    createInfo.pViewportState = &viewportState;
    createInfo.pRasterizationState = &rasterizationState;
    createInfo.pMultisampleState  = &multisampleState;
    createInfo.pDepthStencilState = &depthStencilState;
    createInfo.pColorBlendState = &blendState;
    createInfo.pDynamicState = &dynamicState;
    createInfo.layout = layout;
    createInfo.renderPass = renderPass;
    createInfo.subpass = 0;
    createInfo.basePipelineIndex = -1;
    createInfo.basePipelineHandle = VK_NULL_HANDLE;

    pipeline = device.createGraphicsPipeline(createInfo);
}

std::vector<VkCommandBuffer> VulkanCube::buildCommandBuffers(uint32_t i) {
    return { commandBuffers[i] };
}

void VulkanCube::update(float time) {
    VulkanBaseApp::update(time);
}

void VulkanCube::createCommandBuffer() {
    commandBuffers.resize(swapChain.imageCount() + 1);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = commandBuffers.size();

    REPORT_ERROR(vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()), "Failed to allocate command buffer");

    auto last = commandBuffers.begin();
    std::advance(last, commandBuffers.size() - 1);
    commandBuffers.erase(last);

    for(auto i = 0; i < commandBuffers.size(); i++){
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0;
        vkBeginCommandBuffer(commandBuffers[i], &beginInfo);

        VkClearValue clear{0.f, 0.f, 0.f, 1.f};

        VkRenderPassBeginInfo renderPassBeginInfo{};
        renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassBeginInfo.renderPass = renderPass;
        renderPassBeginInfo.framebuffer = framebuffers[i];
        renderPassBeginInfo.renderArea.offset = {0, 0};
        renderPassBeginInfo.renderArea.extent = swapChain.extent;
        renderPassBeginInfo.clearValueCount = 1;
        renderPassBeginInfo.pClearValues = &clear;
        vkCmdBeginRenderPass(commandBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE );

        std::vector<VkDescriptorSet> descriptorSets{ cameraDescriptorSets[i], descriptorSet };
        //   std::vector<VkDescriptorSet> descriptorSets{ cameraDescriptorSets[i] };

        vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, COUNT(descriptorSets), descriptorSets.data(), 0, nullptr);
        vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline );
        VkDeviceSize offset = 0;
        vkCmdBindIndexBuffer(commandBuffers[i], indexBuffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, &vertexBuffer.buffer, &offset);
        vkCmdDrawIndexed(commandBuffers[i], static_cast<uint32_t>(mesh.indices.size()), 1u, 0u, 0u, 0u);
//        vkCmdDraw(commandBuffers[i], 3, 1, 0, 0);

        vkCmdEndRenderPass(commandBuffers[i]);

        vkEndCommandBuffer(commandBuffers[i]);
    }
}

void VulkanCube::createDescriptorPool() {


    VkDescriptorPoolSize  texturePoolSize{};
    texturePoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    texturePoolSize.descriptorCount = 1;

    std::vector<VkDescriptorPoolSize> poolSizes{
            texturePoolSize
    };

    descriptorPool = device.createDescriptorPool(swapChain.imageCount(), poolSizes);


}

void VulkanCube::createDescriptorSet() {
    const auto swapChainImageCount = 1;

    std::vector<VkDescriptorSetLayout> layouts(swapChainImageCount, descriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &descriptorSetLayout.handle;
    vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet);

    VkWriteDescriptorSet textureWrites{};
    textureWrites.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    textureWrites.dstSet = descriptorSet;
    textureWrites.dstBinding = 0;
    textureWrites.dstArrayElement = 0;
    textureWrites.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    textureWrites.descriptorCount = 1;

    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = texture.imageView;
    imageInfo.sampler = texture.sampler;
    textureWrites.pImageInfo = &imageInfo;

    std::vector<VkWriteDescriptorSet> writes{
            textureWrites,
    };

    vkUpdateDescriptorSets(device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
}

void VulkanCube::createTextureBuffers() {
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load("../../data/media/vulkan.png", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    VkDeviceSize imageSize = texWidth * texHeight * STBI_rgb_alpha;

    if(!pixels){
        throw std::runtime_error{"failed to load texture image!"};
    }
    VulkanBuffer stagingBuffer = device.createBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY, imageSize);
    stagingBuffer.copy(pixels, imageSize);

    stbi_image_free(pixels);

    VkImageCreateInfo imageCreateInfo{};
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    imageCreateInfo.extent = { static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), 1u};
    imageCreateInfo.mipLevels = 1;
    imageCreateInfo.arrayLayers = 1;
    imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCreateInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    texture.image = device.createImage(imageCreateInfo, VMA_MEMORY_USAGE_GPU_ONLY);
    texture.image.transitionLayout(commandPool, device.queues.graphics, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    commandPool.oneTimeCommand(device.queues.graphics, [&](auto cmdBuffer) {
        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;

        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = {0, 0, 0};
        region.imageExtent = {static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), 1u};

        vkCmdCopyBufferToImage(cmdBuffer, stagingBuffer, texture.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                               &region);
    });

    texture.image.transitionLayout(commandPool, device.queues.graphics, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    VkImageSubresourceRange subresourceRange;
    subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceRange.baseMipLevel = 0;
    subresourceRange.levelCount = 1;
    subresourceRange.baseArrayLayer = 0;
    subresourceRange.layerCount = 1;

    texture.imageView = texture.image.createView(VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_VIEW_TYPE_2D, subresourceRange);

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 16;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    texture.sampler = device.createSampler(samplerInfo);
}

void VulkanCube::createVertexBuffer() {

    VkDeviceSize size = sizeof(Vertex) * mesh.vertices.size();

    VulkanBuffer stagingBuffer = device.createBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY, size);
    stagingBuffer.copy(mesh.vertices.data(), size);

    vertexBuffer = device.createBuffer(
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VMA_MEMORY_USAGE_GPU_ONLY,
            size);

    commandPool.oneTimeCommand(device.queues.graphics, [&](VkCommandBuffer cmdBuffer){
        VkBufferCopy copy{};
        copy.size = size;
        copy.dstOffset = 0;
        copy.srcOffset = 0;
        vkCmdCopyBuffer(cmdBuffer, stagingBuffer, vertexBuffer, 1u, &copy);
    });
}

void VulkanCube::createIndexBuffer() {
    VkDeviceSize size = sizeof(mesh.indices[0]) * mesh.indices.size();

    VulkanBuffer stagingBuffer = device.createBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY, size);
    stagingBuffer.copy(mesh.indices.data(), size);

    indexBuffer = device.createBuffer(
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            VMA_MEMORY_USAGE_GPU_ONLY,
            size);

    commandPool.oneTimeCommand(device.queues.graphics, [&](VkCommandBuffer cmdBuffer){
        VkBufferCopy copy{};
        copy.size = size;
        copy.dstOffset = 0;
        copy.srcOffset = 0;
        vkCmdCopyBuffer(cmdBuffer, stagingBuffer, indexBuffer, 1u, &copy);
    });
}

void VulkanCube::createCommandPool() {
    VkCommandPoolCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    createInfo.queueFamilyIndex = device.queueFamilyIndex.graphics.value();

    commandPool = device.createCommandPool(*device.queueFamilyIndex.graphics);
}

void VulkanCube::createDescriptorSetLayout() {
    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding = 0;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.pImmutableSamplers = nullptr;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    std::vector<VkDescriptorSetLayoutBinding> bindings {
            samplerLayoutBinding
    };

    VkDescriptorSetLayoutCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    createInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    createInfo.pBindings = bindings.data();

    descriptorSetLayout = device.createDescriptorSetLayout(bindings);
}

