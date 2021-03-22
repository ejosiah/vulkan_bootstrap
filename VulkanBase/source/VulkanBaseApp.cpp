//
// Created by Josiah on 1/17/2021.
//

#include <set>
#include <chrono>
#include <fstream>
#include "VulkanBaseApp.h"
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
 //   cameraController = new OrbitingCameraController{ camera, *this, settings};
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

    createDepthBuffer();
    createRenderPass();
    createFramebuffer();


//    createCameraDescriptorPool();
//    createCameraDescriptorSetLayout();
//    createCameraDescriptorSet();


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

void VulkanBaseApp::createDepthBuffer() {
    if(depthTestEnabled) {
        auto format = findDepthFormat();
        VkImageCreateInfo createInfo = initializers::imageCreateInfo(
                VK_IMAGE_TYPE_2D,
                format,
                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                swapChain.extent.width,
                swapChain.extent.height);
//        createInfo.imageType = VK_IMAGE_TYPE_2D;
//        createInfo.format = format;
//        createInfo.extent = {swapChain.extent.width, swapChain.extent.height, 1};
//        createInfo.samples = VK_SAMPLE_COUNT_1_BIT;
//        createInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
//        createInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

        depthBuffer.image = device.createImage(createInfo, VMA_MEMORY_USAGE_GPU_ONLY);

        VkImageSubresourceRange subresourceRange = initializers::imageSubresourceRange(VK_IMAGE_ASPECT_DEPTH_BIT);
        depthBuffer.imageView = depthBuffer.image.createView(format, VK_IMAGE_VIEW_TYPE_2D, subresourceRange);
    }
}

VkFormat VulkanBaseApp::findDepthFormat() {
    auto possibleFormat = device.findSupportedFormat(depthFormats.formats, depthFormats.tiling, depthFormats.features);
    if(!possibleFormat.has_value()){
        throw std::runtime_error{"Failed to find a suitable depth format"};
    }
    spdlog::info("App will be using depth buffer with: format: {}", *possibleFormat);
    return *possibleFormat;
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


//void VulkanBaseApp::createCameraBuffers() {
//    cameraBuffers.resize(swapChain.imageCount());
//    VkDeviceSize size = sizeof(Camera);
//
//
//    for(auto i = 0; i < swapChain.imageCount(); i++){
//        cameraBuffers[i] = device.createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, size);
//    }
//}


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
    assert(renderPass.renderPass != VK_NULL_HANDLE);

    framebuffers.resize(swapChain.imageCount());
    framebuffers.resize(swapChain.imageCount());
    for(int i = 0; i < framebuffers.size(); i++){
        std::vector<VkImageView> attachments{ swapChain.imageViews[i]};
        if(depthTestEnabled){
            assert(depthBuffer.imageView.handle != VK_NULL_HANDLE);
            attachments.push_back(depthBuffer.imageView);
        }
        framebuffers[i] = device.createFramebuffer(renderPass, attachments
                                               , static_cast<uint32_t>(width), static_cast<uint32_t>(height) );
    }
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

    std::vector<VkAttachmentDescription> attachments{ attachmentDesc };
    if(depthTestEnabled){
        VkAttachmentDescription depthAttachment{};
        depthAttachment.format = depthBuffer.image.format;
        depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        attachments.push_back(depthAttachment);

        VkAttachmentReference depthRef{};
        depthRef.attachment = 1;
        depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        subpassDesc.pDepthStencilAttachment = &depthRef;
    }


    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    renderPass = device.createRenderPass(attachments, {subpassDesc }, {dependency});
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

    uint32_t commandBufferCount;
    auto commandBuffers = buildCommandBuffers(imageIndex, commandBufferCount);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &imageAcquired[currentFrame].handle;
    submitInfo.pWaitDstStageMask = &flags;
    submitInfo.commandBufferCount = commandBufferCount;
    submitInfo.pCommandBuffers = commandBuffers;
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
  //  createCameraBuffers();

    if(depthTestEnabled){
        createDepthBuffer();
    }
    createRenderPass();
    createFramebuffer();

//    createCameraDescriptorSetLayout();
//    createCameraDescriptorPool();
//    createCameraDescriptorSet();
    onSwapChainRecreation();
//    cameraController->perspective(static_cast<float>(swapChain.extent.width)/static_cast<float>(swapChain.extent.height));
}

//void VulkanBaseApp::createCameraDescriptorSetLayout() {
//    VkDescriptorSetLayoutBinding uniformBinding{};
//    uniformBinding.binding = 0;
//    uniformBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
//    uniformBinding.descriptorCount = 1;
//    uniformBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
//
//    std::vector<VkDescriptorSetLayoutBinding> bindings { uniformBinding };
//
//    VkDescriptorSetLayoutCreateInfo createInfo{};
//    createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
//    createInfo.bindingCount = static_cast<uint32_t>(bindings.size());
//    createInfo.pBindings = bindings.data();
//
//    cameraDescriptorSetLayout = device.createDescriptorSetLayout(bindings);
//}
//
//void VulkanBaseApp::createCameraDescriptorPool() {
//    const auto swapChainImageCount = static_cast<uint32_t>(swapChain.imageCount());
//
//    VkDescriptorPoolSize uniformPoolSize{};
//    uniformPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
//    uniformPoolSize.descriptorCount = swapChainImageCount;
//
//    std::vector<VkDescriptorPoolSize> poolSizes{ uniformPoolSize };
//
//    cameraDescriptorPool = device.createDescriptorPool(swapChain.imageCount(), poolSizes);
//}
//
//void VulkanBaseApp::createCameraDescriptorSet() {
//    const auto swapChainImageCount = static_cast<uint32_t>(swapChain.imageCount());
//    cameraDescriptorSets.resize(swapChainImageCount);
//
//    std::vector<VkDescriptorSetLayout> layouts(swapChainImageCount, cameraDescriptorSetLayout);
//    VkDescriptorSetAllocateInfo allocInfo{};
//    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
//    allocInfo.descriptorPool = cameraDescriptorPool;
//    allocInfo.descriptorSetCount = static_cast<uint32_t>(layouts.size());
//    allocInfo.pSetLayouts = layouts.data();
//    vkAllocateDescriptorSets(device, &allocInfo, cameraDescriptorSets.data());
//
//    for(int i = 0; i < swapChainImageCount; i++) {
//        VkDescriptorBufferInfo bufferInfo{};
//        bufferInfo.buffer = cameraBuffers[i].buffer;
//        bufferInfo.offset = 0;
//        bufferInfo.range = VK_WHOLE_SIZE;
//
//        VkWriteDescriptorSet cameraWrites{};
//        cameraWrites.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
//        cameraWrites.dstSet = cameraDescriptorSets[i];
//        cameraWrites.dstBinding = 0;
//        cameraWrites.descriptorCount = 1;
//        cameraWrites.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
//        cameraWrites.pBufferInfo = &bufferInfo;
//
//        std::vector<VkWriteDescriptorSet> writes{ cameraWrites };
//
//        vkUpdateDescriptorSets(device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
//    }
//}


void VulkanBaseApp::update(float time) {
//    spdlog::info("time: {}", time);
//    camera.model = glm::rotate(glm::mat4(1.0f), elapsedTime * glm::half_pi<float>(), {0.0f, 0.0f, 1.0f});
//    camera.view = glm::lookAt({2.0f, 2.0f, 2.0f}, glm::vec3(0.0f), {0.0f, 0.0f, 1.0f});
//    camera.proj = glm::perspective(glm::quarter_pi<float>(), swapChain.extent.width/ static_cast<float>(swapChain.extent.height), 0.1f, 10.0f);
//    cameraController->update(time);
//    camera.proj[1][1] *= 1;
//    camera.model = glm::mat4(1);
//    camera.view = glm::lookAt({0, 0, 2}, glm::vec3(0), {0, 1, 0});
//    auto& buffer = cameraBuffers[currentImageIndex];
//    buffer.copy(&camera, sizeof(camera));
}

void VulkanBaseApp::cleanupSwapChain() {

    onSwapChainDispose();
//    dispose(cameraDescriptorPool);
//    dispose(cameraDescriptorSetLayout);


    for(auto& framebuffer : framebuffers){
        dispose(framebuffer);
    }
    dispose(renderPass);

    if(depthTestEnabled){
        dispose(depthBuffer.imageView);
        dispose(depthBuffer.image);
    }

    dispose(swapChain);
//    for(auto& cameraBuffer : cameraBuffers){
//        dispose(cameraBuffer);
//    }

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

VkCommandBuffer *VulkanBaseApp::buildCommandBuffers(uint32_t imageIndex, uint32_t &numCommandBuffers) {
    return nullptr;
}

void VulkanBaseApp::onSwapChainRecreation() {

}

void VulkanBaseApp::onSwapChainDispose() {

}

inline void VulkanBaseApp::checkAppInputs() {
//    cameraController->processInput();
}

inline bool VulkanBaseApp::isRunning() const {
    return !glfwWindowShouldClose(window);
}

void VulkanBaseApp::cleanup() {

}

void VulkanBaseApp::onPause() {

}


