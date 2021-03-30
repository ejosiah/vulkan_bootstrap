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
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "keys.h"
#include "events.h"
#include "Window.h"
#include "InputManager.h"
#include "Texture.h"
#include "VulkanShaderModule.h"
#include "VulkanStructs.h"
#include "glm_format.h"

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

using RenderPassInfo = std::tuple<std::vector<VkAttachmentDescription>,
                                  std::vector<SubpassDescription>,
                                  std::vector<VkSubpassDependency>>;

struct Settings{
    bool fullscreen = false;
    bool relativeMouseMode = false;
    bool depthTest = false;
    bool vSync = false;
    VkPhysicalDeviceFeatures enabledFeatures{};
};

class VulkanBaseApp : protected Window, protected InputManager{
public:
    explicit VulkanBaseApp(std::string_view name, int width = 1080, int height = 720, const Settings& settings = {});
    void init();
    void run();

    static std::vector<char> loadFile(const std::string& path);

protected:
    void initWindow() override;

    void initVulkan();

    virtual void initApp() = 0;

    void createInstance();

    void pickPhysicalDevice();

    void createSwapChain();

    void createDepthBuffer();

    VkFormat findDepthFormat();

    void createRenderPass();

    virtual RenderPassInfo buildRenderPass();

    void createFramebuffer();

    void createSyncObjects();

    void recreateSwapChain();

    virtual void onSwapChainDispose();

    virtual void onSwapChainRecreation();

    virtual VkCommandBuffer* buildCommandBuffers(uint32_t imageIndex, uint32_t& numCommandBuffers) = 0;

    virtual void drawFrame();

    void calculateFPS();

    virtual void update(float time);

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

    std::vector<VulkanFramebuffer> framebuffers;

    VmaAllocator memoryAllocator;

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
    DepthFormats depthFormats;
    DepthBuffer depthBuffer;
    bool depthTestEnabled = false;
    bool vSync;
    VkPhysicalDeviceFeatures enabledFeatures{};
    uint64_t frameCount = 0;
    uint32_t framePerSecond = 0;
};