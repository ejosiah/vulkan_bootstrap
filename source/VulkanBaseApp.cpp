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

VulkanBaseApp::VulkanBaseApp(std::string_view name, int width, int height)
: name(name)
, width(width)
, height(height)
{
}


void VulkanBaseApp::init() {
    initWindow();
    initVulkan();
}

void VulkanBaseApp::initWindow() {
    if(!glfwInit()){
        throw std::runtime_error{"Failed to initialize GFLW"};
    }
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWmonitor* monitor = nullptr;
    if(fullscreen) {
        auto mainMonitor = glfwGetPrimaryMonitor();
        int numMonitors;
        GLFWmonitor **monitors = glfwGetMonitors(&numMonitors);
        for (int i = 0; i < numMonitors; i++) {
            if (monitors[i] == mainMonitor) {
                monitor = monitors[i];
                break;
            }
        }
        auto vidMode = glfwGetVideoMode(monitor);
        width = vidMode->width;
        height = vidMode->height;
    }


    window = glfwCreateWindow(width, height, name.data(), monitor, nullptr);
    glfwSetWindowUserPointer(window, this);
    uint32_t size;
    auto extensions = glfwGetRequiredInstanceExtensions(&size);
    instanceExtensions = std::vector<const char*>(extensions, extensions + size);

    if(enableValidation){
        instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        instanceExtensions.push_back(VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME);
        validationLayers.push_back("VK_LAYER_KHRONOS_validation");
    }
    glfwSetKeyCallback(window, onKeyPress);
    glfwSetFramebufferSizeCallback(window, onResize);
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
    appInfo.pApplicationName = name.data();
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
        auto debugInfo = debugCreateInfo();
        createInfo.pNext = &debugInfo;
    }

    REPORT_ERROR(vkCreateInstance(&createInfo, nullptr, &instance), "Failed to create Vulkan instance");
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
    physicalDevice = device.physicalDevice;
    spdlog::info("selected device: {}", device.name());
}

void VulkanBaseApp::createCommandPool() {
    VkCommandPoolCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    createInfo.queueFamilyIndex = device.queueFamilyIndex.graphics.value();

//    REPORT_ERROR(vkCreateCommandPool(device, &createInfo, nullptr, &commandPool), "Failed to create command pool!");
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

        vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &descriptorSets[i], 0, nullptr);
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

template<typename Command>
void VulkanBaseApp::oneTimeCommand(const VkQueue destination, Command&& command) {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer cmdBuffer;
    REPORT_ERROR(vkAllocateCommandBuffers(device, &allocInfo, &cmdBuffer), "Failed to allocate command buffer");

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(cmdBuffer, &beginInfo);
    command(cmdBuffer);
    vkEndCommandBuffer(cmdBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmdBuffer;
    vkQueueSubmit(destination, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(destination);
    vkFreeCommandBuffers(device, commandPool, 1, &cmdBuffer);
}

void VulkanBaseApp::createVertexBuffer() {

    VkDeviceSize size = sizeof(Vertex) * mesh.vertices.size();

    VulkanBuffer stagingBuffer = device.createBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY, size);
    stagingBuffer.copy(mesh.vertices.data(), size);

    vertexBuffer = device.createBuffer(
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VMA_MEMORY_USAGE_GPU_ONLY,
            size);

    oneTimeCommand(device.queues.graphics, [&](VkCommandBuffer cmdBuffer){
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

    oneTimeCommand(device.queues.graphics, [&](VkCommandBuffer cmdBuffer){
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

    VkBufferCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    createInfo.size = size;
    createInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

    for(auto i = 0; i < swapChain.imageCount(); i++){
        REPORT_ERROR(vmaCreateBuffer(memoryAllocator, &createInfo, &allocInfo, &cameraBuffers[i].resource, &cameraBuffers[i].allocation, nullptr), "Failed to create Camera buffer");
    }
}
void VulkanBaseApp::createTextureBuffers() {
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load("../../data/media/vulkan.png", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    VkDeviceSize imageSize = texWidth * texHeight * STBI_rgb_alpha;

    if(!pixels){
        throw std::runtime_error{"failed to load texture image!"};
    }
    Buffer stagingBuffer;

    VkBufferCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    createInfo.size = imageSize;
    createInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
    vmaCreateBuffer(memoryAllocator, &createInfo, &allocInfo, &stagingBuffer.resource, &stagingBuffer.allocation, nullptr);

    void* data;
    vmaMapMemory(memoryAllocator, stagingBuffer.allocation, &data);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    vmaUnmapMemory(memoryAllocator, stagingBuffer.allocation);

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

    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    vmaCreateImage(memoryAllocator, &imageCreateInfo, &allocInfo, &texture.image.resource, &texture.image.allocation, nullptr);

    transitionImageLayout(texture.image.resource, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    oneTimeCommand(device.queues.graphics, [&](auto cmdBuffer){
        VkBufferImageCopy  region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;

        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = { 0, 0, 0};
        region.imageExtent = { static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), 1u};

        vkCmdCopyBufferToImage(cmdBuffer, stagingBuffer.resource, texture.image.resource, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    });
    vmaDestroyBuffer(memoryAllocator, stagingBuffer, stagingBuffer.allocation);
    transitionImageLayout(texture.image.resource, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    VkImageViewCreateInfo imvCreateInfo{};
    imvCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imvCreateInfo.image = texture.image.resource;
    imvCreateInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    imvCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imvCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imvCreateInfo.subresourceRange.baseMipLevel = 0;
    imvCreateInfo.subresourceRange.levelCount = 1;
    imvCreateInfo.subresourceRange.baseArrayLayer = 0;
    imvCreateInfo.subresourceRange.layerCount = 1;

    REPORT_ERROR(vkCreateImageView(device, &imvCreateInfo, nullptr, &texture.imageView), "Failed to create Image view");

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

    auto res = vkCreateSampler(device, &samplerInfo, nullptr, &texture.sampler);
    REPORT_ERROR(res, "Failed to create texture sampler!");
}

void VulkanBaseApp::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout,
                                          VkImageLayout newLayout) {

    oneTimeCommand(device.queues.graphics, [&](VkCommandBuffer commandBuffer) {

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        VkPipelineStageFlags sourceStage;
        VkPipelineStageFlags destinationStage;

        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
                   newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        } else {
            throw std::runtime_error{"unsupported layout transition!"};
        }

        vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    });
}

void VulkanBaseApp::run() {
    init();
    mainLoop();
    cleanup();
}

void VulkanBaseApp::mainLoop() {
    while(!glfwWindowShouldClose(window)){
        glfwPollEvents();
        drawFrame();
    //    vkDeviceWaitIdle(device);
    }

    vkDeviceWaitIdle(device);
}

VkBool32 VulkanBaseApp::debugCallBack(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                      VkDebugUtilsMessageTypeFlagsEXT messageTypes,
                                      const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData) {

    if(messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT){
        spdlog::debug(pCallbackData->pMessage);
    }
    if(messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT){
        spdlog::info(pCallbackData->pMessage);
    }
    if(messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT){
        spdlog::warn(pCallbackData->pMessage);
    }
    if(messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT){
        spdlog::error(pCallbackData->pMessage);
    }

    return VK_FALSE;
}
void VulkanBaseApp::createDebugMessenger() {
    auto debugInfo = debugCreateInfo();
    REPORT_ERROR(ext.createDebugUtilsMessenger(instance, &debugInfo, nullptr, &debugMessenger), "Failed to create Debug Messenger");
}

VkDebugUtilsMessengerCreateInfoEXT VulkanBaseApp::debugCreateInfo() {
    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.pfnUserCallback = debugCallBack;

    return createInfo;
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
        framebuffers[i] = VulkanFramebuffer{device, renderPass, {swapChain.imageViews[i] }
                                               , static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
    }
}

void VulkanBaseApp::createGraphicsPipeline() {
    assert(renderPass != VK_NULL_HANDLE);
    auto vertexShaderModule = ShaderModule{"../../data/shaders/triangle.vert.spv", device};
    auto fragmentShaderModule = ShaderModule{"../../data/shaders/triangle.frag.spv", device};

    std::vector<VkPipelineShaderStageCreateInfo> stages(2);
    stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = vertexShaderModule;
    stages[0].pName= "main";

    stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = fragmentShaderModule;
    stages[1].pName = "main";

    VkPipelineVertexInputStateCreateInfo vertexInputState{};
    vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputState.vertexBindingDescriptionCount = 1;
    auto vertexInputBinding = Vertex::bindingDisc();
    vertexInputState.pVertexBindingDescriptions = &vertexInputBinding;
    auto attributeDesc = Vertex::attributeDisc();
    vertexInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDesc.size());
    vertexInputState.pVertexAttributeDescriptions = attributeDesc.data();

    VkPipelineInputAssemblyStateCreateInfo InputAssemblyState{};
    InputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    InputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    InputAssemblyState.primitiveRestartEnable = VK_FALSE;


    VkViewport viewport{};
    viewport.width = swapChain.extent.width;
    viewport.height = swapChain.extent.height;
    viewport.minDepth = 0;
    viewport.maxDepth = 1;
    viewport.x = 0;
    viewport.y = 0;

    VkRect2D scissor{};
    scissor.extent = swapChain.extent;
    scissor.offset = {0, 0};

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizationState{};
    rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizationState.rasterizerDiscardEnable = VK_FALSE;
    rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizationState.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizationState.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizationState.depthBiasEnable = VK_FALSE;
    rasterizationState.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo multisampleState{};
    multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;


    VkPipelineDepthStencilStateCreateInfo depthStencilState{};
    depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencilState.depthTestEnable = VK_FALSE;
    depthStencilState.depthWriteEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState  blendAttachmentState{};
    blendAttachmentState.blendEnable = VK_FALSE;
    blendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                                               VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
                                               VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo blendState{};
    blendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    blendState.attachmentCount = 1;
    blendState.pAttachments = &blendAttachmentState;

    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = 0;
    dynamicState.pDynamicStates = VK_NULL_HANDLE;

    VkPushConstantRange range;
    range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    range.offset = 0;
    range.size = sizeof(Camera);

    layout = device.createPipelineLayout({descriptorSetLayout }, {range });

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
    //VkPipeline lPipeline;
    //REPORT_ERROR(vkCreateGraphicsPipelines(device, nullptr, 1, &createInfo, nullptr, &lPipeline), "Failed to create pipeline");
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


    VkRenderPassCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    createInfo.attachmentCount = 1;
    createInfo.pAttachments = &attachmentDesc;
    createInfo.subpassCount = 1;
    createInfo.pSubpasses = &subpassDesc;
    createInfo.dependencyCount = 1;
    createInfo.pDependencies = &dependency;

    renderPass = VulkanRenderPass{device, {attachmentDesc}, {subpassDesc }, {dependency}};
//    REPORT_ERROR(vkCreateRenderPass(device, &createInfo, nullptr, &renderPass), "Failed to create Render pass");
}

void VulkanBaseApp::createSyncObjects() {
    imageAcquired.resize(MAX_IN_FLIGHT_FRAMES);
    renderingFinished.resize(MAX_IN_FLIGHT_FRAMES);
    inFlightFences.resize(MAX_IN_FLIGHT_FRAMES);
    inFlightImages.resize(swapChain.imageCount());

    VkSemaphoreCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    for(auto i = 0; i < MAX_IN_FLIGHT_FRAMES; i++){
        REPORT_ERROR(vkCreateSemaphore(device, &createInfo, nullptr, &renderingFinished[i]), "Failed to create rendering finished semaphore");
        REPORT_ERROR(vkCreateSemaphore(device, &createInfo, nullptr, &imageAcquired[i]), "Failed to create image acquired semaphore");

        VkFenceCreateInfo fCreateInfo{};
        fCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        REPORT_ERROR(vkCreateFence(device, &fCreateInfo, nullptr, &inFlightFences[i]), "Failed to create in flight fence");
    }
}

void VulkanBaseApp::drawFrame() {
    vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
    auto imageIndex = swapChain.acquireNextImage(imageAcquired[currentFrame]);
    if(swapChain.isOutOfDate()) {
        recreateSwapChain();
        return;
    }

    currentImageIndex = imageIndex;

    if(inFlightImages[imageIndex]){
        vkWaitForFences(device, 1, &inFlightImages[imageIndex], VK_TRUE, UINT64_MAX);
    }
    inFlightImages[imageIndex] = inFlightFences[currentFrame];
    update(currentTime());
    VkPipelineStageFlags flags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &imageAcquired[currentFrame];
    submitInfo.pWaitDstStageMask = &flags;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers[imageIndex];
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &renderingFinished[currentFrame];

    vkResetFences(device, 1, &inFlightFences[currentFrame]);

    REPORT_ERROR(vkQueueSubmit(device.queues.graphics, 1, &submitInfo, inFlightFences[currentFrame]), "Failed to submit command");

//    VkPresentInfoKHR presentInfo{};
//    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
//    presentInfo.waitSemaphoreCount = 1;
//    presentInfo.pWaitSemaphores = &renderingFinished[currentFrame];
//    presentInfo.swapchainCount = 1;
//    presentInfo.pSwapchains = swapChain;
//    presentInfo.pImageIndices = &imageIndex;
//
//   res =  vkQueuePresentKHR(device.queues.present, &presentInfo);
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
    createRenderPass();
    createFramebuffer();
    createDescriptorSetLayout();
    createDescriptorPool();
    createDescriptorSet();
    createGraphicsPipeline();
    createCommandBuffer();
}

void VulkanBaseApp::createDescriptorSetLayout() {
    VkDescriptorSetLayoutBinding uniformBinding{};
    uniformBinding.binding = 0;
    uniformBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uniformBinding.descriptorCount = 1;
    uniformBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;


    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding = 1;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.pImmutableSamplers = nullptr;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    std::vector<VkDescriptorSetLayoutBinding> bindings {
        uniformBinding, samplerLayoutBinding
    };

    VkDescriptorSetLayoutCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    createInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    createInfo.pBindings = bindings.data();

    descriptorSetLayout = device.createDescriptorSetLayout(bindings);
}

void VulkanBaseApp::createDescriptorPool() {
    const auto swapChainImageCount = static_cast<uint32_t>(swapChain.imageCount());
    descriptorSets.resize(swapChainImageCount);

    VkDescriptorPoolSize uniformPoolSize{};
    uniformPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uniformPoolSize.descriptorCount = swapChainImageCount;

    VkDescriptorPoolSize  texturePoolSize{};
    texturePoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    texturePoolSize.descriptorCount = swapChainImageCount;

    std::vector<VkDescriptorPoolSize> poolSizes{
            uniformPoolSize, texturePoolSize
    };

   descriptorPool = device.createDescriptorPool(swapChain.imageCount(), poolSizes);
}

void VulkanBaseApp::createDescriptorSet() {
    const auto swapChainImageCount = static_cast<uint32_t>(swapChain.imageCount());
    descriptorSets.resize(swapChainImageCount);

    std::vector<VkDescriptorSetLayout> layouts(swapChainImageCount, descriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(layouts.size());
    allocInfo.pSetLayouts = layouts.data();
    vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data());

    for(int i = 0; i < swapChainImageCount; i++) {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = cameraBuffers[i].resource;
        bufferInfo.offset = 0;
        bufferInfo.range = VK_WHOLE_SIZE;

        VkWriteDescriptorSet cameraWrites{};
        cameraWrites.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        cameraWrites.dstSet = descriptorSets[i];
        cameraWrites.dstBinding = 0;
        cameraWrites.descriptorCount = 1;
        cameraWrites.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        cameraWrites.pBufferInfo = &bufferInfo;

        VkWriteDescriptorSet textureWrites{};
        textureWrites.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        textureWrites.dstSet = descriptorSets[i];
        textureWrites.dstBinding = 1;
        textureWrites.dstArrayElement = 0;
        textureWrites.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        textureWrites.descriptorCount = 1;

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = texture.imageView;
        imageInfo.sampler = texture.sampler;
        textureWrites.pImageInfo = &imageInfo;

        std::vector<VkWriteDescriptorSet> writes{
            cameraWrites, textureWrites,
        };

        vkUpdateDescriptorSets(device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
    }
}

void VulkanBaseApp::update(float time) {
    camera.model = glm::rotate(glm::mat4(1.0f), time * glm::half_pi<float>(), {0.0f, 0.0f, 1.0f});
    camera.view = glm::lookAt({2.0f, 2.0f, 2.0f}, glm::vec3(0.0f), {0.0f, 0.0f, 1.0f});
    camera.proj = glm::perspective(glm::quarter_pi<float>(), swapChain.extent.width/ static_cast<float>(swapChain.extent.height), 0.1f, 10.0f);
    //camera.proj[1][1] *= 1;
    //sendPushConstants();
    auto& buffer = cameraBuffers[currentImageIndex];

    void* data;
    vmaMapMemory(memoryAllocator, buffer.allocation, &data);
    memcpy(data, &camera, sizeof(camera));
    vmaUnmapMemory(memoryAllocator, buffer.allocation);
}

float VulkanBaseApp::currentTime() {
    static chrono::high_resolution_clock::time_point current = chrono::high_resolution_clock::now();
    auto now = chrono::high_resolution_clock::now();
    auto diff = now - current;
    return chrono::duration_cast<chrono::milliseconds>(diff).count()/1000.f;
}

void VulkanBaseApp::cleanupSwapChain() {
    commandPool.free(commandBuffers);
    dispose(pipeline);
    dispose(layout);
    dispose(descriptorPool);
    dispose(descriptorSetLayout);
    for(auto& framebuffer : framebuffers) dispose(framebuffer);
    dispose(renderPass);

    dispose(swapChain);
    for(auto& cameraBuffer : cameraBuffers){
        vmaDestroyBuffer(memoryAllocator, cameraBuffer, cameraBuffer.allocation);
    }

}
void VulkanBaseApp::cleanup() {
    cleanupSwapChain();
    vkDestroySurfaceKHR(instance,surface, nullptr);
    vkDestroyCommandPool(device, commandPool, nullptr);

    vmaDestroyBuffer(memoryAllocator, vertexBuffer, vertexBuffer.allocation);
    vmaDestroyBuffer(memoryAllocator, indexBuffer, indexBuffer.allocation);

    vmaDestroyImage(memoryAllocator, texture.image, texture.image.allocation);
    vkDestroyImageView(device, texture.imageView, nullptr);
    vkDestroySampler(device, texture.sampler, nullptr);

    for(int i = 0; i < MAX_IN_FLIGHT_FRAMES; i++) {
        vkDestroySemaphore(device, imageAcquired[i], nullptr);
        vkDestroySemaphore(device, renderingFinished[i], nullptr);
        vkDestroyFence(device, inFlightFences[i], nullptr);
    }

    vmaDestroyAllocator(memoryAllocator);
    vkDestroyDevice(device, nullptr);
    ext.destroyDebugUtilsMessenger(instance, debugMessenger, nullptr);
    vkDestroyInstance(instance, nullptr);
    glfwDestroyWindow(window);
    glfwTerminate();
}

void VulkanBaseApp::sendPushConstants() {
    unsigned char mvp[192];
    memcpy(mvp, &camera, sizeof(Camera));

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vkBeginCommandBuffer(pushConstantCmdBuffer, &beginInfo);
    vkCmdPushConstants(pushConstantCmdBuffer, layout, VK_SHADER_STAGE_VERTEX_BIT, 0, 192u, mvp);
    vkEndCommandBuffer(pushConstantCmdBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &pushConstantCmdBuffer;

    vkQueueSubmit(device.queues.graphics, 1, &submitInfo, VK_NULL_HANDLE);
}
