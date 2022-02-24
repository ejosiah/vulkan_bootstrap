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
#include "Mesh.h"
#include <fstream>
#include "halfedge.hpp"
#include <bitset>
#include <meshoptimizer.h>

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
    spdlog::info("\n\n");

    for(int i = 0; i <= 15; i++){
        int inputSize = 1 << (i + 8);
        spdlog::info("{:<20}{:<20}{:<20}{:<20}{:<20}{:<20}", "Operation", "Input Size", "Average Time (ms)", "Min Time (ms)", "Max Time (ms)", "Median Time (ms)");
        float total = 0.0f;
        for(const auto& [op, stats] : statistics){
            total += stats[i].meanValue;
            spdlog::info("{:<20}{:<20}{:<20}{:<20}{:<20}{:<20}", op, inputSize, stats[i].meanValue, stats[i].minValue, stats[i].maxValue, stats[i].medianValue);
        }
        spdlog::info("{:<20}{:<20}{:<20}{:<20}{:<20}{:<20}\n", "total", "", total, "", "", "", "");
    }
    spdlog::info("\n\n");
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

std::string saveToOFF(const std::string& filename){
    std::fstream  fin(filename.data());
    if(!fin.good()) throw std::runtime_error{"Failed to open file: " + filename};

    std::vector<mesh::Mesh> meshes;
    mesh::load(meshes, filename);
    fin.close();

    auto numVertices = 0;
    auto numTriangles = 0;
    for(auto& mesh : meshes){
        numVertices += mesh.vertices.size();
        numTriangles += mesh.indices.size()/3;
    }
    spdlog::info("loaded model with {} vertices and {} triangles", numVertices, numTriangles);

    auto getName = [](fs::path p){
        auto file = p.string();
        auto start = file.find_last_of('\\') + 1;
        auto end = file.find_last_of('.');
        auto count = end - start;
        return file.substr(start, count);
    };

    spdlog::info("saving model to OFF format...");
    auto name = getName(filename) + ".off";
    auto out_filename = "C:\\temp\\" + name;
    auto startTime = std::chrono::steady_clock::now();

    std::ofstream fout(out_filename);
    if(fout.bad()) throw std::runtime_error{"unable to open for writing, filename: " + out_filename};

    fout << "OFF" << "\n";
//    fout << "# " << name << "\n\n";
    fout << numVertices << " " << numTriangles << " " << "0\n";

    for(auto& mesh : meshes){
        for(auto& vertex : mesh.vertices){
            fout << " " << vertex.position.x << " " << vertex.position.y << " " << vertex.position.z << "\n";
        }
        fout.flush();
    }

    for(auto& mesh : meshes){
        auto size = mesh.indices.size();
        for(int i = 0; i < size; i+= 3){
            fout << "3 ";
            fout << mesh.indices[i + 0] << " ";
            fout << mesh.indices[i + 1] << " ";
            fout << mesh.indices[i + 2] << " ";
            fout << "\n";
        }
        fout.flush();
    }
    fout.close();
    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<chrono::milliseconds>(endTime - startTime).count();
    spdlog::info("model saved to {} in {} seconds", out_filename, duration/1000.0f);

    return out_filename;
}

void logBones(const aiScene* scene){


    std::function<void(aiNode*)> logger = [&](aiNode* node) {

        if(node->mNumMeshes > 0){
            for(int i = 0; i < node->mNumMeshes; i++){
                const auto mesh = scene->mMeshes[node->mMeshes[i]];
                if(mesh->HasBones()){
                    spdlog::info("num bones: {}\n", mesh->mNumBones);
                    for(int bid = 0; bid < mesh->mNumBones; bid++){
                        const auto bone = mesh->mBones[bid];
                        spdlog::info("\t{}\n", bone->mName.C_Str());
                        spdlog::info("\t\tweights: {}\n", bone->mNumWeights);
//                        for(int wid = 0; wid < bone->mNumWeights; wid++){
//                            spdlog::info("\t\tvertexId: {}, weight: {}\n", bone->mWeights[wid].mVertexId, bone->mWeights[wid].mWeight);
//                        }
                    }
                }
            }
        }
        for(int i = 0; i < node->mNumChildren; i++){
            logger(node->mChildren[i]);
        }
    };
    spdlog::info("bones:\n");
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

void halfEdgeMeshStuff(){
    std::string filename = R"(C:\Users\Josiah\OneDrive\media\models\dragon.obj)";
    std::vector<mesh::Mesh> meshes;

    auto start = chrono::high_resolution_clock::now();
    mesh::load(meshes, filename);
    auto end = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::seconds>(end - start).count();
    spdlog::info("mesh loaded in {} seconds\n", duration);

    auto& mesh = meshes.front();
    auto& indices = mesh.indices;
    spdlog::info("num vertices loaded {}\n", mesh.vertices.size());


    auto cube = primitives::cube();


    start = chrono::high_resolution_clock::now();
    HalfEdgeMesh<Vertex, 0, 4> halfEdgeMesh{mesh.indices.data(), mesh.indices.size(), mesh.vertices.data()};
    end = chrono::high_resolution_clock::now();
    duration = chrono::duration_cast<chrono::seconds>(end - start).count();
    spdlog::info("half edges generated in {} seconds", duration);
}

int findQueueFamily(std::vector<VkQueueFamilyProperties> queueFamilies, VkQueueFlagBits queueFlagBits){
    std::vector<std::pair<int, VkQueueFamilyProperties>> matches;
    for(auto i = 0; i < queueFamilies.size(); i++){
        if(queueFamilies[i].queueFlags & queueFlagBits){
            matches.emplace_back(i, queueFamilies[i]);
        }
    }
    std::sort(matches.begin(), matches.end(), [](const auto& lhs, const auto& rhs){
        return
            std::bitset<32>{lhs.second.queueFlags}.count() < std::bitset<32>{rhs.second.queueFlags}.count();
    });
    return matches.empty() ? -1 : matches.front().first;
}

void formatTilingUsageCombo(){
    std::map<VkFormat, std::string> formats{};
    formats[VK_FORMAT_R8G8B8_SRGB] = "VK_FORMAT_R8G8B8_SRGB";
    formats[VK_FORMAT_R8G8B8A8_SRGB] = "VK_FORMAT_R8G8B8A8_SRGB";
    formats[VK_FORMAT_R8G8B8A8_UNORM] = "VK_FORMAT_R8G8B8A8_UNORM";
    formats[VK_FORMAT_R16G16B16_SFLOAT] = "VK_FORMAT_R16G16B16_SFLOAT";
    formats[VK_FORMAT_R16G16B16A16_SFLOAT] = "VK_FORMAT_R16G16B16A16_SFLOAT";
    formats[VK_FORMAT_R32G32B32_SFLOAT] = "VK_FORMAT_R32G32B32_SFLOAT";
    formats[VK_FORMAT_R32G32B32A32_SFLOAT] = "VK_FORMAT_R32G32B32A32_SFLOAT";
    formats[VK_FORMAT_R16_SFLOAT] = "VK_FORMAT_R16_SFLOAT";
    formats[VK_FORMAT_R16G16_SFLOAT] = "VK_FORMAT_R16_SFLOAT";
    formats[VK_FORMAT_R32_SFLOAT] = "VK_FORMAT_R32_SFLOAT";
    formats[VK_FORMAT_R32G32_SFLOAT] = "VK_FORMAT_R32_SFLOAT";

    std::map<VkImageType, std::string> imageTypes{};
    imageTypes[VK_IMAGE_TYPE_2D] = "VK_IMAGE_TYPE_2D";

    std::map<VkImageTiling, std::string> imageTilings{};
    imageTilings[VK_IMAGE_TILING_OPTIMAL] = "VK_IMAGE_TILING_OPTIMAL";
    imageTilings[VK_IMAGE_TILING_LINEAR] = "VK_IMAGE_TILING_LINEAR";

    std::map<VkImageUsageFlags, std::string> imageUsages{};
    imageUsages[VK_IMAGE_USAGE_SAMPLED_BIT] = "VK_IMAGE_USAGE_SAMPLED_BIT";
//    imageUsages[VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT] = "VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT";
//    imageUsages[VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT] = "VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT";

    for(auto [format, fname] : formats){
        for(auto [imageType, tname] : imageTypes){
            for(auto [tiliing, tiName] : imageTilings){
                for(auto [usage, uname] : imageUsages){
                    VkImageFormatProperties  imageProps;
                    auto res = vkGetPhysicalDeviceImageFormatProperties(g_device, format, imageType, tiliing, usage, 0, &imageProps);
                    if(res == VK_ERROR_FORMAT_NOT_SUPPORTED){
//                        fmt::print("{} - {} - {} - {} combination not supported\n\n", fname, tname, tiName, uname);
                        continue;
                    }
                    fmt::print("{} - {} - {} - {} props:\n", fname, tname, tiName, uname);
                    fmt::print("\tmax extent: [{}, {}, {}]\n", imageProps.maxExtent.width, imageProps.maxExtent.height, imageProps.maxExtent.depth);
                    fmt::print("\tmax mip levels: {}\n", imageProps.maxMipLevels);
                    fmt::print("\tmax array layers: {}\n", imageProps.maxArrayLayers);
                    fmt::print("\tsample count: {}\n", imageProps.sampleCounts);
                    fmt::print("\tmax resource size: {}\n\n", imageProps.maxResourceSize);
                }
            }
        }
    }
}

int main() {

   std::vector<mesh::Mesh> meshes;
   mesh::load(meshes, R"(C:\Users\Josiah\OneDrive\media\models\Lucy-statue\metallic-lucy-statue-stanford-scan.obj)");
   fmt::print("num meshes {}\n", meshes.size());
   auto cube = primitives::cube();

   auto indexCount = cube.indices.size();
   auto vertexCount = cube.vertices.size();
   fmt::print("pre optimization:\t");
   fmt::print("\tvertex count: {}, index count : {}\n", indexCount, vertexCount);

   std::vector<uint32_t> remappedIndices(indexCount);

   vertexCount = meshopt_generateVertexRemap(remappedIndices.data(), cube.indices.data(), indexCount, cube.vertices.data(), vertexCount, sizeof(Vertex));

    indexCount = remappedIndices.size();
    fmt::print("post optimization:\t");
    fmt::print("\tvertex count: {}, index count : {}", indexCount, vertexCount);
    return 0;
}