#include <fmt/format.h>
#include <VulkanInstance.h>
#include <VulkanDevice.h>
#include <Sort.hpp>
#include <VulkanDebug.h>
#include <VulkanShaderModule.h>
#include <mutex>
#include "Statistics.hpp"


static std::vector<const char*> instanceExtensions{VK_EXT_DEBUG_UTILS_EXTENSION_NAME};
static std::vector<const char*> validationLayers{"VK_LAYER_KHRONOS_validation"};
static std::vector<const char*> deviceExtensions{ };

VulkanInstance g_instance;
VulkanDevice g_device;
VulkanDebug g_debug;
Settings g_settings;

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

    g_instance = VulkanInstance{appInfo, {instanceExtensions, validationLayers}};
}

void createDevice(){
    auto pDevice = enumerate<VkPhysicalDevice>([&](uint32_t* size, VkPhysicalDevice* pDevice){
        return vkEnumeratePhysicalDevices(g_instance, size, pDevice);
    }).front();
    g_device = VulkanDevice{ g_instance, pDevice, g_settings};
    VkPhysicalDeviceVulkan12Features features2{};
    features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    features2.hostQueryReset = VK_TRUE;
    VkPhysicalDeviceFeatures enabledFeatures{};
    enabledFeatures.robustBufferAccess = VK_TRUE;
    g_device.createLogicalDevice(enabledFeatures, deviceExtensions, validationLayers, VK_NULL_HANDLE, VK_QUEUE_COMPUTE_BIT, &features2);
}

void initVulkan(){
    g_settings.queueFlags = VK_QUEUE_COMPUTE_BIT;
    createInstance();
    ext::init(g_instance);
    g_debug = VulkanDebug{ g_instance };
    createDevice();
}

float toMillis(uint64_t duration){
    return static_cast<float>(duration) * 1e-6;
}

void verify(VulkanBuffer& buffer){
    VulkanBuffer hostBuffer = g_device.createStagingBuffer(buffer.size);
    g_device.copy(buffer, hostBuffer, buffer.size);

    auto first = reinterpret_cast<uint32_t*>(hostBuffer.map());
    auto last = first + hostBuffer.size/sizeof(uint32_t);
    assert(std::is_sorted(first, last));
    hostBuffer.unmap();
}

int main() {
    initVulkan();
    std::vector<Profiler::QueryGroup> countQueries;
    std::vector<Profiler::QueryGroup> prefixSumQueries;
    std::vector<Profiler::QueryGroup> reorderQueries;
    auto rng = rngFunc<uint32_t>(0, std::numeric_limits<uint32_t>::max() - 1, 1 << 20);
    for(int i = 0; i <= 15; i++){
        std::vector<uint32_t> data(1 << (i + 8));
        std::generate(begin(data), end(data), rng);
        assert(!std::is_sorted(begin(data), end(data)));
        VkBufferUsageFlags usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        VulkanBuffer buffer = g_device.createDeviceLocalBuffer(data.data(), BYTE_SIZE(data), usage);

        RadixSort sort{ &g_device, true };
        sort.init();
        g_device.graphicsCommandPool().oneTimeCommand([&](auto cmdBuffer){
           sort(cmdBuffer, buffer);
        });
        verify(buffer);

        sort.profiler.commit();
        countQueries.push_back(sort.profiler.getGroup("count").value());
        prefixSumQueries.push_back(sort.profiler.getGroup("prefix_sum").value());
        reorderQueries.push_back(sort.profiler.getGroup("reorder").value());
    }
    fmt::print("\n\n");
    fmt::print("{:<20}{:<20}{:<20}{:<20}{:<20}\n", "Input Size", "Count (ms)", "Prefix Sum (ms)", "Reorder (ms)", "Total Time (ms)");

    for(int i = 0; i <= 15; i++){
        int inputSize = 1 << (i + 8);
        float count = toMillis(countQueries[i].runtimes.front());
        float prefixSum = toMillis(prefixSumQueries[i].runtimes.front());
        float reorder = toMillis(reorderQueries[i].runtimes.front());
        float total = count + prefixSum + reorder;

        fmt::print("{:<20}{:<20}{:<20}{:<20}{:<20}\n", inputSize, count, prefixSum, reorder, total);

    }
}