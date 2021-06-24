#pragma once

#include <gtest/gtest.h>
#include "VulkanInstance.h"
#include "VulkanDevice.h"
#include "VulkanShaderModule.h"
#include "VulkanBaseApp.h"

static std::vector<const char*> instanceExtensions{VK_EXT_DEBUG_UTILS_EXTENSION_NAME};
static std::vector<const char*> validationLayers{"VK_LAYER_KHRONOS_validation"};
static std::vector<const char*> deviceExtensions{ };

struct PipelineMetaData{
    std::string name;
    std::string shadePath;
    std::vector<VulkanDescriptorSetLayout*> layouts;
    std::vector<VkPushConstantRange> ranges;
};

struct Pipeline{
    VulkanPipeline pipeline;
    VulkanPipelineLayout layout;
};

class VulkanFixture : public ::testing::Test{
protected:
    VulkanInstance instance;
    VulkanDevice device;
    VulkanDebug debug;
    VulkanDescriptorPool descriptorPool;
    Settings settings;
    std::map<std::string, Pipeline> pipelines;
    uint32_t maxSets = 100;

    void SetUp() override {
        spdlog::set_level(spdlog::level::warn);
        initVulkan();
        createPipelines();
        postVulkanInit();
    }

    void initVulkan(){
        settings.queueFlags = VK_QUEUE_COMPUTE_BIT;
        createInstance();
        debug = VulkanDebug{ instance };
        createDevice();
        createDescriptorPool();
    }

    void createDescriptorPool(){
        std::array<VkDescriptorPoolSize, 17> poolSizes{
                {
                        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 100 * maxSets},
                        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 100 * maxSets},
                        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 100 * maxSets},
                        { VK_DESCRIPTOR_TYPE_SAMPLER, 100 * maxSets },
                        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 100 * maxSets },
                        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 100 * maxSets },
                        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 100 * maxSets },
                        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 100 * maxSets },
                        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 100 * maxSets },
                        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 100 * maxSets },
                        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 100 * maxSets },
                        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 100 * maxSets },
                        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 100 * maxSets },
                        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 100 * maxSets },
                        { VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK_EXT, 100 * maxSets },
                        { VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 100 * maxSets },
                        { VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV, 100 * maxSets },

                }
        };
        descriptorPool = device.createDescriptorPool(maxSets, poolSizes, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT);

    }

    void createInstance(){
        VkApplicationInfo appInfo{};
        appInfo.sType  = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.applicationVersion = VK_MAKE_VERSION(0, 0, 0);
        appInfo.pApplicationName = "Vulkan Test";
        appInfo.apiVersion = VK_API_VERSION_1_2;
        appInfo.pEngineName = "";

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;
        createInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
        createInfo.ppEnabledExtensionNames = instanceExtensions.data();

        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
        auto debugInfo = VulkanDebug::debugCreateInfo();
        createInfo.pNext = &debugInfo;

        instance = VulkanInstance{appInfo, {instanceExtensions, validationLayers}};
    }

    void createDevice(){
        auto pDevice = enumerate<VkPhysicalDevice>([&](uint32_t* size, VkPhysicalDevice* pDevice){
            return vkEnumeratePhysicalDevices(instance, size, pDevice);
        }).front();
        device = VulkanDevice{ instance, pDevice, settings};
        VkPhysicalDeviceFeatures enabledFeatures{};
        enabledFeatures.robustBufferAccess = VK_TRUE;
        device.createLogicalDevice(enabledFeatures, deviceExtensions, validationLayers, VK_NULL_HANDLE, VK_QUEUE_COMPUTE_BIT);
    }

    template<typename Func>
    void execute(Func&& func){
        device.graphicsCommandPool().oneTimeCommand(func);
    }

    void createPipelines(){
        for(auto& metaData : pipelineMetaData()){
            auto shaderModule = VulkanShaderModule{ metaData.shadePath, device};
            auto stage = initializers::shaderStage({ shaderModule, VK_SHADER_STAGE_COMPUTE_BIT});
            Pipeline pipeline;
            std::vector<VkDescriptorSetLayout> setLayouts;
            for(auto& layout : metaData.layouts){
                setLayouts.push_back(layout->handle);
            }
            pipeline.layout = device.createPipelineLayout(setLayouts, metaData.ranges);

            auto createInfo = initializers::computePipelineCreateInfo();
            createInfo.stage = stage;
            createInfo.layout = pipeline.layout;

            pipeline.pipeline = device.createComputePipeline(createInfo);
            pipelines.insert(std::make_pair(metaData.name, std::move(pipeline)));
        }
    }

    VkPipeline pipeline(const std::string& name){
        assert(pipelines.find(name) != end(pipelines));
        return pipelines[name].pipeline;
    }

    VkPipelineLayout layout(const std::string& name){
        assert(pipelines.find(name) != end(pipelines));
        return pipelines[name].layout;
    }

    virtual std::vector<PipelineMetaData> pipelineMetaData() {
        return {};
    }

    virtual void postVulkanInit() {}

    template<typename T>
    std::function<T()> rngFunc(T lower, T upper, uint32_t seed = std::random_device{}()) {
        std::default_random_engine engine{ seed };
        if constexpr(std::is_same_v<T, int>){
            std::uniform_int_distribution<T> dist{lower, upper};
            return std::bind(dist, engine);
        }else {
            std::uniform_real_distribution<T> dist{lower, upper};
            return std::bind(dist, engine);
        }
    }
};