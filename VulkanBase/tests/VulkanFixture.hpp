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
    std::vector<VulkanDescriptorSetLayout> layouts;
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
    Settings settings;
    std::map<std::string, Pipeline> pipelines;

    void SetUp() override {
        spdlog::set_level(spdlog::level::warn);
        initVulkan();
    }

    void initVulkan(){
        settings.queueFlags = VK_QUEUE_COMPUTE_BIT;
        createInstance();
        debug = VulkanDebug{ instance };
        createDevice();
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
        device.createLogicalDevice({}, deviceExtensions, validationLayers, VK_NULL_HANDLE, VK_QUEUE_COMPUTE_BIT);
    }

    template<typename Func>
    void execute(Func&& func){
        device.graphicsCommandPool().oneTimeCommand(func);
    }

    virtual std::vector<PipelineMetaData> pipelineMetaData() {
        return {};
    }

    void createPipelines(){
        for(auto& metaData : pipelineMetaData()){
            auto shaderModule = VulkanShaderModule{ metaData.shadePath, device};
            auto stage = initializers::shaderStage({ shaderModule, VK_SHADER_STAGE_COMPUTE_BIT});
            Pipeline pipeline;
            std::vector<VkDescriptorSetLayout> setLayouts(metaData.layouts.begin(), metaData.layouts.end());
            pipeline.layout = device.createPipelineLayout(setLayouts, metaData.ranges);

            auto createInfo = initializers::computePipelineCreateInfo();
            createInfo.stage = stage;
            createInfo.layout = pipeline.layout;

            pipeline.pipeline = device.createComputePipeline(createInfo);
            pipelines.insert(std::make_pair(metaData.name, std::move(pipeline)));
        }
    }
};