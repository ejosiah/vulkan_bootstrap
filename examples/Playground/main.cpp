

#include <fmt/format.h>
#include <VulkanInstance.h>
#include <VulkanDevice.h>
#include <Sort.hpp>
#include <VulkanDebug.h>
#include <VulkanShaderModule.h>
#include <mutex>
#include "Statistics.hpp"
#include "FourWayRadixSort.hpp"
#include "xforms.h"
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <iostream>
#include "random.h"
#include <functional>
#include "VulkanBaseApp.h"
#include "VulkanDevice1.hpp"

#include <iostream>
#include <fstream>
#include <vector>
#include <memory>
#include <algorithm>
#include <numeric>
#include <array>
#include <type_traits>
#include <utility>
#include <chrono>
#include <string>
#include <exception>

#include <cassert>
#include <cstdlib>
#include <cstdio>
#include <cstdarg>

//#ifndef VMA_IMPLEMENTATION
//#define VMA_IMPLEMENTATION
//#include "vk_mem_alloc1.h"
//#endif

#define STRINGIZE(x) STRINGIZE2(x)
#define STRINGIZE2(x) #x
#define LINE_STRING STRINGIZE(__LINE__)
#define TEST(expr)  do { if(!(expr)) { \
        assert(0 && #expr); \
        throw std::runtime_error(__FILE__ "(" LINE_STRING "): ( " #expr " ) == false"); \
    } } while(false)
#define ERR_GUARD_VULKAN(expr)  do { if((expr) < 0) { \
        assert(0 && #expr); \
        throw std::runtime_error(__FILE__ "(" LINE_STRING "): VkResult( " #expr " ) < 0"); \
    } } while(false)


static std::vector<const char*> instanceExtensions{VK_EXT_DEBUG_UTILS_EXTENSION_NAME};
static std::vector<const char*> validationLayers{"VK_LAYER_KHRONOS_validation"};
static std::vector<const char*> deviceExtensions{ };

//VulkanInstance g_instance;
//VulkanDevice g_device;
//VulkanDebug g_debug;
//Settings g_settings;
//
//void createInstance(){
//    VkApplicationInfo appInfo{};
//    appInfo.sType  = VK_STRUCTURE_TYPE_APPLICATION_INFO;
//    appInfo.applicationVersion = VK_MAKE_VERSION(0, 0, 0);
//    appInfo.pApplicationName = "Vulkan Test";
//    appInfo.apiVersion = VK_API_VERSION_1_2;
//    appInfo.pEngineName = "";
//
//    VkInstanceCreateInfo createInfo{};
//    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
//    createInfo.pApplicationInfo = &appInfo;
//    createInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
//    createInfo.ppEnabledExtensionNames = instanceExtensions.data();
//
//    createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
//    createInfo.ppEnabledLayerNames = validationLayers.data();
//    auto debugInfo = VulkanDebug::debugCreateInfo();
//    createInfo.pNext = &debugInfo;
//
//    g_instance = VulkanInstance{appInfo, {instanceExtensions, validationLayers}};
//}
//
//void createDevice(){
//    auto pDevice = enumerate<VkPhysicalDevice>([&](uint32_t* size, VkPhysicalDevice* pDevice){
//        return vkEnumeratePhysicalDevices(g_instance, size, pDevice);
//    }).front();
//    g_device = VulkanDevice{ g_instance, pDevice, g_settings};
//    VkPhysicalDeviceVulkan12Features features2{};
//    features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
//    features2.hostQueryReset = VK_TRUE;
//    VkPhysicalDeviceFeatures enabledFeatures{};
//    enabledFeatures.robustBufferAccess = VK_TRUE;
//    g_device.createLogicalDevice(enabledFeatures, deviceExtensions, validationLayers, VK_NULL_HANDLE, VK_QUEUE_COMPUTE_BIT, &features2);
//}
//
//void initVulkan(){
//    g_settings.queueFlags = VK_QUEUE_COMPUTE_BIT;
//    createInstance();
//    ext::init(g_instance);
//#ifndef NDEBUG
//    g_debug = VulkanDebug{ g_instance };
//#endif
//    createDevice();
//}
//
//float toMillis(uint64_t duration){
//    return static_cast<float>(duration) * 1e-6;
//}
//
//void verify(VulkanBuffer& buffer){
//    VulkanBuffer hostBuffer = g_device.createStagingBuffer(buffer.size);
//    g_device.copy(buffer, hostBuffer, buffer.size);
//
//    auto first = reinterpret_cast<uint32_t*>(hostBuffer.map());
//    auto last = first + hostBuffer.size/sizeof(uint32_t);
//    assert(std::is_sorted(first, last));
//    hostBuffer.unmap();
//}
//
//template<typename Sorter>
//void perf_test(int iterations, const std::vector<std::string>& operations){
//    std::map<std::string, std::vector<stats::Statistics<float>>> statistics;
//    for(const auto& op : operations){
//        statistics.insert(std::make_pair(op, std::vector<stats::Statistics<float>>{}));
//    }
//
////    auto rng = rngFunc<uint32_t>(0, std::numeric_limits<uint32_t>::max() - 1, 1 << 20);
//    auto rng = rngFunc<uint32_t>(0, 100, 1 << 20);
//    for(int i = 0; i <= 15; i++){
//        auto numItems = 1 << (i + 8);
//        std::vector<uint32_t> data(numItems);
//        std::generate(begin(data), end(data), rng);
//        assert(!std::is_sorted(begin(data), end(data)));
//
//        Sorter sorter{ &g_device };
//        sorter.debug = true;
//        sorter.init();
//        spdlog::info("running {} iterations, sorting {} items", iterations, numItems);
//        for(int j = 0; j < iterations; j++) {
//            spdlog::debug("run number {}, sorting {} items\n", j, numItems);
//            VkBufferUsageFlags usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
//            VulkanBuffer buffer = g_device.createDeviceLocalBuffer(data.data(), BYTE_SIZE(data), usage);
//
//            g_device.graphicsCommandPool().oneTimeCommand([&](auto cmdBuffer) {
//                sorter(cmdBuffer, buffer);
//            });
//            verify(buffer);
//
//            sorter.profiler.commit();
//        }
//        auto stats = sorter.profiler.groupStats();
//
//        for(const auto& op : operations){
//            statistics[op].push_back(stats[op]);
//        }
//
//    }
//    fmt::print("\n\n");
//
//    for(int i = 0; i <= 15; i++){
//        int inputSize = 1 << (i + 8);
//        fmt::print("{:<20}{:<20}{:<20}{:<20}{:<20}{:<20}", "Operation", "Input Size", "Average Time (ms)", "Min Time (ms)", "Max Time (ms)", "Median Time (ms)");
//        float total = 0.0f;
//        for(const auto& [op, stats] : statistics){
//            total += stats[i].meanValue;
//            fmt::print("{:<20}{:<20}{:<20}{:<20}{:<20}{:<20}", op, inputSize, stats[i].meanValue, stats[i].minValue, stats[i].maxValue, stats[i].medianValue);
//        }
//        fmt::print("{:<20}{:<20}{:<20}{:<20}{:<20}{:<20}\n", "total", "", total, "", "", "", "");
//    }
//    fmt::print("\n\n");
//}
//
//float zndc(float z, float n, float f){
//   float num = (z + n) * f;
//   float denum = z * (f - n);
//   return num / denum;
//}
//
//float ndc_to_d(float z, float n, float f){
//    float num = f * n;
//    float denum = z * (f - n) - f;
//    return num/denum;
//}
//
//constexpr uint32_t flags =
//        aiProcess_GenSmoothNormals
//        | aiProcess_Triangulate
//        | aiProcess_CalcTangentSpace
//        | aiProcess_JoinIdenticalVertices
//        | aiProcess_ValidateDataStructure;
//
//
//using namespace Assimp;
//
//uint32_t numMeshes(const aiScene* scene, const aiNode* node){
//    if(node->mNumChildren < 1){
//        return node->mNumMeshes;
//    }
//    auto count = node->mNumMeshes;
//    for(auto i = 0; i < node->mNumChildren; i++){
//        count += numMeshes(scene, node->mChildren[i]);
//    }
//    return count;
//}
//
//uint32_t nodeDepth(const aiScene* scene, const aiNode* node){
//    if(node->mNumChildren < 1) return 1;
//
//    uint32_t maxDepth = std::numeric_limits<uint32_t>::min();
//    for(auto i = 0; i < node->mNumChildren; i++){
//        uint32_t depth = 1 + nodeDepth(scene, node->mChildren[i]);
//        maxDepth = std::max(depth, maxDepth);
//    }
//    return maxDepth;
//}
//
//void logBones(const aiScene* scene){
//
//
//    std::function<void(aiNode*)> logger = [&](aiNode* node) {
//
//        if(node->mNumMeshes > 0){
//            for(int i = 0; i < node->mNumMeshes; i++){
//                const auto mesh = scene->mMeshes[node->mMeshes[i]];
//                if(mesh->HasBones()){
//                    fmt::print("num bones: {}\n", mesh->mNumBones);
//                    for(int bid = 0; bid < mesh->mNumBones; bid++){
//                        const auto bone = mesh->mBones[bid];
//                        fmt::print("\t{}\n", bone->mName.C_Str());
//                        fmt::print("\t\tweights: {}\n", bone->mNumWeights);
////                        for(int wid = 0; wid < bone->mNumWeights; wid++){
////                            fmt::print("\t\tvertexId: {}, weight: {}\n", bone->mWeights[wid].mVertexId, bone->mWeights[wid].mWeight);
////                        }
//                    }
//                }
//            }
//        }
//        for(int i = 0; i < node->mNumChildren; i++){
//            logger(node->mChildren[i]);
//        }
//    };
//    fmt::print("bones:\n");
//    logger(scene->mRootNode);
//}
//
//#include <boost/uuid/uuid.hpp>
//#include <boost/uuid/uuid_generators.hpp>
//#include <boost/uuid/string_generator.hpp>
//#include <boost/uuid/uuid_io.hpp>
//#include <sstream>
//
//
//template<>
//struct fmt::formatter<boost::uuids::uuid>{
//
//    constexpr auto parse(format_parse_context& ctx) {
//        auto it = ctx.begin();
//        it++;
//        return it;
//    }
//
//    template <typename FormatContext>
//    auto format(const boost::uuids::uuid& uuid, FormatContext& ctx) {
//        return format_to(ctx.out(), "{}", to_string(uuid));
//    }
//};
//
//using gen = boost::uuids::random_generator;
//
//inline glm::mat4 qLeft(const glm::quat& q){
//    return {
//            {q.w, -q.x, -q.y, -q.z},
//            {q.x, q.w,  -q.z, q.y},
//            {q.y, q.z,  q.w,  -q.x},
//            {q.z, -q.y, q.x,  q.w}
//    };
//}
//
//inline glm::mat4 qRight(const glm::quat& q){
//    return {
//            {q.w, -q.x, -q.y, -q.z},
//            {q.x,  q.w,  q.z, -q.y},
//            {q.y, -q.z,  q.w,  q.x},
//            {q.z,  q.y, -q.x,  q.w}
//    };
//}
//
//class MyApp : public VulkanBaseApp{
//public:
//    MyApp():VulkanBaseApp{"TEST"}
//    {}
//
//protected:
//    void initApp() override {
//
//    }
//
//    VkCommandBuffer *buildCommandBuffers(uint32_t imageIndex, uint32_t &numCommandBuffers) override {
//        return nullptr;
//    }
//};
//
//void heapCheck(){
//    auto heapstatus = _heapchk();
//    switch( heapstatus )
//    {
//        case _HEAPOK:
//            spdlog::info(" OK - heap is fine\n" );
//            break;
//        case _HEAPEMPTY:
//            spdlog::info(" OK - heap is empty\n" );
//            break;
//        case _HEAPBADBEGIN:
//            spdlog::error( "ERROR - bad start of heap\n" );
//            assert(false);
//            break;
//        case _HEAPBADNODE:
//            spdlog::error( "ERROR - bad node in heap\n" );
//            assert(false);
//            break;
//    }
//}

//int main() {
//
//    VkApplicationInfo info = initializers::AppInfo();
//    info.pApplicationName = "playground";
//    info.pEngineName = "engine";
//    info.engineVersion = 0;
//    info.apiVersion = VK_API_VERSION_1_2;
//
//    std::vector<const char*> extensions{
//        "VK_KHR_surface", "VK_KHR_win32_surface", "VK_KHR_get_surface_capabilities2"
//    };
////    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
//
//    ExtensionsAndValidationLayers ev{ extensions };
//
//    VulkanInstance instance{info, ev};
//
////    auto instance = app.instance.instance;
//
//    auto pDevices = enumerate<VkPhysicalDevice>([&](auto count, auto* devicePtr){
//        return vkEnumeratePhysicalDevices(instance, count, devicePtr);
//    });
//
//    auto pDevice = pDevices.front();
//
//    float priority = 1.0f;
//    VkDeviceQueueCreateInfo qCreateInfo{};
//    qCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
//    qCreateInfo.queueFamilyIndex = 0;
//    qCreateInfo.queueCount = 1;
//    qCreateInfo.pQueuePriorities = &priority;
//
//    VkDeviceCreateInfo createInfo{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
//    createInfo.queueCreateInfoCount = 1;
//    createInfo.pQueueCreateInfos = &qCreateInfo;
//    createInfo.enabledLayerCount = 0;
//    createInfo.enabledExtensionCount = 0;
//
//    VkDevice device;
//    spdlog::info("creating device");
//    vkCreateDevice(pDevice, &createInfo, 0, &device);
//    spdlog::info("device created");
//
//    VmaAllocatorCreateInfo allocatorCreateInfo = {};
//    allocatorCreateInfo.physicalDevice = pDevice;
//    allocatorCreateInfo.device = device;
//    allocatorCreateInfo.instance = instance;
//
//    VmaAllocator allocator = VK_NULL_HANDLE;
//    ASSERT(vmaCreateAllocator(&allocatorCreateInfo, &allocator));
//    spdlog::info("allocator created");
//
//    auto imageCreateInfo = initializers::imageCreateInfo(VK_IMAGE_TYPE_2D, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_SAMPLED_BIT, 1024, 1024, 1);
//
////    VmaAllocationCreateInfo allocInfo{};
////    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
////    VkImage image;
////    VmaAllocation allocation;
////    VmaAllocationInfo allocationInfo{};
////    spdlog::info("allocating memory for image");
////    ASSERT(vmaCreateImage(allocator, &createInfo, &allocInfo, &image, &allocation, &allocationInfo));
////    spdlog::info("allocated memory for image");
//
//    Settings settings{};
//    VulkanDevice vulkanDevice{instance, pDevice, settings};
//    vulkanDevice.createLogicalDevice({}, {});
//    VulkanDevice1 vulkanDevice1{instance, pDevice, device, vulkanDevice.allocator};
//    auto image = vulkanDevice1.createImage(imageCreateInfo);
//
//    auto image1 = vulkanDevice.createImage(imageCreateInfo);
//
//
////    VmaAllocationCreateInfo allocInfo{};
////    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
////    VkImage image2;
////    VmaAllocation allocation;
////    VmaAllocationInfo allocationInfo{};
////    spdlog::info("allocating memory for image");
////    ASSERT(vmaCreateImage(vulkanDevice.allocator, &imageCreateInfo, &allocInfo, &image2, &allocation, &allocationInfo));
////    if(result != VK_SUCCESS){
////        spdlog::error("unable to create image");
////        assert(false);
////    }
////    spdlog::info("memory Allocated");
//
////    VmaAllocationCreateInfo allocInfo{};
////    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
////    VkImage image2 = VK_NULL_HANDLE;
////    VmaAllocation allocation = VK_NULL_HANDLE;
////    VmaAllocationInfo allocationInfo{};
////    spdlog::info("allocating memory for image");
////    ASSERT(vmaCreateImage(allocator, &imageCreateInfo, &allocInfo, &image2, &allocation, &allocationInfo));
////
//
////    spdlog::info("creating buffer");
////    std::array<int, 1024> data{};
////    std::iota(data.begin(), data.end(), 1);
////    VkDeviceSize size = 1024 * sizeof(int);
////    auto buffer = vulkanDevice.createBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY, size);
////
////    spdlog::info("copy data into buffer");
////    void* dest;
////    ERR_GUARD_VULKAN(vmaMapMemory(buffer.allocator, buffer.allocation, &dest));
////    memcpy(dest, data.data(), size);
////    vmaUnmapMemory(buffer.allocator, buffer.allocation);
//
//    spdlog::info("creating buffer");
//    std::array<int, 1024> data{};
//    std::iota(data.begin(), data.end(), 1);
//    VkDeviceSize size = 1024 * sizeof(int);
//
//    Buffer buffer{};
//
//    VkBufferCreateInfo bufferInfo{};
//    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
//    bufferInfo.size = size;
//    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
//    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
//
//    VmaAllocationCreateInfo allocInfo1{};
//    allocInfo1.usage = VMA_MEMORY_USAGE_CPU_ONLY;
//
//    vmaCreateBuffer(allocator, &bufferInfo, &allocInfo1, &buffer.handle, &buffer.allocation, nullptr);
//
//    spdlog::info("copy data into buffer");
//    void* dest;
//    vmaMapMemory(allocator, buffer.allocation, &dest);
//    memcpy(dest, data.data(), size);
//    vmaUnmapMemory(allocator, buffer.allocation);
//
//    spdlog::info("we made it here so we are good");
//    return 0;
//}

VkInstance g_instance;
VkPhysicalDevice g_physicalDevice;
VkDevice g_device;
VmaAllocator g_allocator;

static VkBool32 VKAPI_PTR debugCallBack(
        VkDebugUtilsMessageSeverityFlagBitsEXT           messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT                  messageTypes,
        const VkDebugUtilsMessengerCallbackDataEXT*      pCallbackData,
        void*                                            pUserData){

    if(messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT){
        spdlog::debug(pCallbackData->pMessage);
    }

    if(messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT){
        spdlog::info(pCallbackData->pMessage);
    }
    if(messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT){
        spdlog::warn(pCallbackData->pMessage);
    }
    if(messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT){
        spdlog::error(pCallbackData->pMessage);
    }

    return VK_FALSE;
}

static VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo(){
    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageType =
            VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;

    createInfo.messageSeverity =
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

    createInfo.pfnUserCallback = debugCallBack;

    return createInfo;
}

struct Buffer{
    VmaAllocator allocator;
    VkBuffer handle;
    VmaAllocation allocation;
};

int main() {
    VkApplicationInfo info{VK_STRUCTURE_TYPE_APPLICATION_INFO};
    info.pApplicationName = "playground";
    info.pEngineName = "engine";
    info.engineVersion = 0;
    info.apiVersion = VK_API_VERSION_1_2;

    std::vector<const char*> extensions{
            "VK_KHR_surface", "VK_KHR_win32_surface", "VK_KHR_get_surface_capabilities2"
    };
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    static std::vector<const char*> validationLayers{"VK_LAYER_KHRONOS_validation"};

    auto debugInfo = debugCreateInfo();
    VkInstanceCreateInfo createInfo{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    createInfo.pNext = &debugInfo;
    createInfo.pApplicationInfo = &info;
    createInfo.enabledExtensionCount = COUNT(extensions);
    createInfo.ppEnabledExtensionNames = extensions.data();
    createInfo.enabledLayerCount = COUNT(validationLayers);
    createInfo.ppEnabledLayerNames = validationLayers.data();

    vkCreateInstance(&createInfo, 0, &g_instance);

    VkResult result;
    uint32_t count;
    do{
        result = vkEnumeratePhysicalDevices(g_instance, &count, VK_NULL_HANDLE);
    }while(result == VK_INCOMPLETE);
    ERR_GUARD_VULKAN(result);
    spdlog::info("found {} devices", count);

    std::vector<VkPhysicalDevice> pDevices(count);
    do{
        result = vkEnumeratePhysicalDevices(g_instance, &count, pDevices.data());
    }while(result == VK_INCOMPLETE);
    ERR_GUARD_VULKAN(result);

    g_physicalDevice = pDevices.front();

    float priority = 1.0f;
    VkDeviceQueueCreateInfo qCreateInfo{};
    qCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    qCreateInfo.queueFamilyIndex = 0;
    qCreateInfo.queueCount = 1;
    qCreateInfo.pQueuePriorities = &priority;

    VkDeviceCreateInfo createDeviceInfo{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    createDeviceInfo.queueCreateInfoCount = 1;
    createDeviceInfo.pQueueCreateInfos = &qCreateInfo;
    createDeviceInfo.enabledLayerCount = 0;
    createDeviceInfo.enabledExtensionCount = 0;

    spdlog::info("creating device");
    vkCreateDevice(g_physicalDevice, &createDeviceInfo, 0, &g_device);
    spdlog::info("device created");

    VmaAllocatorCreateInfo allocatorCreateInfo = {};
    allocatorCreateInfo.physicalDevice = g_physicalDevice;
    allocatorCreateInfo.device = g_device;
    allocatorCreateInfo.instance = g_instance;

    ERR_GUARD_VULKAN(vmaCreateAllocator(&allocatorCreateInfo, &g_allocator));
    spdlog::info("allocator created");

    VkImageCreateInfo imageCreateInfo{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT;
    imageCreateInfo.extent.width = 1024;
    imageCreateInfo.extent.height = 1024;
    imageCreateInfo.extent.depth = 1;
    imageCreateInfo.mipLevels = 1;
    imageCreateInfo.arrayLayers = 1;
    imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCreateInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageCreateInfo.queueFamilyIndexCount = 0;
    imageCreateInfo.pQueueFamilyIndices = nullptr;
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    VkImage image;
    VmaAllocation allocation;
    VmaAllocationInfo allocationInfo{};
    spdlog::info("allocating memory for image");
    ERR_GUARD_VULKAN(vmaCreateImage(g_allocator, &imageCreateInfo, &allocInfo, &image, &allocation, &allocationInfo));
    spdlog::info("allocated memory for image");

    spdlog::info("creating buffer");
    std::array<int, 1024> data{};
    std::iota(data.begin(), data.end(), 1);
    VkDeviceSize size = 1024 * sizeof(int);


    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo1{};
    allocInfo1.usage = VMA_MEMORY_USAGE_CPU_ONLY;

//    Buffer buffer{g_allocator};
    VkBuffer handle;
    VmaAllocation allocation1;
//    vmaCreateBuffer(g_allocator, &bufferInfo, &allocInfo1, &handle, &allocation1, nullptr);

    VulkanDevice vulkanDevice{g_instance, g_physicalDevice, {}};
    vulkanDevice.createLogicalDevice({}, {});
//    vulkanDevice.allocator = g_allocator;
//    vmaCreateBuffer(vulkanDevice.allocator, &bufferInfo, &allocInfo1, &handle, &allocation1, nullptr);
//
    auto buffer = vulkanDevice.createBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY, size);
//    buffer.allocator = g_allocator;
//    auto buffer = VulkanBuffer{vulkanDevice.allocator, handle, allocation1, size, ""};
    void* dest;
    spdlog::info("mapping memory");
    vmaMapMemory(buffer.allocator, buffer.allocation, &dest);
    spdlog::info("copy data into buffer");
    memcpy(dest, data.data(), size);
    vmaUnmapMemory(buffer.allocator, buffer.allocation);

    spdlog::info("we made it here so we are good");

    return 0;
}