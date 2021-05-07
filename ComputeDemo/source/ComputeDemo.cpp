#include "ComputeDemo.hpp"

ComputeDemo::ComputeDemo(const Settings &settings) : VulkanBaseApp("Compute Demo", settings) {

}

void ComputeDemo::initApp() {
    commandPool = device.createCommandPool(*device.queueFamilyIndex.graphics, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    commandBuffers = commandPool.allocate(swapChainImageCount);
    cpuCmdBuffers = commandPool.allocate(swapChainImageCount);
    createDescriptorPool();
    createVertexBuffer();
    createDescriptorSetLayout();
    createComputeDescriptorSetLayout();
    loadTexture();
    createComputeImage();
    createDescriptorSet();
    createGraphicsPipeline();
    createComputePipeline();
}


VkCommandBuffer& ComputeDemo::dispatchCompute() {
    auto& cpuCmdBuffer = cpuCmdBuffers[currentImageIndex];
    auto beginInfo = initializers::commandBufferBeginInfo();
    vkBeginCommandBuffer(cpuCmdBuffer, &beginInfo);
    vkCmdBindPipeline(cpuCmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, compute.pipeline);
    vkCmdBindDescriptorSets(cpuCmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, compute.pipelineLayout, 0, 1, &compute.descriptorSet, 0, nullptr);
    vkCmdDispatch(cpuCmdBuffer, swapChain.extent.width/32, swapChain.extent.height/32, 1);
    vkEndCommandBuffer(cpuCmdBuffer);


//    commandPool.oneTimeCommand(device.queues.compute, [&](auto cpuCmdBuffer){
//        vkCmdBindPipeline(cpuCmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, compute.pipeline);
//        vkCmdBindDescriptorSets(cpuCmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, compute.pipelineLayout, 0, 1, &compute.descriptorSet, 0, nullptr);
//        vkCmdDispatch(cpuCmdBuffer, width/32, height/32, 1);
//    });
    return cpuCmdBuffer;
}

VkCommandBuffer *ComputeDemo::buildCommandBuffers(uint32_t imageIndex, uint32_t &numCommandBuffers) {
    static std::array<VkCommandBuffer, 2> cmdBuffers{};
    numCommandBuffers = cmdBuffers.size();
   // numCommandBuffers = 1;

    auto& commandBuffer = commandBuffers[imageIndex];
    cmdBuffers[0] = commandBuffer;
    cmdBuffers[1] = dispatchCompute();

    VkClearValue clearValue{0.0f, 0.0f, 0.0f, 1.0f};

    auto beginInfo = initializers::commandBufferBeginInfo();
    vkBeginCommandBuffer(commandBuffer, &beginInfo);
    auto rpassBeginInfo = initializers::renderPassBeginInfo();
    rpassBeginInfo.renderPass = renderPass;
    rpassBeginInfo.framebuffer = framebuffers[imageIndex];
    rpassBeginInfo.renderArea.offset = {0, 0};
    rpassBeginInfo.renderArea.extent = swapChain.extent;
    rpassBeginInfo.clearValueCount = 1;
    rpassBeginInfo.pClearValues = &clearValue;

    vkCmdBeginRenderPass(commandBuffer, &rpassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, VK_NULL_HANDLE);
    std::array<VkDeviceSize, 1> offsets = {0u};
    std::array<VkBuffer, 2> buffers{ vertexBuffer, vertexColorBuffer};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers.data() , offsets.data());
    vkCmdDraw(commandBuffer, 4, 1, 0, 0);

    vkCmdEndRenderPass(commandBuffer);
    vkEndCommandBuffer(commandBuffer);

    return cmdBuffers.data();
}

void ComputeDemo::update(float time) {    VulkanBaseApp::update(time);
}

void ComputeDemo::checkAppInputs() {
    VulkanBaseApp::checkAppInputs();
}

void ComputeDemo::createGraphicsPipeline() {
    auto vertexShaderModule = ShaderModule{ "../../data/shaders/quad.vert.spv", device };
    auto fragmentShaderModule = ShaderModule{ "../../data/shaders/quad.frag.spv", device };

    auto stages = initializers::vertexShaderStages({
        { vertexShaderModule, VK_SHADER_STAGE_VERTEX_BIT}
        , {fragmentShaderModule, VK_SHADER_STAGE_FRAGMENT_BIT}
    });

    auto bindings = ClipSpace::bindingDescription();
 //   bindings.push_back({1, sizeof(glm::vec4), VK_VERTEX_INPUT_RATE_VERTEX});

    auto attributes = ClipSpace::attributeDescriptions();
 //   attributes.push_back({2, 1, VK_FORMAT_R32G32B32A32_SFLOAT, 0});

    auto vertexInputState = initializers::vertexInputState(bindings, attributes);

    auto inputAssemblyState = initializers::inputAssemblyState(ClipSpace::Quad::topology);

    auto viewport = initializers::viewport(swapChain.extent);
    auto scissor = initializers::scissor(swapChain.extent);

    auto viewportState = initializers::viewportState( viewport, scissor);

    auto rasterState = initializers::rasterizationState();
    rasterState.polygonMode = VK_POLYGON_MODE_FILL;
    rasterState.cullMode = VK_CULL_MODE_NONE;
    rasterState.frontFace = ClipSpace::Quad::frontFace;

    auto multisampleState = initializers::multisampleState();
    multisampleState.rasterizationSamples = settings.msaaSamples;

    auto depthStencilState = initializers::depthStencilState();

    auto colorBlendAttachment = std::vector<VkPipelineColorBlendAttachmentState>(1);
    colorBlendAttachment[0].colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    auto colorBlendState = initializers::colorBlendState(colorBlendAttachment);

    pipelineLayout = device.createPipelineLayout({ descriptorSetLayout});


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
    createInfo.renderPass = renderPass;
    createInfo.subpass = 0;

    pipeline = device.createGraphicsPipeline(createInfo);
}

void ComputeDemo::createVertexBuffer() {
    VkDeviceSize size = sizeof(glm::vec2) * ClipSpace::Quad::positions.size();
    auto data = ClipSpace::Quad::positions;
    vertexBuffer = device.createDeviceLocalBuffer(data.data(), size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);


    std::vector<glm::vec4> colors{
            {1, 0, 0, 1},
            {0, 1, 0, 1},
            {0, 0, 1, 1},
            {1, 1, 0, 1}
    };
    size = sizeof(glm::vec4) * colors.size();
    vertexColorBuffer = device.createDeviceLocalBuffer(colors.data(), size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
}

void ComputeDemo::createDescriptorSetLayout() {
    std::array<VkDescriptorSetLayoutBinding, 1> binding{};
    binding[0].binding = 0;
    binding[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    binding[0].descriptorCount = 1;
    binding[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    descriptorSetLayout = device.createDescriptorSetLayout(binding);
}

void ComputeDemo::createDescriptorSet() {
    auto sets = descriptorPool.allocate({ descriptorSetLayout, compute.descriptorSetLayout });
    descriptorSet = sets.front();
    compute.descriptorSet = sets.back();

//    std::array<VkDescriptorImageInfo, 1> imageInfo{};
//    imageInfo[0].imageView = texture.imageView;
//    imageInfo[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
//    imageInfo[0].sampler = texture.sampler;

    std::array<VkDescriptorImageInfo, 1> imageInfo{};
    imageInfo[0].imageView = compute.texture.imageView;
    imageInfo[0].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    imageInfo[0].sampler = compute.texture.sampler;

    auto writes = initializers::writeDescriptorSets<2>();
    writes[0].dstSet = descriptorSet;
    writes[0].dstBinding = 0;
    writes[0].dstArrayElement = 0;
    writes[0].descriptorCount = 1;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[0].pImageInfo = imageInfo.data();

    VkDescriptorImageInfo computeImageInfo{};
    computeImageInfo.imageView = compute.texture.imageView;
    computeImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

    writes[1].dstSet = compute.descriptorSet;
    writes[1].dstBinding = 0;
    writes[1].dstArrayElement = 0;
    writes[1].descriptorCount = 1;
    writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    writes[1].pImageInfo = &computeImageInfo;

    vkUpdateDescriptorSets(device, COUNT(writes), writes.data(), 0, nullptr);
}

void ComputeDemo::loadTexture() {
    textures::fromFile(device, texture, "../../data/textures/portrait.jpg", false, VK_FORMAT_R8G8B8A8_SRGB);
}

void ComputeDemo::createDescriptorPool() {
    std::array<VkDescriptorPoolSize, 2> poolSizes{};
    poolSizes[0].descriptorCount = 1;
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

    poolSizes[1].descriptorCount = 1;
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;

    descriptorPool = device.createDescriptorPool(2, poolSizes, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT);
}

void ComputeDemo::createComputeDescriptorSetLayout() {
    std::array<VkDescriptorSetLayoutBinding, 1> bindings{};
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    bindings[0].descriptorCount = 1;

    compute.descriptorSetLayout = device.createDescriptorSetLayout(bindings);
}



void ComputeDemo::createComputePipeline() {
    auto computeShaderModule = ShaderModule{"../../data/shaders/compute.comp.spv", device};
    auto stage = initializers::computeShaderStage({ computeShaderModule, VK_SHADER_STAGE_COMPUTE_BIT});
    compute.pipelineLayout = device.createPipelineLayout({compute.descriptorSetLayout});

    auto createInfo = initializers::computePipelineCreateInfo();
    createInfo.stage = stage;
    createInfo.layout = compute.pipelineLayout;

    compute.pipeline = device.createComputePipeline(createInfo);
}

void ComputeDemo::createComputeImage() {
    VkImageCreateInfo info = initializers::imageCreateInfo(VK_IMAGE_TYPE_2D, VK_FORMAT_R32G32B32A32_SFLOAT,
                                                           VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, swapChain.extent.width, swapChain.extent.height, 1);

    compute.texture.image = device.createImage(info, VMA_MEMORY_USAGE_GPU_ONLY);
    commandPool.oneTimeCommand( [&](auto commandBuffer) {
        auto barrier = initializers::ImageMemoryBarrier();
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = compute.texture.image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0,
                             0, nullptr, 0, nullptr, 1, &barrier);

    });

    VkImageSubresourceRange subresourceRange{};
    subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceRange.baseMipLevel = 0;
    subresourceRange.levelCount = 1;
    subresourceRange.baseArrayLayer = 0;
    subresourceRange.layerCount = 1;

    compute.texture.imageView = compute.texture.image.createView(VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_VIEW_TYPE_2D,
                                                                 subresourceRange);
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

    compute.texture.sampler = device.createSampler(samplerInfo);
}

void ComputeDemo::onSwapChainDispose() {
    dispose(compute.pipelineLayout);
    dispose(compute.pipeline);
    dispose(pipelineLayout);
    dispose(pipeline);
    descriptorPool.free({ compute.descriptorSet, descriptorSet });
    dispose(compute.texture.imageView);
    dispose(compute.texture.sampler);
    dispose(compute.texture.image);
}

void ComputeDemo::onSwapChainRecreation() {
    createComputeImage();
    createDescriptorSet();
    createGraphicsPipeline();
    createComputePipeline();
}

int main(){
    Settings settings;
    settings.width = 1024;
    settings.height = 1024;
  //  settings.surfaceFormat.format = VK_FORMAT_B8G8R8A8_SRGB;
    settings.queueFlags = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT;

    try{
        auto app = ComputeDemo{ settings };
        app.run();
    }catch(const std::runtime_error& err){
        spdlog::error(err.what());
    }
}