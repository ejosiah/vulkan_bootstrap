#include <fmt/format.h>
#include <VulkanInstance.h>
#include <VulkanDevice.h>
#include <Sort.hpp>
#include <VulkanDebug.h>
#include <VulkanShaderModule.h>
#include <mutex>
#include "Statistics.hpp"
#include "FourWayRadixSort.hpp"

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

template<typename Sorter>
void perf_test(int iterations, const std::vector<std::string>& operations){
    std::map<std::string, std::vector<stats::Statistics<float>>> statistics;
    for(const auto& op : operations){
        statistics.insert(std::make_pair(op, std::vector<stats::Statistics<float>>{}));
    }

//    auto rng = rngFunc<uint32_t>(0, std::numeric_limits<uint32_t>::max() - 1, 1 << 20);
    auto rng = rngFunc<uint32_t>(0, 100, 1 << 20);
    for(int i = 0; i <= 15; i++){
        auto numItems = 1 << (i + 8);
        std::vector<uint32_t> data(numItems);
        std::generate(begin(data), end(data), rng);
        assert(!std::is_sorted(begin(data), end(data)));

        Sorter sorter{ &g_device };
        sorter.debug = true;
        sorter.init();
        spdlog::info("running {} iterations, sorting {} items", iterations, numItems);
        for(int j = 0; j < iterations; j++) {
            spdlog::debug("run number {}, sorting {} items\n", j, numItems);
            VkBufferUsageFlags usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            VulkanBuffer buffer = g_device.createDeviceLocalBuffer(data.data(), BYTE_SIZE(data), usage);

            g_device.graphicsCommandPool().oneTimeCommand([&](auto cmdBuffer) {
                sorter(cmdBuffer, buffer);
            });
            verify(buffer);

            sorter.profiler.commit();
        }
        auto stats = sorter.profiler.groupStats();

        for(const auto& op : operations){
            statistics[op].push_back(stats[op]);
        }

    }
    fmt::print("\n\n");

    for(int i = 0; i <= 15; i++){
        int inputSize = 1 << (i + 8);
        fmt::print("{:<20}{:<20}{:<20}{:<20}{:<20}{:<20}", "Operation", "Input Size", "Average Time (ms)", "Min Time (ms)", "Max Time (ms)", "Median Time (ms)");
        float total = 0.0f;
        for(const auto& [op, stats] : statistics){
            total += stats[i].meanValue;
            fmt::print("{:<20}{:<20}{:<20}{:<20}{:<20}{:<20}", op, inputSize, stats[i].meanValue, stats[i].minValue, stats[i].maxValue, stats[i].medianValue);
        }
        fmt::print("{:<20}{:<20}{:<20}{:<20}{:<20}{:<20}\n", "total", "", total, "", "", "", "");
    }
    fmt::print("\n\n");
}

int main() {
    size_t iterations = 100;
    initVulkan();
    perf_test<RadixSort>(iterations, {"count", "prefix_sum", "reorder"});
    perf_test<FourWayRadixSort>(iterations, {"local_sort", "prefix_sum", "global_shuffle"});
}