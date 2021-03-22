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
#include "Texture.h"
#include "VulkanShaderModule.h"
#ifdef NDBUG
constexpr bool enableValidation = false;
#else
constexpr bool enableValidation = true;
#endif

#define REPORT_ERROR(result, msg) if(result != VK_SUCCESS) throw std::runtime_error{msg}
#define offsetOf(s,m) static_cast<uint32_t>(offsetof(s, m))

constexpr int MAX_IN_FLIGHT_FRAMES = 2;

struct DepthFormats{
    std::vector<VkFormat> formats{
            VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT,
            VK_FORMAT_D24_UNORM_S8_UINT
    };
    VkFormatFeatureFlags  features = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
    VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
};

struct DepthBuffer{
    VulkanImage image;
    VulkanImageView imageView;
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

    void createDepthBuffer();

    VkFormat findDepthFormat();

    void createRenderPass();

    void createFramebuffer();

    void createSyncObjects();

    void createCameraDescriptorSetLayout();

    void createCameraDescriptorPool();

    void createCameraDescriptorSet();

    void recreateSwapChain();

    virtual void onSwapChainDispose();

    virtual void onSwapChainRecreation();

    virtual VkCommandBuffer* buildCommandBuffers(uint32_t imageIndex, uint32_t& numCommandBuffers);

    virtual void drawFrame();

    virtual void update(float time);

    void createCameraBuffers();

    void createLogicalDevice();

    void mainLoop();

    virtual void checkSystemInputs();

    virtual void checkAppInputs();

    void createDebugMessenger();

    void cleanupSwapChain();

    float getTime();

    virtual void cleanup();

    virtual void onPause();

    bool isRunning() const;

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
    DepthFormats depthFormats;
    DepthBuffer depthBuffer;
    bool depthTestEnabled = false;
};