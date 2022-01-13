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

float zndc(float z, float n, float f){
   float num = (z + n) * f;
   float denum = z * (f - n);
   return num / denum;
}

float ndc_to_d(float z, float n, float f){
    float num = f * n;
    float denum = z * (f - n) - f;
    return num/denum;
}

constexpr uint32_t flags =
        aiProcess_GenSmoothNormals
        | aiProcess_Triangulate
        | aiProcess_CalcTangentSpace
        | aiProcess_JoinIdenticalVertices
        | aiProcess_ValidateDataStructure;


using namespace Assimp;

uint32_t numMeshes(const aiScene* scene, const aiNode* node){
    if(node->mNumChildren < 1){
        return node->mNumMeshes;
    }
    auto count = node->mNumMeshes;
    for(auto i = 0; i < node->mNumChildren; i++){
        count += numMeshes(scene, node->mChildren[i]);
    }
    return count;
}

uint32_t nodeDepth(const aiScene* scene, const aiNode* node){
    if(node->mNumChildren < 1) return 1;

    uint32_t maxDepth = std::numeric_limits<uint32_t>::min();
    for(auto i = 0; i < node->mNumChildren; i++){
        uint32_t depth = 1 + nodeDepth(scene, node->mChildren[i]);
        maxDepth = std::max(depth, maxDepth);
    }
    return maxDepth;
}

void logBones(const aiScene* scene){


    std::function<void(aiNode*)> logger = [&](aiNode* node) {

        if(node->mNumMeshes > 0){
            for(int i = 0; i < node->mNumMeshes; i++){
                const auto mesh = scene->mMeshes[node->mMeshes[i]];
                if(mesh->HasBones()){
                    fmt::print("num bones: {}\n", mesh->mNumBones);
                    for(int bid = 0; bid < mesh->mNumBones; bid++){
                        const auto bone = mesh->mBones[bid];
                        fmt::print("\t{}\n", bone->mName.C_Str());
                        fmt::print("\t\tweights: {}\n", bone->mNumWeights);
//                        for(int wid = 0; wid < bone->mNumWeights; wid++){
//                            fmt::print("\t\tvertexId: {}, weight: {}\n", bone->mWeights[wid].mVertexId, bone->mWeights[wid].mWeight);
//                        }
                    }
                }
            }
        }
        for(int i = 0; i < node->mNumChildren; i++){
            logger(node->mChildren[i]);
        }
    };
    fmt::print("bones:\n");
    logger(scene->mRootNode);
}

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/string_generator.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <sstream>


template<>
struct fmt::formatter<boost::uuids::uuid>{

    constexpr auto parse(format_parse_context& ctx) {
        auto it = ctx.begin();
        it++;
        return it;
    }

    template <typename FormatContext>
    auto format(const boost::uuids::uuid& uuid, FormatContext& ctx) {
        return format_to(ctx.out(), "{}", to_string(uuid));
    }
};

using gen = boost::uuids::random_generator;

inline glm::mat4 qLeft(const glm::quat& q){
    return {
            {q.w, -q.x, -q.y, -q.z},
            {q.x, q.w,  -q.z, q.y},
            {q.y, q.z,  q.w,  -q.x},
            {q.z, -q.y, q.x,  q.w}
    };
}

inline glm::mat4 qRight(const glm::quat& q){
    return {
            {q.w, -q.x, -q.y, -q.z},
            {q.x,  q.w,  q.z, -q.y},
            {q.y, -q.z,  q.w,  q.x},
            {q.z,  q.y, -q.x,  q.w}
    };
}

int main() {

    glm::quat q(1, 0, 0, 0);
    auto axis = glm::axis(q);
    auto angle = glm::angle(q);
    fmt::print("axis: {}, angle: {}", axis, glm::degrees(angle));
    return 0;
}