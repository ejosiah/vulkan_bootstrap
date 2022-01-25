#include <ComputeDemo.hpp>
#include "ComputeDemo.hpp"
#include "ImGuiPlugin.hpp"

ComputeDemo::ComputeDemo(const Settings &settings) : VulkanBaseApp("Compute Demo", settings) {

}

void ComputeDemo::initApp() {
    commandPool = device.createCommandPool(*device.queueFamilyIndex.graphics, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    commandBuffers = commandPool.allocateCommandBuffers(swapChainImageCount);
    cpuCmdBuffers = commandPool.allocateCommandBuffers(swapChainImageCount);

//    blur.constants.weights[0][0] = 1/273.0f;
//    blur.constants.weights[0][1] = 4/273.0f;
//    blur.constants.weights[0][2] = 7/273.0f;
//    blur.constants.weights[0][3] = 4/273.0f;
//    blur.constants.weights[0][4] = 1/273.0f;
//
//    blur.constants.weights[1][0] = 4/273.0f;
//    blur.constants.weights[1][1] = 16/273.0f;
//    blur.constants.weights[1][2] = 25/273.0f;
//    blur.constants.weights[1][3] = 16/273.0f;
//    blur.constants.weights[1][4] = 4/273.0f;
//
//
//    blur.constants.weights[2][0] = 7/273.0f;
//    blur.constants.weights[2][1] = 26/273.0f;
//    blur.constants.weights[2][2] = 41/273.0f;
//    blur.constants.weights[2][3] = 26/273.0f;
//    blur.constants.weights[2][4] = 7/273.0f;
//
//    blur.constants.weights[3][0] = 4/273.0f;
//    blur.constants.weights[3][1] = 16/273.0f;
//    blur.constants.weights[3][2] = 26/273.0f;
//    blur.constants.weights[3][3] = 16/273.0f;
//    blur.constants.weights[3][4] = 4/273.0f;
//
//    blur.constants.weights[4][0] = 1/273.0f;
//    blur.constants.weights[4][1] = 4/273.0f;
//    blur.constants.weights[4][2] = 7/273.0f;
//    blur.constants.weights[4][3] = 4/273.0f;
//    blur.constants.weights[4][4] = 1/273.0f;
    createDescriptorPool();
    createVertexBuffer();
    createSamplers();
    createDescriptorSetLayout();
    createComputeDescriptorSetLayout();
    loadTexture();
    createComputeImage();
    initRenderBlur();
    createDescriptorSet();
    createGraphicsPipeline();
    createComputePipeline();
//    blurImage();
//    blurImageRender();
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
    static std::array<VkCommandBuffer, 1> cmdBuffers{};
    numCommandBuffers = cmdBuffers.size();

    auto& commandBuffer = commandBuffers[imageIndex];
    cmdBuffers[0] = commandBuffer;
//    cmdBuffers[1] = dispatchCompute();

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

//    auto set = blur.on ? blur.renderBlur[0].descriptorSet : descriptorSet;
    VkDescriptorSet set = VK_NULL_HANDLE;
    if(blur.on){
        set = blur.useRender ? blur.renderBlur[0].descriptorSet : blur.inSet;
    }else{
        set = descriptorSet;
    }
    vkCmdBeginRenderPass(commandBuffer, &rpassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &set, 0, VK_NULL_HANDLE);
    std::array<VkDeviceSize, 1> offsets = {0u};
    std::array<VkBuffer, 2> buffers{ vertexBuffer, vertexColorBuffer};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers.data() , offsets.data());
    vkCmdDraw(commandBuffer, 4, 1, 0, 0);

    renderUI(commandBuffer);

    vkCmdEndRenderPass(commandBuffer);
    vkEndCommandBuffer(commandBuffer);

    return cmdBuffers.data();
}

void ComputeDemo::update(float time) {
    glfwSetWindowTitle(window, fmt::format("{} - FPS {}", title, framePerSecond).c_str());
    static int preIterations = 0;
    static float sd = 0;
    static bool useRender = false;
    if(blur.on && (preIterations != blur.iterations || useRender != blur.useRender || blur.sd != sd)){
        preIterations = blur.iterations;
        useRender = blur.useRender;
        sd = blur.sd;
        if(useRender){
            spdlog::info("using render pipeline blur");
            blurImageRender();
        }else{
            spdlog::info("using compute pipeline blur");
            updateBlurFunc();
            blurImage();
        }
    }
}

void ComputeDemo::checkAppInputs() {
    VulkanBaseApp::checkAppInputs();
}

void ComputeDemo::createGraphicsPipeline() {
    auto vertexShaderModule = VulkanShaderModule{"../../data/shaders/quad.vert.spv", device };
    auto fragmentShaderModule = VulkanShaderModule{"../../data/shaders/quad.frag.spv", device };

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

    pipelineLayout = device.createPipelineLayout({textureSetLayout});


    VkGraphicsPipelineCreateInfo createInfo = initializers::graphicsPipelineCreateInfo();
    createInfo.flags |= VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT;
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

    fragmentShaderModule = VulkanShaderModule{"../../data/shaders/blur.frag.spv", device };
    stages = initializers::vertexShaderStages({
            { vertexShaderModule, VK_SHADER_STAGE_VERTEX_BIT}
          , {fragmentShaderModule, VK_SHADER_STAGE_FRAGMENT_BIT}
    });

    auto extent = texture.image.getDimensions();
    viewport = initializers::viewport({extent.x, extent.y});
    scissor = initializers::scissor({extent.x, extent.y});

    viewportState = initializers::viewportState( viewport, scissor);


    createInfo.flags |= VK_PIPELINE_CREATE_DERIVATIVE_BIT;
    createInfo.basePipelineHandle = pipeline;
    createInfo.basePipelineIndex = -1;

    for(int i = 0; i < 2; i++){
        blur.renderBlur[i].layout = device.createPipelineLayout({textureSetLayout}, {{VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(blur.constants)}});
        device.setName<VK_OBJECT_TYPE_PIPELINE_LAYOUT>(fmt::format("render_blur_", i), blur.renderBlur[i].layout.pipelineLayout);

        createInfo.stageCount = 2;
        createInfo.pStages = stages.data();
        createInfo.pViewportState = &viewportState;
        createInfo.layout = blur.renderBlur[i].layout;
        createInfo.renderPass = blur.renderBlur[i].renderPass;
        createInfo.subpass = 0;
        blur.renderBlur[i].pipeline = device.createGraphicsPipeline(createInfo);
        device.setName<VK_OBJECT_TYPE_PIPELINE>(fmt::format("render_blur_", i), blur.renderBlur[i].pipeline.handle);
    }
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
    binding[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
    binding[0].pImmutableSamplers = &sampler.handle;

    textureSetLayout = device.createDescriptorSetLayout(binding);
}

void ComputeDemo::createDescriptorSet() {
    auto sets = descriptorPool.allocate({textureSetLayout, textureSetLayout,  compute.imageSetLayout, compute.imageSetLayout });
    descriptorSet = sets[0];
    blur.inSet = sets[1];
    compute.descriptorSet = sets[2];
    blur.outSet = sets[3];

    device.setName<VK_OBJECT_TYPE_DESCRIPTOR_SET>("blur_in", blur.inSet);
    device.setName<VK_OBJECT_TYPE_DESCRIPTOR_SET>("blur_out", blur.outSet);
    device.setName<VK_OBJECT_TYPE_IMAGE>("texture", texture.image.image);
    device.setName<VK_OBJECT_TYPE_IMAGE_VIEW>("texture", texture.imageView.handle);
    device.setName<VK_OBJECT_TYPE_IMAGE>("compute_image", compute.texture.image.image);
    device.setName<VK_OBJECT_TYPE_IMAGE_VIEW>("compute_image", compute.texture.imageView.handle);

    std::vector<VkDescriptorImageInfo> imageInfo(1);
    imageInfo[0].imageView = texture.imageView;
    imageInfo[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo[0].sampler = VK_NULL_HANDLE;

//    imageInfo[0].imageView = compute.texture.imageView;
//    imageInfo[0].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
//    imageInfo[0].sampler = VK_NULL_HANDLE;

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

    // blur set update

    imageInfo[0].imageView = compute.texture.imageView;
    imageInfo[0].imageLayout = VK_IMAGE_LAYOUT_GENERAL;

    writes = initializers::writeDescriptorSets<2>();
    writes[0].dstSet = blur.inSet;
    writes[0].dstBinding = 0;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[0].descriptorCount = 1;
    writes[0].pImageInfo = imageInfo.data();

    writes[1].dstSet = blur.outSet;
    writes[1].dstBinding = 0;
    writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    writes[1].descriptorCount = 1;
    writes[1].pImageInfo = imageInfo.data();

    device.updateDescriptorSets(writes);


    // blur using graphics pipeline
    sets =  descriptorPool.allocate({textureSetLayout, textureSetLayout});
    writes = initializers::writeDescriptorSets<2>();
    imageInfo.resize(2);
    for(int i = 0; i < 2; i++){
        blur.renderBlur[i].descriptorSet = sets[i];

        writes[i].dstSet = blur.renderBlur[i].descriptorSet;
        writes[i].dstBinding = 0;
        writes[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writes[i].descriptorCount = 1;

        imageInfo[i].imageView = blur.renderBlur[i].colorAttachment.imageView;
        imageInfo[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo[i].sampler = VK_NULL_HANDLE;
        writes[i].pImageInfo = &imageInfo[i];
    }
    device.updateDescriptorSets(writes);
}

void ComputeDemo::loadTexture() {
    textures::fromFile(device, texture, "../../data/textures/portrait.jpg", false, VK_FORMAT_R8G8B8A8_UNORM);
}

void ComputeDemo::createDescriptorPool() {
    std::array<VkDescriptorPoolSize, 2> poolSizes{};
    poolSizes[0].descriptorCount = 10;
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

    poolSizes[1].descriptorCount = 10;
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;

    descriptorPool = device.createDescriptorPool(10, poolSizes, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT);
}

void ComputeDemo::createComputeDescriptorSetLayout() {
    std::array<VkDescriptorSetLayoutBinding, 1> bindings{};
    bindings[0].binding = 0;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    bindings[0].descriptorCount = 1;

    compute.imageSetLayout = device.createDescriptorSetLayout(bindings);
}



void ComputeDemo::createComputePipeline() {
    auto computeShaderModule = VulkanShaderModule{"../../data/shaders/compute.comp.spv", device};
    auto stage = initializers::computeShaderStage({ computeShaderModule, VK_SHADER_STAGE_COMPUTE_BIT});
    compute.pipelineLayout = device.createPipelineLayout({compute.imageSetLayout});

    auto createInfo = initializers::computePipelineCreateInfo();
    createInfo.stage = stage;
    createInfo.layout = compute.pipelineLayout;

    compute.pipeline = device.createComputePipeline(createInfo);

    auto blurShader = VulkanShaderModule{"../../data/shaders/blur.comp.spv", device};
    stage = initializers::computeShaderStage({ blurShader, VK_SHADER_STAGE_COMPUTE_BIT});
    VkPushConstantRange range{VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(blur.constants)};
    blur.layout = device.createPipelineLayout({ textureSetLayout, compute.imageSetLayout }, {range});
    createInfo.stage = stage;
    createInfo.layout = blur.layout;
    blur.pipeline = device.createComputePipeline(createInfo);
}

void ComputeDemo::createComputeImage() {
    auto width = texture.image.dimension.width;
    auto height = texture.image.dimension.height;
    VkImageCreateInfo info = initializers::imageCreateInfo(VK_IMAGE_TYPE_2D, VK_FORMAT_R32G32B32A32_SFLOAT,
                                                           VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, width, height, 1);

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

void ComputeDemo::blurImage() {
    device.computeCommandPool().oneTimeCommand([&](auto commandBuffer){

        static std::array<VkDescriptorSet, 2> sets;
        sets[0] = descriptorSet;
        sets[1] = blur.outSet;

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, blur.pipeline);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, blur.layout, 0, 2, sets.data(), 0, VK_NULL_HANDLE);
        glm::vec3 groupCount{compute.texture.image.getDimensions()};
        int blurAmount = blur.iterations;
        for(int i = 0; i < blurAmount; i++){
            blur.constants.horizontal = i%2 == 0;
            vkCmdPushConstants(commandBuffer, blur.layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(blur.constants), &blur.constants);
            vkCmdDispatch(commandBuffer, groupCount.x, groupCount.y, groupCount.z);

            VkImageMemoryBarrier barrier = initializers::ImageMemoryBarrier();
            barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.image = compute.texture.image;
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.layerCount = 1;
            barrier.subresourceRange.levelCount = 1;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.baseArrayLayer = 0;
            vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT , VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT , 0
                                 , 0, VK_NULL_HANDLE, 0, VK_NULL_HANDLE, 1, &barrier);


            sets[0] = blur.inSet;
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, blur.layout, 0, 2, sets.data(), 0, VK_NULL_HANDLE);

        }

    });
}

void ComputeDemo::createSamplers() {
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_NEAREST;
    samplerInfo.minFilter = VK_FILTER_NEAREST;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST ;

    sampler = device.createSampler(samplerInfo);
}

void ComputeDemo::renderUI(VkCommandBuffer commandBuffer) {
    ImGui::Begin("Blur");
    ImGui::SetWindowSize({250, 150});

    ImGui::Checkbox("blur", &blur.on);

    if(blur.on){
        static int option = 0;
        ImGui::RadioButton("compute", &option, 0); ImGui::SameLine();
        ImGui::RadioButton("render", &option, 1);
        blur.useRender = option == 1;
        ImGui::SliderFloat("sd", &blur.sd, 0.5, 5.0);
        ImGui::SliderInt("iterations", &blur.iterations, 1, 1000);

    }
    ImGui::End();
    plugin(IM_GUI_PLUGIN).draw(commandBuffer);
}

void ComputeDemo::updateBlurFunc() {
    static float sd = 0.0f;
    static glm::vec2 avg{0};
    auto update = sd != blur.sd;
//    if(update) {
//        sd = blur.sd;
//        avg = blur.avg;
//        for (int i = 0; i < 5; i++) {
//            for (int j = 0; j < 5; j++) {
//                float x = glm::mix(-2.0f, 2.0f, float(i) / 4);
//                float y = glm::mix(-2.0f, 2.0f, float(j) / 4);
//                auto rhs = glm::exp(-((x - avg.x) * (x-avg.x) + (y-avg.y) * (y-avg.y)) / (2 * sd * sd));
//                auto lhs = 1.0f / (2.0f * PI * sd * sd);
//                auto z = lhs * rhs;
////                blur.constants.weights[i][j] = z;
//            fmt::print("{} ", (z));
//            }
//        fmt::print("\n");
//        }
//    }
        sd = blur.sd;
        int N = blur.constants.weights.size();
        float sum = 0;
        for (int i = 0; i < N; i++) {
                auto x = static_cast<float>(i);
                auto rhs = glm::exp(-(x * x) / (2 * sd * sd));
                auto lhs = 1.0f / glm::sqrt(2.0f * PI * sd * sd);
                auto y = lhs * rhs;
                blur.constants.weights[i] = y;
                fmt::print("{} => {} ", x, y);
                sum += y;
        }
        fmt::print("\narea: {}", sum);
}

void ComputeDemo::initRenderBlur() {

    VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
    auto width = texture.image.getDimensions().x;
    auto height = texture.image.getDimensions().y;
    auto createFrameBufferAttachments = [&]{
        auto subresourceRange = initializers::imageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);

        auto createImageInfo = initializers::imageCreateInfo(VK_IMAGE_TYPE_2D, format,
                                                             VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
                                                             | VK_IMAGE_USAGE_SAMPLED_BIT,
                                                             width,
                                                             height);

        for(int i = 0; i < 2; i++){
            blur.renderBlur[i].colorAttachment.image = device.createImage(createImageInfo);
            blur.renderBlur[i].colorAttachment.imageView =
                    blur.renderBlur[i].colorAttachment
                        .image.createView(format, VK_IMAGE_VIEW_TYPE_2D, subresourceRange);

            device.setName<VK_OBJECT_TYPE_IMAGE>(fmt::format("render_blur_{}", i), blur.renderBlur[i].colorAttachment.image.image);
            device.setName<VK_OBJECT_TYPE_IMAGE_VIEW>(fmt::format("render_blur_{}", i), blur.renderBlur[i].colorAttachment.imageView.handle);
        }
    };
    
    auto createRenderPass = [&]{
        for(int i = 0; i < 2; i++) {
            std::vector<VkAttachmentDescription> attachments{
                    {
                            0, // flags
                            format,
                            VK_SAMPLE_COUNT_1_BIT, // TODO may need to add resolve if more than 1
                            VK_ATTACHMENT_LOAD_OP_CLEAR,
                            VK_ATTACHMENT_STORE_OP_STORE,
                            VK_ATTACHMENT_LOAD_OP_DONT_CARE,    // stencil load op
                            VK_ATTACHMENT_STORE_OP_DONT_CARE,   // stencil store op
                            VK_IMAGE_LAYOUT_UNDEFINED,
                            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    }
            };

            std::vector<SubpassDescription> subpasses(1);
            subpasses[0].colorAttachments.push_back({0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});

            std::vector<VkSubpassDependency> dependencies(2);
            dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
            dependencies[0].dstSubpass = 0;
            dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            dependencies[0].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
            dependencies[0].dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
            dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

            dependencies[1].srcSubpass = 0;
            dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
            dependencies[1].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            dependencies[1].srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
            dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

            blur.renderBlur[i].renderPass = device.createRenderPass(attachments, subpasses, dependencies);
            device.setName<VK_OBJECT_TYPE_RENDER_PASS>(fmt::format("render_blur_{}", i), blur.renderBlur[i].renderPass.renderPass);
        }

    };

    auto createFrameBuffer = [&]{
        for(int i = 0; i < 2; i++){
            assert(blur.renderBlur[i].renderPass);
            std::vector<VkImageView> attachments{ blur.renderBlur[i].colorAttachment.imageView};
            blur.renderBlur[i].framebuffer = device.createFramebuffer(blur.renderBlur[i].renderPass, attachments, width, height);
            device.setName<VK_OBJECT_TYPE_FRAMEBUFFER>(fmt::format("render_blur_{}", i), blur.renderBlur[i].framebuffer.frameBuffer);
        }
    };

    createFrameBufferAttachments();
    createRenderPass();
    createFrameBuffer();
}

void ComputeDemo::blurImageRender() {
    device.graphicsCommandPool().oneTimeCommand([&](auto commandBuffer){

       static VkClearValue clearValue{0.0f, 0.0f, 0.0f, 1.0f};
       auto imageSet = descriptorSet;
       int next;
       int blurAmount = int(float(blur.iterations + 1)/2) * 2;
       spdlog::info("blurAmount {}", blurAmount);
       for(int i = 0; i < blurAmount; i++){
            next = i%2;
            VkRenderPassBeginInfo renderInfo = initializers::renderPassBeginInfo();
            renderInfo.renderPass = blur.renderBlur[next].renderPass;
            renderInfo.framebuffer = blur.renderBlur[next].framebuffer;
            renderInfo.renderArea.offset = {0, 0};
            renderInfo.renderArea.extent = {texture.image.dimension.width, texture.image.dimension.height};
            renderInfo.clearValueCount = 1;
            renderInfo.pClearValues = &clearValue;

           vkCmdBeginRenderPass(commandBuffer, &renderInfo, VK_SUBPASS_CONTENTS_INLINE);


           vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, blur.renderBlur[next].pipeline);
           vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, blur.renderBlur[next].layout, 0, 1, &imageSet, 0, VK_NULL_HANDLE);

           blur.constants.horizontal = next == 0;
           vkCmdPushConstants(commandBuffer, blur.renderBlur[next].layout, VK_SHADER_STAGE_FRAGMENT_BIT
                              , 0, sizeof(blur.constants), &blur.constants);

           std::array<VkDeviceSize, 1> offsets = {0u};
           std::array<VkBuffer, 2> buffers{ vertexBuffer, vertexColorBuffer};
           vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers.data() , offsets.data());
           vkCmdDraw(commandBuffer, 4, 1, 0, 0);

           vkCmdEndRenderPass(commandBuffer);

//           auto barrier = initializers::ImageMemoryBarrier();
//           barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
//           barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
//           barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
//           barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
//           barrier.image = blur.renderBlur[next].colorAttachment.image;
//           barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
//           barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
//           barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
//           barrier.subresourceRange.baseMipLevel = 0;
//           barrier.subresourceRange.levelCount = 1;
//           barrier.subresourceRange.baseArrayLayer = 0;
//           barrier.subresourceRange.layerCount = 1;
//
//           vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
//                                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
//                                0, 0, VK_NULL_HANDLE, 0,
//                                VK_NULL_HANDLE, 1, &barrier);

           imageSet = blur.renderBlur[next].descriptorSet;
       }
       spdlog::info("final set {}", int(!next));
    });
}

int main(){
    Settings settings;
    settings.width = 1024;
    settings.height = 1024;
//    settings.deviceExtensions.push_back("VK_KHR_shader_non_semantic_info");
  //  settings.surfaceFormat.format = VK_FORMAT_B8G8R8A8_SRGB;
    settings.queueFlags = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT;

    try{
        auto app = ComputeDemo{ settings };
        std::unique_ptr<Plugin> plugin = std::make_unique<ImGuiPlugin>();
        app.addPlugin(plugin);
        app.run();
    }catch(const std::runtime_error& err){
        spdlog::error(err.what());
    }
}