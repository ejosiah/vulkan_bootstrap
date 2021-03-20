#pragma once

#include <vulkan/vulkan.h>
#include <string_view>
#include <vector>
#include <GLFW/glfw3.h>
#include <stdexcept>
#include <spdlog/spdlog.h>
#include "VulkanExtensions.h"
#include <optional>
#include "container_operations.h"
#include "primitives.h"
#include "vk_mem_alloc.h"
#include "VulkanDevice.h"
#include "VulkanBuffer.h"
#include "VulkanSurface.h"
#include "VulkanSwapChain.h"
#include "VulkanRenderPass.h"
#include "VulkanFramebuffer.h"
#include "VulkanRAII.h"
#include "VulkanDescriptorSet.h"
#include "VulkanDebug.h"
#include "VulkanInstance.h"
#include "Camera.h"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "keys.h"
#include "events.h"
#include "Window.h"
#include "InputManager.h"

#ifdef NDBUG
constexpr bool enableValidation = false;
#else
constexpr bool enableValidation = true;
#endif

#define REPORT_ERROR(result, msg) if(result != VK_SUCCESS) throw std::runtime_error{msg}
#define offsetOf(s,m) static_cast<uint32_t>(offsetof(s, m))

constexpr int MAX_IN_FLIGHT_FRAMES = 2;


struct Texture{
    VulkanImage image;
    VulkanImageView imageView;
    VulkanSampler sampler;
};


struct ShaderModule{
    ShaderModule(const std::string& path, VkDevice device);
    ~ShaderModule();

    VkShaderModule shaderModule = VK_NULL_HANDLE;

    inline operator VkShaderModule() const {
        return shaderModule;
    }
private:
    VkDevice device = VK_NULL_HANDLE;
};


class VulkanBaseApp : Window, InputManager{
public:
    explicit VulkanBaseApp(std::string_view name, int width = 1080, int height = 720, bool relativeMouseMode = true);
    void init();
    void run();

    static std::vector<char> loadFile(const std::string& path);

protected:
    void initWindow() override;

    void initVulkan();

    virtual void initApp() {};

    void createInstance();

    void pickPhysicalDevice();

    void createSwapChain();

    void createRenderPass();

    void createFramebuffer();

    void createSyncObjects();

    void createCameraDescriptorSetLayout();

    void createCameraDescriptorPool();

    void createCameraDescriptorSet();


    void recreateSwapChain();

    virtual void onSwapChainDispose(){};

    virtual void onSwapChainRecreation() {};

    virtual VkCommandBuffer* buildCommandBuffers(uint32_t imageIndex, uint32_t& numCommandBuffers) {
        return nullptr;
    }

    virtual void drawFrame();

    virtual void update(float time);


    void createCameraBuffers();

    void createLogicalDevice();

    void mainLoop();

    virtual void checkSystemInputs();

    virtual void checkAppInputs() {
        cameraController->processInput();
    };

    void createDebugMessenger();

    void cleanupSwapChain();

    float getTime();

    virtual void cleanup() {}

    virtual void onPause() {}

    bool isRunning() const {
        return !glfwWindowShouldClose(window);
    }

private:
    void setPaused(bool flag);

protected:
    VulkanInstance instance;
    VulkanDebug vulkanDebug;
    VulkanSurface surface;
    VulkanDevice device;
    VulkanSwapChain swapChain;
    VulkanRenderPass renderPass;
    VulkanDescriptorSetLayout cameraDescriptorSetLayout;
    VulkanDescriptorPool cameraDescriptorPool;


    std::vector<VulkanFramebuffer> framebuffers;
    std::vector<VkDescriptorSet> cameraDescriptorSets;


    VmaAllocator memoryAllocator;

    std::vector<VulkanBuffer> cameraBuffers;
    Camera camera;


    std::vector<VulkanSemaphore> imageAcquired;
    std::vector<VulkanSemaphore> renderingFinished;
    std::vector<VulkanFence> inFlightFences;
    std::vector<VulkanFence*> inFlightImages;

    std::vector<const char*> instanceExtensions;
    std::vector<const char*> validationLayers;
    std::vector<const char*> deviceExtensions{
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    };
    VulkanExtensions ext;

    Action* exit = nullptr;
    Action* pause = nullptr;
    bool paused = false;
    float elapsedTime = 0.0f;

    uint32_t currentImageIndex = 0;
    int currentFrame = 0;
    Mesh mesh;
    BaseCameraController* cameraController;
};