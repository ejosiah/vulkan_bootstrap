#include "ComputeDemo.hpp"

ComputeDemo::ComputeDemo(const Settings &settings) : VulkanBaseApp("Compute Demo", settings) {

}

void ComputeDemo::initApp() {
    commandPool = device.createCommandPool(*device.queueFamilyIndex.graphics, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    commandBuffers = commandPool.allocate(swapChainImageCount);
    createDescriptorPool();
    createVertexBuffer();
    createDescriptorSetLayout();
    loadTexture();
    createDescriptorSet();
    createGraphicsPipeline();
}

VkCommandBuffer *ComputeDemo::buildCommandBuffers(uint32_t imageIndex, uint32_t &numCommandBuffers) {
    numCommandBuffers = 1;
    auto& commandBuffer = commandBuffers[imageIndex];

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

    return &commandBuffer;
}

void ComputeDemo::update(float time) {
    VulkanBaseApp::update(time);
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
    descriptorSet = descriptorPool.allocate({ descriptorSetLayout }).front();

    std::array<VkDescriptorImageInfo, 1> imageInfo{};
    imageInfo[0].imageView = texture.imageView;
    imageInfo[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo[0].sampler = texture.sampler;

    auto writes = initializers::writeDescriptorSet();
    writes.dstSet = descriptorSet;
    writes.dstBinding = 0;
    writes.dstArrayElement = 0;
    writes.descriptorCount = 1;
    writes.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes.pImageInfo = imageInfo.data();

    vkUpdateDescriptorSets(device, 1, &writes, 0, nullptr);
}

void ComputeDemo::loadTexture() {
    textures::fromFile(device, texture, "../../data/textures/portrait.jpg", false, VK_FORMAT_R8G8B8A8_SRGB);
}

void ComputeDemo::createDescriptorPool() {
    std::array<VkDescriptorPoolSize, 1> poolSizes{};
    poolSizes[0].descriptorCount = 1;
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

    descriptorPool = device.createDescriptorPool(1, poolSizes);
}

int main(){
    Settings settings;
    settings.width = 1080;
    settings.height = 1349;
    settings.surfaceFormat.format = VK_FORMAT_B8G8R8A8_SRGB;

    try{
        auto app = ComputeDemo{ settings };
        app.run();
    }catch(const std::runtime_error& err){
        spdlog::error(err.what());
    }
}