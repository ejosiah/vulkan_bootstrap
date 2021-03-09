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
#include "VulkanResource.h"
#include "VulkanSurface.h"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#ifdef NDBUG
constexpr bool enableValidation = false;
#else
constexpr bool enableValidation = true;
#endif

#define REPORT_ERROR(result, msg) if(result != VK_SUCCESS) throw std::runtime_error{msg}
#define offsetOf(s,m) static_cast<uint32_t>(offsetof(s, m))

constexpr int MAX_IN_FLIGHT_FRAMES = 2;

template<typename Resource>
struct VulkanResource{
    Resource resource;
    VmaAllocation allocation;

    inline operator Resource() const {
        return resource;
    }
};

using Buffer  = VulkanResource<VkBuffer>;
using Image = VulkanResource<VkImage>;

struct Texture{
    Image image;
    VkImageView imageView;
    VkSampler sampler;
};

struct SwapChainSupportDetails{
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentMode;
};

struct SwapChainDetails{
    VkSwapchainKHR swapchain;
    VkFormat format;
    VkExtent2D extent;
    std::vector<VkImage> images;
    std::vector<VkImageView> imageviews;
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

struct Camera{
    glm::mat4 model = glm::mat4(1);
    glm::mat4 view = glm::mat4(1);
    glm::mat4 proj = glm::mat4(1);
};

class VulkanBaseApp {
public:
    explicit VulkanBaseApp(std::string_view name, int width = 1080, int height = 720);
    void init();
    void run();

    static std::vector<char> loadFile(const std::string& path);

    bool fullscreen = false;

private:
    void initWindow();

    void initVulkan();

    void createInstance();

    void pickPhysicalDevice();

    void createSwapChain();

    void createCommandPool();

    void createRenderPass();

    void createFramebuffer();

    void createCommandBuffer();

    void createGraphicsPipeline();

    void createSyncObjects();

    void createDescriptorSetLayout();

    void createDescriptorPool();

    void createDescriptorSet();

    void recreateSwapChain();

    void drawFrame();

    void sendPushConstants();

    void update(float time);

    template<typename Command>
    void oneTimeCommand(const VkQueue destination, Command&& command);

    void createVertexBuffer();

    void createIndexBuffer();

    void createCameraBuffers();

    void createTextureBuffers();

    uint32_t findMemoryTypeIndex(uint32_t memoryTypeBitsReq, VkMemoryPropertyFlags requiredProperties);

    void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout,
                               VkImageLayout newLayout);

    SwapChainSupportDetails querySwapChainSupport();

    VkSurfaceFormatKHR chooseSurfaceFormat(std::vector<VkSurfaceFormatKHR>& formats);

    VkPresentModeKHR choosePresentMode(std::vector<VkPresentModeKHR> &presentModes);

    void createLogicalDevice();

    void mainLoop();

    void createDebugMessenger();

    float currentTime();

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo();

    void cleanupSwapChain();

    void cleanup();

    static inline void onResize(GLFWwindow* window, int width, int height){
        VulkanBaseApp* self = reinterpret_cast<VulkanBaseApp*>(glfwGetWindowUserPointer(window));
        self->resized = true;
    }

    static inline void onKeyPress(GLFWwindow* window, int key, int scancode, int action, int mods){
       if(key == GLFW_KEY_ESCAPE){
           glfwSetWindowShouldClose(window, GLFW_TRUE);
       }
    }

    static VkBool32 VKAPI_PTR  debugCallBack(
    VkDebugUtilsMessageSeverityFlagBitsEXT           messageSeverity,
            VkDebugUtilsMessageTypeFlagsEXT                  messageTypes,
    const VkDebugUtilsMessengerCallbackDataEXT*      pCallbackData,
    void*                                            pUserData);

private:
    std::string_view name;
    int width;
    int height;
    VkInstance instance = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VulkanDevice device;
    GLFWwindow* window = nullptr;
    std::vector<const char*> instanceExtensions;
    std::vector<const char*> validationLayers;
    std::vector<const char*> deviceExtensions{
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
            VK_EXT_FULL_SCREEN_EXCLUSIVE_EXTENSION_NAME
    };
    VkDebugUtilsMessengerEXT debugMessenger;
    VulkanExtensions ext;

    VmaAllocator memoryAllocator;

    VulkanSurface surface;
    SwapChainDetails swapChainDetails;

    std::vector<Vertex> vertices = {
            {{-0.5f, -0.5f, 0.0f, 1.0f}, glm::vec3(0), {1.0f, 0.0f, 0.0f}, glm::vec2(0)},
            {{0.5f, -0.5f, 0.0f, 1.0f}, glm::vec3(0), {0.0f, 1.0f, 0.0f}, glm::vec2(0)},
            {{0.5f, 0.5f, 0.0f, 1.0f}, glm::vec3(0), {0.0f, 0.0f, 1.0f}, glm::vec2(0)},
            {{-0.5f, 0.5f, 0.0f, 1.0f}, glm::vec3(0), {1.0f, 1.0f, 1.0f}, glm::vec2(0)}
    };

    std::vector<uint32_t> indices = {
        0, 1, 2, 2, 3, 0
    };

    VulkanBuffer vertexBuffer;
    VulkanBuffer indexBuffer;
    std::vector<Buffer> cameraBuffers;
    Texture texture;
    Camera camera;
    VkCommandPool commandPool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> commandBuffers;
    VkCommandBuffer pushConstantCmdBuffer;
    VkRenderPass renderPass = VK_NULL_HANDLE;

    VkPipeline pipeline = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptorSetLayout;
    std::vector<VkDescriptorSet> descriptorSets;
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    VkPipelineLayout layout = VK_NULL_HANDLE;

    std::vector<VkFramebuffer> framebuffers;
    std::vector<VkSemaphore> imageAcquired;
    std::vector<VkSemaphore> renderingFinished;
    std::vector<VkFence> inFlightFences;
    std::vector<VkFence> inFlightImages;
    uint32_t currentImageIndex;
    bool resized = false;
    int currentFrame = 0;
    Mesh mesh;
};