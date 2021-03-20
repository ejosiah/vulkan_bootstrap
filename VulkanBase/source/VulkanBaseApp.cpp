//
// Created by Josiah on 1/17/2021.
//

#include <set>
#include <chrono>
#include <fstream>
#include "VulkanBaseApp.h"
#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include  <stb_image.h>

#endif

#include "keys.h"
#include "events.h"
#include "VulkanInitializers.h"

namespace chrono = std::chrono;

ShaderModule::ShaderModule(const std::string& path, VkDevice device)
    :device(device) {
    auto data = VulkanBaseApp::loadFile(path);
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = data.size();
    createInfo.pCode = reinterpret_cast<uint32_t*>(data.data());

    REPORT_ERROR(vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule), "Failed to create shader module: " + path);
}

ShaderModule::~ShaderModule() {
    vkDestroyShaderModule(device, shaderModule, nullptr);
}

VulkanBaseApp::VulkanBaseApp(std::string_view name, int width, int height, bool relativeMouseMode)
: Window(name, width, height)
, InputManager(relativeMouseMode)
{
}


void VulkanBaseApp::init() {
    initWindow();
    initInputMgr(*this);
    exit = &mapToKey(Key::ESCAPE, "Exit", Action::detectInitialPressOnly());
    pause = &mapToKey(Key::P, "Pause", Action::detectInitialPressOnly());
    initVulkan();
    initApp();
    OrbitingCameraSettings settings{};
    settings.offsetDistance = 2.0f;
    settings.rotationSpeed = 0.1f;
    settings.zNear = 0.1f;
    settings.zFar = 10.0f;
    settings.fieldOfView = 45.0f;
    settings.modelHeight = 0;
    settings.aspectRatio = static_cast<float>(swapChain.extent.width)/static_cast<float>(swapChain.extent.height);
    cameraController = new OrbitingCameraController{ camera, *this, settings};
//    cameraController = new FirstPersonCameraController{ camera, *this, settings};
//    cameraController->lookAt({0, 0, 2}, glm::vec3(0), {0, 1, 0});
}

void VulkanBaseApp::initWindow() {
    Window::initWindow();
    uint32_t size;
    auto extensions = glfwGetRequiredInstanceExtensions(&size);
    instanceExtensions = std::vector<const char*>(extensions, extensions + size);

    if(enableValidation){
        instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        instanceExtensions.push_back(VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME);
        validationLayers.push_back("VK_LAYER_KHRONOS_validation");
    }
}

void VulkanBaseApp::initVulkan() {
    mesh = primitives::cube();
    createInstance();
    ext = VulkanExtensions{instance};
    createDebugMessenger();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
    createCommandPool();
    createVertexBuffer();
    createIndexBuffer();
    createCameraBuffers();
    createRenderPass();
    createFramebuffer();
    createTextureBuffers();

    createCameraDescriptorPool();
    createCameraDescriptorSetLayout();
    createCameraDescriptorSet();

    createDescriptorSetLayout();
    createDescriptorPool();
    createDescriptorSet();

    createGraphicsPipeline();
    createCommandBuffer();
    createSyncObjects();
}

void VulkanBaseApp::createInstance() {
    VkApplicationInfo appInfo{};
    appInfo.sType  = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.applicationVersion = VK_MAKE_VERSION(0, 0, 0);
    appInfo.pApplicationName = title.data();
    appInfo.apiVersion = VK_API_VERSION_1_2;
    appInfo.pEngineName = "";

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
    createInfo.ppEnabledExtensionNames = instanceExtensions.data();

    if(enableValidation){
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
        auto debugInfo = VulkanDebug::debugCreateInfo();
        createInfo.pNext = &debugInfo;
    }

    instance = VulkanInstance{appInfo, {instanceExtensions, validationLayers}};
}

void VulkanBaseApp::createSwapChain() {
    glfwGetFramebufferSize(window, &width, &height);
    swapChain = VulkanSwapChain{ device, surface, static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
}

void VulkanBaseApp::createLogicalDevice() {
    VkPhysicalDeviceFeatures enabledFeatures{};
    device.createLogicalDevice(enabledFeatures, deviceExtensions, validationLayers, surface, VK_QUEUE_GRAPHICS_BIT);
    memoryAllocator = device.allocator;
}

void VulkanBaseApp::pickPhysicalDevice() {
    surface = VulkanSurface{instance, window};
    auto pDevices = enumerate<VkPhysicalDevice>([&](uint32_t* size, VkPhysicalDevice* pDevice){
        return vkEnumeratePhysicalDevices(instance, size, pDevice);
    });

    std::vector<VulkanDevice> devices(pDevices.size());
    std::transform(begin(pDevices), end(pDevices), begin(devices),[&](auto pDevice){
        return VulkanDevice{instance, pDevice};
    });

    std::sort(begin(devices), end(devices), [](auto& a, auto& b){
        return a.score() > b.score();
    });

    device = std::move(devices.front());
    spdlog::info("selected device: {}", device.name());
}

void VulkanBaseApp::createCommandPool() {
    VkCommandPoolCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    createInfo.queueFamilyIndex = device.queueFamilyIndex.graphics.value();

    commandPool = device.createCommandPool(*device.queueFamilyIndex.graphics);
}

void VulkanBaseApp::createCommandBuffer() {
    commandBuffers.resize(swapChain.imageCount() + 1);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = commandBuffers.size();

    REPORT_ERROR(vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()), "Failed to allocate command buffer");

    pushConstantCmdBuffer = commandBuffers.back();
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


void VulkanBaseApp::createVertexBuffer() {

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

void VulkanBaseApp::createIndexBuffer() {
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

void VulkanBaseApp::createCameraBuffers() {
    cameraBuffers.resize(swapChain.imageCount());
    VkDeviceSize size = sizeof(Camera);


    for(auto i = 0; i < swapChain.imageCount(); i++){
        cameraBuffers[i] = device.createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, size);
    }
}
void VulkanBaseApp::createTextureBuffers() {
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

void VulkanBaseApp::run() {
    init();
    mainLoop();
    cleanup();
}

void VulkanBaseApp::mainLoop() {
    while(isRunning()){
        recenter();
        glfwPollEvents();

        checkSystemInputs();

        if(!paused) {
            checkAppInputs();
            update(getTime());
            drawFrame();
        }else{
            glfwSetTime(elapsedTime);
            onPause();
        }
    }

    vkDeviceWaitIdle(device);
}

void VulkanBaseApp::checkSystemInputs() {
    if(exit->isPressed()){
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
    if(pause->isPressed()){
        setPaused(!paused);
    }
}

void VulkanBaseApp::createDebugMessenger() {
    vulkanDebug = VulkanDebug{ instance };
}

std::vector<char> VulkanBaseApp::loadFile(const std::string& path) {
    std::ifstream fin(path.data(), std::ios::binary | std::ios::ate);
    if(!fin.good()) throw std::runtime_error{"Failed to open file: " + path};

    auto size = fin.tellg();
    fin.seekg(0);
    std::vector<char> data(size);
    fin.read(data.data(), size);

    return data;
}

void VulkanBaseApp::createFramebuffer() {
    assert(renderPass != VK_NULL_HANDLE);

    framebuffers.resize(swapChain.imageCount());
    framebuffers.resize(swapChain.imageCount());
    for(int i = 0; i < framebuffers.size(); i++){
        framebuffers[i] = device.createFramebuffer(renderPass, {swapChain.imageViews[i] }
                                               , static_cast<uint32_t>(width), static_cast<uint32_t>(height) );
    }
}

void VulkanBaseApp::createGraphicsPipeline() {
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

void VulkanBaseApp::createRenderPass() {

    VkAttachmentDescription attachmentDesc{};
    attachmentDesc.format = swapChain.format;
    attachmentDesc.samples = VK_SAMPLE_COUNT_1_BIT;
    attachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachmentDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachmentDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachmentDesc.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference ref{};
    ref.attachment = 0;
    ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpassDesc{};
    subpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDesc.colorAttachmentCount = 1;
    subpassDesc.pColorAttachments = &ref;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    renderPass = device.createRenderPass({attachmentDesc}, {subpassDesc }, {dependency});
}

void VulkanBaseApp::createSyncObjects() {
    imageAcquired.resize(MAX_IN_FLIGHT_FRAMES);
    renderingFinished.resize(MAX_IN_FLIGHT_FRAMES);
    inFlightFences.resize(MAX_IN_FLIGHT_FRAMES);
    inFlightImages.resize(swapChain.imageCount());

    VkSemaphoreCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    for(auto i = 0; i < MAX_IN_FLIGHT_FRAMES; i++){
        imageAcquired[i] = device.createSemaphore();
        renderingFinished[i] = device.createSemaphore();
        inFlightFences[i] = device.createFence();
    }
}

void VulkanBaseApp::drawFrame() {
    inFlightFences[currentFrame].wait();
    auto imageIndex = swapChain.acquireNextImage(imageAcquired[currentFrame]);
    if(swapChain.isOutOfDate()) {
        recreateSwapChain();
        return;
    }

    currentImageIndex = imageIndex;

    if(inFlightImages[imageIndex]){
        inFlightImages[imageIndex]->wait();
    }
    inFlightImages[imageIndex] = &inFlightFences[currentFrame];

    VkPipelineStageFlags flags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    auto commandBuffers = buildCommandBuffers(imageIndex);
    if(commandBuffers.empty()){
        commandBuffers.push_back(this->commandBuffers[imageIndex]);
    }

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &imageAcquired[currentFrame].handle;
    submitInfo.pWaitDstStageMask = &flags;
    submitInfo.commandBufferCount = COUNT(commandBuffers);
    submitInfo.pCommandBuffers = commandBuffers.data();
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &renderingFinished[currentFrame].handle;

    inFlightFences[currentFrame].reset();

    ASSERT(vkQueueSubmit(device.queues.graphics, 1, &submitInfo, inFlightFences[currentFrame]));

    swapChain.present(imageIndex, { renderingFinished[currentFrame] });
    if(swapChain.isSubOptimal() || swapChain.isOutOfDate() || resized) {
        resized = false;
        recreateSwapChain();
        return;
    }
    currentFrame = (currentFrame + 1)%MAX_IN_FLIGHT_FRAMES;
}

void VulkanBaseApp::recreateSwapChain() {
    int width, height;
    do{
        glfwGetFramebufferSize(window, &width, &height);
        glfwWaitEvents();
    }while(width == 0 && height == 0);

    vkDeviceWaitIdle(device);
    cleanupSwapChain();

    createSwapChain();
    createCameraBuffers();
    onSwapChainRecreation();

    createRenderPass();
    createFramebuffer();

    createCameraDescriptorSetLayout();
    createCameraDescriptorPool();
    createDescriptorSet();

    createDescriptorSetLayout();
    createDescriptorPool();
    createDescriptorSet();

    createGraphicsPipeline();
    createCommandBuffer();
}

void VulkanBaseApp::createDescriptorSetLayout() {
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

void VulkanBaseApp::createCameraDescriptorSetLayout() {
    VkDescriptorSetLayoutBinding uniformBinding{};
    uniformBinding.binding = 0;
    uniformBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uniformBinding.descriptorCount = 1;
    uniformBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    std::vector<VkDescriptorSetLayoutBinding> bindings { uniformBinding };

    VkDescriptorSetLayoutCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    createInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    createInfo.pBindings = bindings.data();

    cameraDescriptorSetLayout = device.createDescriptorSetLayout(bindings);
}

void VulkanBaseApp::createDescriptorPool() {
  //  const auto swapChainImageCount = static_cast<uint32_t>(swapChain.imageCount());


    VkDescriptorPoolSize  texturePoolSize{};
    texturePoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    texturePoolSize.descriptorCount = 1;

    std::vector<VkDescriptorPoolSize> poolSizes{
            texturePoolSize
    };

   descriptorPool = device.createDescriptorPool(swapChain.imageCount(), poolSizes);
}

void VulkanBaseApp::createCameraDescriptorPool() {
    const auto swapChainImageCount = static_cast<uint32_t>(swapChain.imageCount());

    VkDescriptorPoolSize uniformPoolSize{};
    uniformPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uniformPoolSize.descriptorCount = swapChainImageCount;

    std::vector<VkDescriptorPoolSize> poolSizes{ uniformPoolSize };

    cameraDescriptorPool = device.createDescriptorPool(swapChain.imageCount(), poolSizes);
}

void VulkanBaseApp::createCameraDescriptorSet() {
    const auto swapChainImageCount = static_cast<uint32_t>(swapChain.imageCount());
    cameraDescriptorSets.resize(swapChainImageCount);

    std::vector<VkDescriptorSetLayout> layouts(swapChainImageCount, cameraDescriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = cameraDescriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(layouts.size());
    allocInfo.pSetLayouts = layouts.data();
    vkAllocateDescriptorSets(device, &allocInfo, cameraDescriptorSets.data());

    for(int i = 0; i < swapChainImageCount; i++) {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = cameraBuffers[i].buffer;
        bufferInfo.offset = 0;
        bufferInfo.range = VK_WHOLE_SIZE;

        VkWriteDescriptorSet cameraWrites{};
        cameraWrites.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        cameraWrites.dstSet = cameraDescriptorSets[i];
        cameraWrites.dstBinding = 0;
        cameraWrites.descriptorCount = 1;
        cameraWrites.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        cameraWrites.pBufferInfo = &bufferInfo;

        std::vector<VkWriteDescriptorSet> writes{ cameraWrites };

        vkUpdateDescriptorSets(device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
    }
}

void VulkanBaseApp::createDescriptorSet() {
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

void VulkanBaseApp::update(float time) {
//    spdlog::info("time: {}", time);
//    camera.model = glm::rotate(glm::mat4(1.0f), elapsedTime * glm::half_pi<float>(), {0.0f, 0.0f, 1.0f});
//    camera.view = glm::lookAt({2.0f, 2.0f, 2.0f}, glm::vec3(0.0f), {0.0f, 0.0f, 1.0f});
//    camera.proj = glm::perspective(glm::quarter_pi<float>(), swapChain.extent.width/ static_cast<float>(swapChain.extent.height), 0.1f, 10.0f);
    cameraController->update(time);
//    camera.proj[1][1] *= 1;
//    camera.model = glm::mat4(1);
//    camera.view = glm::lookAt({0, 0, 2}, glm::vec3(0), {0, 1, 0});
    auto& buffer = cameraBuffers[currentImageIndex];
    buffer.copy(&camera, sizeof(camera));
}

void VulkanBaseApp::cleanupSwapChain() {
    commandPool.free(commandBuffers);
    dispose(pipeline);
    dispose(layout);
    onSwapChainDispose();
    dispose(cameraDescriptorPool);
    dispose(cameraDescriptorSetLayout);
    dispose(descriptorPool);
    dispose(descriptorSetLayout);

    for(auto& framebuffer : framebuffers){
        dispose(framebuffer);
    }
    dispose(renderPass);


    dispose(swapChain);
    for(auto& cameraBuffer : cameraBuffers){
        dispose(cameraBuffer);
    }

}

void VulkanBaseApp::setPaused(bool flag) {
    if(paused != flag){
        paused = flag;
        resetAllActions();
    }
}

float VulkanBaseApp::getTime() {
    auto now = static_cast<float>(glfwGetTime());
    auto dt = now - elapsedTime;
    elapsedTime += dt;
    auto timeFreq = glfwGetTimerFrequency();
    auto timeValue = glfwGetTimerValue();
 //   spdlog::info("elapsedTime: {}, time freq: {}, time value: {}", now, timeFreq, timeValue);
    return dt;
}
