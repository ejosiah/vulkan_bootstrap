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

//template<typename Resource>
//struct VulkanResource{
//    Resource resource;
//    VmaAllocation allocation;
//
//    inline operator Resource() const {
//        return resource;
//    }
//};
//
//using Buffer  = VulkanResource<VkBuffer>;
//using Image = VulkanResource<VkImage>;

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

    virtual std::vector<VkCommandBuffer> buildCommandBuffers(uint32_t i) {
        return {};
    }

    void drawFrame();

    void sendPushConstants();

    virtual void update(float time);

    void createVertexBuffer();

    void createIndexBuffer();

    void createCameraBuffers();

    void createTextureBuffers();

    void createLogicalDevice();

    void mainLoop();

    void createDebugMessenger();

    float currentTime();

//    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo();

    void cleanupSwapChain();

    void cleanup();

    static inline void onResize(GLFWwindow* window, int width, int height){
        VulkanBaseApp* self = reinterpret_cast<VulkanBaseApp*>(glfwGetWindowUserPointer(window));
        self->resized = true;
        self->width = width;
        self->height = height;
    }

    static inline void onKeyPress(GLFWwindow* window, int key, int scancode, int action, int mods){
       if(key == GLFW_KEY_ESCAPE){
           glfwSetWindowShouldClose(window, GLFW_TRUE);
       }
    }

//    static VkBool32 VKAPI_PTR  debugCallBack(
//    VkDebugUtilsMessageSeverityFlagBitsEXT           messageSeverity,
//            VkDebugUtilsMessageTypeFlagsEXT                  messageTypes,
//    const VkDebugUtilsMessengerCallbackDataEXT*      pCallbackData,
//    void*                                            pUserData);

private:
    std::string_view name;
    int width;
    int height;
    VulkanInstance vkInstance;
    VulkanDebug vulkanDebug;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VulkanDevice device;
    GLFWwindow* window = nullptr;
    std::vector<const char*> instanceExtensions;
    std::vector<const char*> validationLayers;
    std::vector<const char*> deviceExtensions{
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    };
//    VkDebugUtilsMessengerEXT debugMessenger; // TODO
    VulkanExtensions ext;

    VmaAllocator memoryAllocator;   // TODO remove

    VulkanSurface surface;
    VulkanSwapChain swapChain;

    VulkanBuffer vertexBuffer;
    VulkanBuffer indexBuffer;
    std::vector<VulkanBuffer> cameraBuffers;
    Texture texture;
    Camera camera;
    VulkanCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;
    VkCommandBuffer pushConstantCmdBuffer;
    VulkanRenderPass renderPass;

    VulkanPipeline pipeline;
    VulkanDescriptorSetLayout descriptorSetLayout;
    std::vector<VkDescriptorSet> descriptorSets;
    VulkanDescriptorPool descriptorPool;
    VulkanPipelineLayout layout;

    std::vector<VulkanFramebuffer> framebuffers;
    std::vector<VulkanSemaphore> imageAcquired;
    std::vector<VulkanSemaphore> renderingFinished;
    std::vector<VulkanFence> inFlightFences;
    std::vector<VulkanFence*> inFlightImages;
    uint32_t currentImageIndex;
    bool resized = false;
    int currentFrame = 0;
    Mesh mesh;
};