#include "vulkan_context.hpp"
#include <spdlog/spdlog.h>

#include <utility>

class VulkanContext::Impl{
public:
    Impl(ContextCreateInfo createInfo, VulkanContext& ctx)
    : createInfo{ std::move(createInfo) }
    , context{ ctx }
    {}

    void init(){
        initFileManager();
        createInstance();
        initExtensions();
        createDebugMessenger();
        pickPhysicalDevice();
        createLogicalDevice();
    }

    void initFileManager(){
        context.fileManager = FileManager{ createInfo.searchPaths };
    }

    void createInstance() {
        context.instance = VulkanInstance(createInfo.applicationInfo, createInfo.instanceExtAndLayers);
    }

    void initExtensions() {
        context.extensions = VulkanExtensions{ context.instance };
        ext::init(context.instance);
    }

    void pickPhysicalDevice() {
        auto& instance = context.instance;
        auto & settings = createInfo.settings;
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

        auto& device = context.device;
        device = std::move(devices.front());
        settings.msaaSamples = std::min(settings.msaaSamples, device.getMaxUsableSampleCount());
        checkDeviceExtensionSupport();
        spdlog::info("selected device: {}", device.name());
    }

    void checkDeviceExtensionSupport() {
        auto& device = context.device;
        auto deviceExtensions = createInfo.deviceExtAndLayers.extensions;
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

    void createLogicalDevice() {
        context.device.createLogicalDevice(
                createInfo.enabledFeature,
                createInfo.deviceExtAndLayers.extensions,
                createInfo.deviceExtAndLayers.validationLayers,
                createInfo.surface,
                createInfo.settings.queueFlags,
                createInfo.deviceCreateNextChain);

    }

    void createDebugMessenger(){
//#ifdef DEBUG_MODE
        context.vulkanDebug = VulkanDebug{ context.instance };
//#endif
    }

    std::string resource(const std::string& path){
        return context.fileManager.getFullPath(path)->string();
    }

private:
    VulkanContext& context;
    ContextCreateInfo createInfo;
};

VulkanContext::VulkanContext(ContextCreateInfo createInfo)
: pimpl{ new Impl(std::move(createInfo), *this)}
{

}

void VulkanContext::init() {
    pimpl->init();
}

VulkanContext::~VulkanContext() {
    delete pimpl;
}

std::string VulkanContext::resource(const std::string &path) {
    return pimpl->resource(path);
}
