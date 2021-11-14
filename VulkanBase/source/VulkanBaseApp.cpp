//
// Created by Josiah on 1/17/2021.
//
#ifndef VMA_IMPLEMENTATION
#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"
#endif
#include <set>
#include <chrono>
#include <fstream>
#include <VulkanShaderModule.h>

#include "VulkanBaseApp.h"
#include "keys.h"
#include "events.h"
#include "VulkanInitializers.h"
#include "Plugin.hpp"
#include "VulkanRayQuerySupport.hpp"

namespace chrono = std::chrono;

VulkanBaseApp::VulkanBaseApp(std::string_view name, const Settings& settings, std::vector<std::unique_ptr<Plugin>> plugins)
        : Window(name, settings.width, settings.height, settings.fullscreen)
        , InputManager(settings.relativeMouseMode)
        , enabledFeatures(settings.enabledFeatures)
        , settings(settings)
        , plugins(std::move(plugins))
{
    appInstance = this;
    this->settings.deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
}

VulkanBaseApp::~VulkanBaseApp(){
    ready = false;
    appInstance = nullptr;
}

void VulkanBaseApp::init() {
    checkInstanceExtensionSupport();
    initWindow();
    initInputMgr(*this);
    exit = &mapToKey(Key::ESCAPE, "Exit", Action::detectInitialPressOnly());
    pause = &mapToKey(Key::P, "Pause", Action::detectInitialPressOnly());
    addPluginExtensions();
    initVulkan();
    postVulkanInit();
    initPlugins();
    initApp();
    ready = true;
}

void VulkanBaseApp::initMixins() {
    if(auto rayQuery = dynamic_cast<VulkanRayQuerySupport*>(this)){
        rayQuery->enableRayQuery();
    }
}

void VulkanBaseApp::initWindow() {
    Window::initWindow();
    uint32_t size;
    auto extensions = glfwGetRequiredInstanceExtensions(&size);
    instanceExtensions = std::vector<const char*>(extensions, extensions + size);

    for(auto& extension : settings.instanceExtensions){
        instanceExtensions.push_back(extension);
    }

    for(auto& extension : settings.deviceExtensions){
        deviceExtensions.push_back(extension);
    }

    for(auto& layer : settings.validationLayers){
        validationLayers.push_back(layer);
    }

    if(enableValidation){
        instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        instanceExtensions.push_back(VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME);
        validationLayers.push_back("VK_LAYER_KHRONOS_validation");
    }
}

void VulkanBaseApp::initVulkan() {
    createInstance();
    this->ext = VulkanExtensions{instance};
    ext::init(instance);
    createDebugMessenger();
    pickPhysicalDevice();
    initMixins();
    createLogicalDevice();
    createSwapChain();

    createColorBuffer();
    createDepthBuffer();
    createRenderPass();
    createFramebuffer();

    createSyncObjects();
}

void VulkanBaseApp::postVulkanInit() {
}

void VulkanBaseApp::addPluginExtensions() {
    for(auto& plugin : plugins){
        for(auto extension : plugin->instanceExtensions()){
            instanceExtensions.push_back(extension);
        }
        for(auto layer : plugin->validationLayers()){
            validationLayers.push_back(layer);
        }
        for(auto extension : plugin->deviceExtensions()){
            deviceExtensions.push_back(extension);
        }
    }
}

void VulkanBaseApp::initPlugins() {

    for(auto& plugin : plugins){
        spdlog::info("initializing plugin: {}", plugin->name());
        plugin->set({ &instance, &device, &renderPass, &swapChain, window, &currentImageIndex, settings.msaaSamples});
        plugin->init();
        registerPluginEventListeners(plugin.get());
    }
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
    glfwGetFramebufferSize(window, &width, &height);    // TODO use settings and remove width/height
    settings.width = width;
    settings.height = height;
    swapChain = VulkanSwapChain{ device, surface, settings};
    swapChainImageCount = swapChain.imageCount();
}

void VulkanBaseApp::createDepthBuffer() {
    if(!settings.depthTest) return;

    auto format = findDepthFormat();
    VkImageCreateInfo createInfo = initializers::imageCreateInfo(
            VK_IMAGE_TYPE_2D,
            format,
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            swapChain.extent.width,
            swapChain.extent.height);
    createInfo.samples = settings.msaaSamples;

    depthBuffer.image = device.createImage(createInfo, VMA_MEMORY_USAGE_GPU_ONLY);

    VkImageSubresourceRange subresourceRange = initializers::imageSubresourceRange(VK_IMAGE_ASPECT_DEPTH_BIT);
    depthBuffer.imageView = depthBuffer.image.createView(format, VK_IMAGE_VIEW_TYPE_2D, subresourceRange);
}

void VulkanBaseApp::createColorBuffer(){
    if(settings.msaaSamples == VK_SAMPLE_COUNT_1_BIT) return;

    VkImageCreateInfo createInfo = initializers::imageCreateInfo(
            VK_IMAGE_TYPE_2D,
            swapChain.format,
            VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            swapChain.extent.width,
            swapChain.extent.height);
    createInfo.samples = settings.msaaSamples;

    colorBuffer.image = device.createImage(createInfo, VMA_MEMORY_USAGE_GPU_ONLY);
    VkImageSubresourceRange subresourceRange = initializers::imageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);
    colorBuffer.imageView = colorBuffer.image.createView(swapChain.format, VK_IMAGE_VIEW_TYPE_2D, subresourceRange);
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
    device.createLogicalDevice(enabledFeatures, deviceExtensions, validationLayers, surface, settings.queueFlags, deviceCreateNextChain);
}

void VulkanBaseApp::pickPhysicalDevice() {
    surface = VulkanSurface{instance, window};
    auto pDevices = enumerate<VkPhysicalDevice>([&](uint32_t* size, VkPhysicalDevice* pDevice){
        return vkEnumeratePhysicalDevices(instance, size, pDevice);
    });

    std::vector<VulkanDevice> devices(pDevices.size());
    std::transform(begin(pDevices), end(pDevices), begin(devices),[&](auto pDevice){
        return VulkanDevice{instance, pDevice, settings};
    });

    std::sort(begin(devices), end(devices), [](auto& a, auto& b){
        return a.score() > b.score();
    });

    device = std::move(devices.front());
    settings.msaaSamples = std::min(settings.msaaSamples, device.getMaxUsableSampleCount());
    checkDeviceExtensionSupport();
    spdlog::info("selected device: {}", device.name());
}

void VulkanBaseApp::addPlugin(std::unique_ptr<Plugin>& plugin) {
    plugins.push_back(std::move(plugin));
}

void VulkanBaseApp::run() {
    init();
    mainLoop();
    cleanupPlugins();
    cleanup();
}

void VulkanBaseApp::mainLoop() {
    while(isRunning()){
        recenter();
        glfwPollEvents();
        fullscreenCheck();

        if(swapChainInvalidated){
            swapChainInvalidated = false;
            recreateSwapChain();
        }

        checkSystemInputs();
        if(!isRunning()) break;

        if(!paused) {
            checkAppInputs();
            notifyPluginsOfNewFrameStart();
            newFrame();
            drawFrame();
            presentFrame();
            notifyPluginsOfEndFrame();
            endFrame();
            nextFrame();
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

void VulkanBaseApp::createFramebuffer() {
    assert(renderPass.renderPass != VK_NULL_HANDLE);

    framebuffers.resize(swapChain.imageCount());
    for(int i = 0; i < framebuffers.size(); i++){
        std::vector<VkImageView> attachments{ swapChain.imageViews[i]};
        if(settings.depthTest){
            assert(depthBuffer.imageView.handle != VK_NULL_HANDLE);
            attachments.push_back(depthBuffer.imageView);
        }
        if(settings.msaaSamples != VK_SAMPLE_COUNT_1_BIT){
            attachments.push_back(colorBuffer.imageView);
            std::swap(attachments[0], attachments[attachments.size() - 1]);
        }
        framebuffers[i] = device.createFramebuffer(renderPass, attachments
                                               , static_cast<uint32_t>(width), static_cast<uint32_t>(height) );
    }
}


void VulkanBaseApp::createRenderPass() {
    auto [attachments, subpassDesc, dependency] = buildRenderPass();
    renderPass = device.createRenderPass(attachments, subpassDesc, dependency);
}

RenderPassInfo VulkanBaseApp::buildRenderPass() {
    bool msaaEnabled = settings.msaaSamples != VK_SAMPLE_COUNT_1_BIT;
    VkAttachmentDescription attachmentDesc{};
    attachmentDesc.format = swapChain.format;
    attachmentDesc.samples = settings.msaaSamples;
    attachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachmentDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachmentDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachmentDesc.finalLayout = msaaEnabled ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;


    VkAttachmentReference ref{};
    ref.attachment = 0;
    ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;


    SubpassDescription subpassDesc{};
    subpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDesc.colorAttachments.push_back(ref);

    std::vector<VkAttachmentDescription> attachments{ attachmentDesc };
    if(settings.depthTest){
        VkAttachmentDescription depthAttachment{};
        depthAttachment.format = depthBuffer.image.format;
        depthAttachment.samples = settings.msaaSamples;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        attachments.push_back(depthAttachment);

        VkAttachmentReference depthRef{};
        depthRef.attachment = attachments.size() - 1;
        depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        subpassDesc.depthStencilAttachments = depthRef;
    }

    if(msaaEnabled){
        VkAttachmentDescription colorAttachmentResolve{};
        colorAttachmentResolve.format = swapChain.format;
        colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        attachments.push_back(colorAttachmentResolve);

        VkAttachmentReference resolveRef{};
        resolveRef.attachment = attachments.size() - 1;
        resolveRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        subpassDesc.resolveAttachments.push_back(resolveRef);
    }


    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    std::vector<SubpassDescription> subpassDescs{ subpassDesc };
    std::vector<VkSubpassDependency> dependencies{ dependency };

    return std::make_tuple(attachments, subpassDescs, dependencies);
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
    if(swapChainInvalidated) return;
    frameCount++;
    inFlightFences[currentFrame].wait();
    auto imageIndex = swapChain.acquireNextImage(imageAcquired[currentFrame]);
    if(swapChain.isOutOfDate()) {
        swapChainInvalidated = true;
    //    recreateSwapChain();
        return;
    }

    currentImageIndex = imageIndex;

    if(inFlightImages[imageIndex]){
        inFlightImages[imageIndex]->wait();
    }
    inFlightImages[imageIndex] = &inFlightFences[currentFrame];

    auto time = getTime();
    updatePlugins(time);
    update(time);
    calculateFPS(time);

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
}

void VulkanBaseApp::presentFrame() {
    if(swapChainInvalidated) return;

    swapChain.present(currentImageIndex, { renderingFinished[currentFrame] });
    if(swapChain.isSubOptimal() || swapChain.isOutOfDate() || resized) {
        resized = false;
        swapChainInvalidated = true;
        return;
    }
}

void VulkanBaseApp::nextFrame() {
    currentFrame = (currentFrame + 1)%MAX_IN_FLIGHT_FRAMES;
}

void VulkanBaseApp::calculateFPS(float dt) {
    static float oneSecond = 0.f;

    oneSecond += dt;

    if(oneSecond > 1.0f){
        framePerSecond = frameCount;
        totalFrames += frameCount;
        frameCount = 0;
        oneSecond = 0.f;
    }
  //  framePerSecond = frameCount / elapsedTime;
}

void VulkanBaseApp::recreateSwapChain() {
    do{
        glfwGetFramebufferSize(window, &width, &height);
        glfwWaitEvents();
    }while(width == 0 && height == 0);

    vkDeviceWaitIdle(device);
    cleanupSwapChain();

    createSwapChain();

    if(settings.depthTest){
        createDepthBuffer();
    }
    if(settings.msaaSamples != VK_SAMPLE_COUNT_1_BIT){
        createColorBuffer();
    }
    createRenderPass();
    createFramebuffer();

    notifyPluginsOfSwapChainRecreation();
    onSwapChainRecreation();
}



void VulkanBaseApp::update(float time) {

}

void VulkanBaseApp::cleanupSwapChain() {
    notifyPluginsOfSwapChainDisposal();
    onSwapChainDispose();

    for(auto& framebuffer : framebuffers){
        dispose(framebuffer);
    }
    dispose(renderPass);

    if(settings.depthTest){
        dispose(depthBuffer.image);
        dispose(depthBuffer.imageView);
    }
    if(settings.msaaSamples != VK_SAMPLE_COUNT_1_BIT){
        dispose(colorBuffer.image);
        dispose(colorBuffer.imageView);
    }

    dispose(swapChain);
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
    return dt;
}

void VulkanBaseApp::onSwapChainRecreation() {

}

void VulkanBaseApp::onSwapChainDispose() {

}

inline void VulkanBaseApp::checkAppInputs() {
}

inline bool VulkanBaseApp::isRunning() const {
    return !glfwWindowShouldClose(window);
}

void VulkanBaseApp::cleanup() {

}

void VulkanBaseApp::onPause() {

}

void VulkanBaseApp::newFrame() {

}

void VulkanBaseApp::endFrame() {

}

void VulkanBaseApp::notifyPluginsOfNewFrameStart() {
    for(auto& plugin : plugins){
        plugin->newFrame();
    }
}

void VulkanBaseApp::notifyPluginsOfEndFrame() {
    for(auto& plugin : plugins){
        plugin->endFrame();
    }
}

void VulkanBaseApp::notifyPluginsOfSwapChainDisposal() {
    for(auto& plugin : plugins){
        plugin->onSwapChainDispose();
    }
}

void VulkanBaseApp::notifyPluginsOfSwapChainRecreation() {
    for(auto& plugin : plugins){
        plugin->onSwapChainRecreation();
    }
}

void VulkanBaseApp::cleanupPlugins() {
    for(auto& plugin : plugins){
        plugin->cleanup();
    }
}

void VulkanBaseApp::registerPluginEventListeners(Plugin* plugin) {
    addWindowResizeListeners(plugin->windowResizeListener());
    addMousePressListener(plugin->mousePressListener());
    addMouseReleaseListener(plugin->mouseReleaseListener());
    addMouseClickListener(plugin->mouseClickListener());
    addMouseMoveListener(plugin->mouseMoveListener());
    addMouseWheelMoveListener(plugin->mouseWheelMoveListener());
    addKeyPressListener(plugin->keyPressListener());
    addKeyReleaseListener(plugin->keyReleaseListener());
}

void VulkanBaseApp::updatePlugins(float dt){
    for(auto& plugin : plugins){
        plugin->update(dt);
    }
}

VulkanBaseApp* VulkanBaseApp::appInstance = nullptr;

void VulkanBaseApp::fullscreenCheck() {
    if(toggleFullscreen && !fullscreen){
        toggleFullscreen = false;
        swapChainInvalidated = setFullScreen();

    }else if(toggleFullscreen && fullscreen){
        toggleFullscreen = false;
        swapChainInvalidated = unsetFullScreen();
    }
}

void VulkanBaseApp::checkDeviceExtensionSupport() {
    std::vector<const char*> unsupported;
    for(auto extension : deviceExtensions){
        if(!device.extensionSupported(extension)){
            unsupported.push_back(extension);
        }
    }

    if(!unsupported.empty()){
        throw std::runtime_error{
            fmt::format("Vulkan device [{}] does not support the following extensions {}", device.name(), unsupported) };
    }
}

void VulkanBaseApp::checkInstanceExtensionSupport() {
    auto supportedExtensions = enumerate<VkExtensionProperties>([](auto size, auto properties){
        return vkEnumerateInstanceExtensionProperties("", size, properties);
    });

    std::vector<const char*> unsupported;
    for(auto& extension : instanceExtensions){
        auto supported = std::any_of(begin(supportedExtensions), end(supportedExtensions), [&](auto supported){
            return std::strcmp(extension, supported.extensionName) == 0;
        });
        if(supported){
            unsupported.push_back(extension);
        }
    }

    if(!unsupported.empty()){
        throw std::runtime_error{fmt::format("this Vulkan instance does not support the following extensions {}", unsupported) };
    }
}

byte_string VulkanBaseApp::load(const std::string &resource) {
    return fileLoader.load(resource);
}
