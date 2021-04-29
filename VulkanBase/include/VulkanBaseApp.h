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
#include "../../3rdParty/include/vk_mem_alloc.h"
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
#include "Plugin.hpp"
#include "Settings.hpp"

#ifdef NDBUG
constexpr bool enableValidation = false;
#else
constexpr bool enableValidation = true;
#endif

#define REPORT_ERROR(result, msg) if(result != VK_SUCCESS) throw std::runtime_error{msg}
#define offsetOf(s,m) static_cast<uint32_t>(offsetof(s, m))

constexpr int MAX_IN_FLIGHT_FRAMES = 2;

struct FramebufferAttachment{
    VulkanImage image;
    VulkanImageView imageView;
};


struct DepthFormats{
    std::vector<VkFormat> formats{
            VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT,
            VK_FORMAT_D24_UNORM_S8_UINT
    };
    VkFormatFeatureFlags  features = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
    VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
};

using DepthBuffer = FramebufferAttachment;
using ColorBuffer = FramebufferAttachment;

using RenderPassInfo = std::tuple<std::vector<VkAttachmentDescription>,
                                  std::vector<SubpassDescription>,
                                  std::vector<VkSubpassDependency>>;


/**
 * @file VulkanBaseApp.h
 * @breif Basic interface for interfacing with Vulkan, creates a vulkan swapchain, framebuffer and connects it with a ui window
 */
class VulkanBaseApp : protected Window, protected InputManager{
    friend class Plugin;
public:
    explicit VulkanBaseApp(std::string_view name, const Settings& settings = {}, std::vector<std::unique_ptr<Plugin>> plugins = {});

    virtual ~VulkanBaseApp();
    /**
     *  initializes the window, input managers Vulkan
     */
    void init();


    void addPlugin(std::unique_ptr<Plugin>& plugin);

    /**
     * runs the application loop
     */
    void run();


    /**
     * loads a files from the file system
     * @param path path of file to load
     * @return the contents of the file
     */
    static std::vector<char> loadFile(const std::string& path);

protected:
    void initWindow() override;

    void initVulkan();

    void initPlugins();

    /**
     * app specific initialization, this should be overridden by subclasses
     */
    virtual void initApp() = 0;

    void createInstance();

    void pickPhysicalDevice();

    void createSwapChain();

    void createDepthBuffer();

    void createColorBuffer();

    VkFormat findDepthFormat();

    void createRenderPass();

    /**
     * constructs the render pass graph required to create the render pass
     * to connect to the swapchain, this should be overridden that require
     * a custom render graph
     * @return a renderPassInfo containing a single subpass with a color attachment
     * and depth attachment (if enabled) with no subpass dependencies
     */
    virtual RenderPassInfo buildRenderPass();

    void createFramebuffer();

    void createSyncObjects();

    void recreateSwapChain();

    virtual void onSwapChainDispose();

    virtual void onSwapChainRecreation();

    /**
     * Command Buffer to be submitted to the graphics queue for rendering
     * @param imageIndex current image index retrieved from the swapchain
     * @param numCommandBuffers number of command buffers in the returned pointer
     * @return pointer to an array of command buffers
     */
    virtual VkCommandBuffer* buildCommandBuffers(uint32_t imageIndex, uint32_t& numCommandBuffers) = 0;

    void  notifyPluginsOfNewFrameStart();

    void notifyPluginsOfSwapChainDisposal();

    void notifyPluginsOfSwapChainRecreation();

    void registerPluginEventListeners(Plugin* plugin);

    void updatePlugins(float dt);

    void cleanupPlugins();

    virtual void newFrame();

    /**
     * Renders the current image on the swap chain and then sends it for presentation
     * subclasses can override this for custom drawing routines
     */
    virtual void drawFrame();

    void presentFrame();

    void nextFrame();

    void calculateFPS(float dt);

    /**
     * subclasses should override this to update their scenes just before rendering
     * and presenting to the swapchain
     * @param time time elapsed since the last frame
     */
    virtual void update(float time);

    void createLogicalDevice();

    void mainLoop();

    /**
     * Checks if system inputs where triggered, default system inputs are pause and exit
     * subclasses should override this to add or define app custom system inputs
     */
    virtual void checkSystemInputs();

    /**
     * Should be overridden by subclass for checking app inputs
     */
    virtual void checkAppInputs();

    void createDebugMessenger();

    void cleanupSwapChain();

    void fullscreenCheck();

    float getTime();

    /**
     * Subclasses should override this method to clean application just before shutdown
     */
    virtual void cleanup();

    /**
     * Triggered when the application is paused, subclasses should override this to display
     * custom pause menus
     */
    virtual void onPause();

    bool isRunning() const;

    template<typename PluginType = Plugin>
    static PluginType& plugin(const std::string &name) {
        assert(appInstance != nullptr && appInstance->ready);
        for(auto& plugin : appInstance->plugins){
            if(plugin->name() == name){
                return reinterpret_cast<PluginType&>(*plugin);
            }
        }
        throw std::runtime_error{"Requested plugin not found"};
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

    std::vector<VulkanFramebuffer> framebuffers;

    std::vector<VulkanSemaphore> imageAcquired;
    std::vector<VulkanSemaphore> renderingFinished;
    std::vector<VulkanFence> inFlightFences;
    std::vector<VulkanFence*> inFlightImages;

    std::vector<const char*> instanceExtensions;
    std::vector<const char*> validationLayers;
    std::vector<const char*> deviceExtensions{
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    };
    VulkanExtensions ext{};

    Action* exit = nullptr;
    Action* pause = nullptr;
    bool paused = false;
    float elapsedTime = 0.0f;

    uint32_t currentImageIndex = 0;
    int currentFrame = 0;
    DepthFormats depthFormats;
    DepthBuffer depthBuffer;
    ColorBuffer colorBuffer;
    bool swapChainInvalidated = false;
    VkPhysicalDeviceFeatures enabledFeatures{};
    uint64_t totalFrames = 0;
    uint32_t framePerSecond = 0;
    uint32_t frameCount = 0;
    uint32_t swapChainImageCount = 0;
    std::vector<std::unique_ptr<Plugin>> plugins;
    Settings settings;
    bool ready = false;
    bool toggleFullscreen = false;
    void* deviceCreateNextChain = nullptr;

private:
    static VulkanBaseApp* appInstance;
};