#include <fmt/format.h>
#include <VulkanInstance.h>
#include <VulkanDevice.h>
#include <VulkanDebug.h>
#include <VulkanShaderModule.h>
#include <mutex>

std::mutex g_mutex;
VkInstance g_instance = VK_NULL_HANDLE;
VkPhysicalDevice g_physicalDevice = VK_NULL_HANDLE;
VkDevice g_device = VK_NULL_HANDLE;
VkQueue g_queue = VK_NULL_HANDLE;
uint32_t g_queueFamilyIndex = VK_NULL_HANDLE;
VkDebugUtilsMessengerEXT g_debugMessenger;
VkDebugUtilsMessengerCreateInfoEXT g_debugInfo;
VkPipelineLayout g_layout = VK_NULL_HANDLE;
VkPipeline g_Pipeline = VK_NULL_HANDLE;
VkCommandPool g_commandPool = VK_NULL_HANDLE;

const std::vector<const char*> g_validationLayers {
        "VK_LAYER_KHRONOS_validation"
};

static  VkBool32 VKAPI_PTR debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT           messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT                  messageTypes,
        const VkDebugUtilsMessengerCallbackDataEXT*      pCallbackData,
        void*                                            pUserData){


    if(messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT){
        printf("info: %s\n", pCallbackData->pMessage);
    }
    if(messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT){
        printf("warning: %s\n", pCallbackData->pMessage);
    }

    if(messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT){
        printf("error: %ss\n", pCallbackData->pMessage);
    }
    return VK_FALSE;

}

void initDebugInfo(){
    g_debugInfo = VkDebugUtilsMessengerCreateInfoEXT{};
    g_debugInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    g_debugInfo.messageSeverity =
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    g_debugInfo.messageType =
            VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
    g_debugInfo.pfnUserCallback = debugCallback;
}


void initInstance(){
    const std::vector<const char*> instanceExtensions{
            VK_EXT_DEBUG_UTILS_EXTENSION_NAME
    };

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.apiVersion = VK_API_VERSION_1_2;
    appInfo.applicationVersion = VK_MAKE_VERSION(0, 0, 1);
    appInfo.engineVersion = VK_MAKE_VERSION(0,  0, 1);
    appInfo.pApplicationName = "Nsight Aftermath test";
    appInfo.pEngineName = "";

    VkInstanceCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    info.pNext = &g_debugInfo;
    info.pApplicationInfo = &appInfo;
    info.enabledExtensionCount = COUNT(instanceExtensions);
    info.ppEnabledExtensionNames = instanceExtensions.data();
    info.enabledLayerCount = COUNT(g_validationLayers);
    info.ppEnabledLayerNames = g_validationLayers.data();

    ASSERT(vkCreateInstance(&info, nullptr, &g_instance));
    printf("instance created...\n");
}

void createDebugMessenger(){
    assert(g_instance);
    auto vkCreateDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(
            g_instance, "vkCreateDebugUtilsMessengerEXT"));
    if(vkCreateDebugUtilsMessengerEXT == nullptr){
        printf("unable to retrieve create debut utils messenger function pointer\n");
        exit(120);
    }

    auto result = vkCreateDebugUtilsMessengerEXT(g_instance, &g_debugInfo, nullptr, &g_debugMessenger);
    if(result != VK_SUCCESS){
        printf("unable to create debug messenger\n");
        exit(140);
    }
}

void createDevice(){
    assert(g_instance);

    uint32_t count = 0;

    VkResult result;
    do {
        printf("enumerating devices\n");
        result = vkEnumeratePhysicalDevices(g_instance, &count, VK_NULL_HANDLE);
    }while(result == VK_INCOMPLETE);
    printf("%d devices found\n", count);

    std::vector<VkPhysicalDevice> physicalDevices(count);
    vkEnumeratePhysicalDevices(g_instance, &count, physicalDevices.data());
    g_physicalDevice = physicalDevices.front();

    vkGetPhysicalDeviceQueueFamilyProperties(g_physicalDevice, &count, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilyProperties(count);
    vkGetPhysicalDeviceQueueFamilyProperties(g_physicalDevice, &count, queueFamilyProperties.data());

    for(uint32_t i = 0; i < count; i++){
        if(queueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT){
            g_queueFamilyIndex = i;
            break;
        }
    }

    float priority = 1;
    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = g_queueFamilyIndex;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &priority;

    const std::vector<const char*> deviceExtensions{};
    VkPhysicalDeviceFeatures enabledFeatures{};

    VkDeviceCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    info.queueCreateInfoCount = 1;
    info.pQueueCreateInfos = &queueCreateInfo;
    info.enabledExtensionCount = COUNT(deviceExtensions);
    info.ppEnabledExtensionNames = deviceExtensions.data();
    info.pEnabledFeatures = &enabledFeatures;
    info.enabledLayerCount = COUNT(g_validationLayers);
    info.ppEnabledLayerNames = g_validationLayers.data();

    result = vkCreateDevice(g_physicalDevice, &info, nullptr, &g_device);
    if(result != VK_SUCCESS){
        printf("unable to create device\n");
        exit(500);
    }
    vkGetDeviceQueue(g_device, g_queueFamilyIndex, 0, &g_queue);
}

void createCommandPool(){
    VkCommandPoolCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    info.queueFamilyIndex = g_queueFamilyIndex;

    ASSERT(vkCreateCommandPool(g_device, &info, nullptr, &g_commandPool));
}

void createComputePipeline(){
    auto source = ShaderModule{ "../../data/shaders/crash.comp.spv", g_device};
    VkPipelineShaderStageCreateInfo stageInfo{};
    stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    stageInfo.module = source;
    stageInfo.pName = "main";

    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    ASSERT(vkCreatePipelineLayout(g_device, &layoutInfo, nullptr, &g_layout));

    VkComputePipelineCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    createInfo.stage = stageInfo;
    createInfo.layout = g_layout;

    ASSERT(vkCreateComputePipelines(g_device, VK_NULL_HANDLE, 1, &createInfo, nullptr, &g_Pipeline));
}

void cleanup(){
    vkDestroyPipeline(g_device, g_Pipeline, nullptr);
    vkDestroyPipelineLayout(g_device, g_layout, nullptr);

    vkDestroyCommandPool(g_device, g_commandPool, nullptr);
    vkDestroyDevice(g_device, nullptr);
    auto vkDestroyDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(g_instance, "vkDestroyDebugUtilsMessengerEXT"));
    if(!vkDestroyDebugUtilsMessengerEXT){
        printf("unable to retrieve destroy debug utils function ptr\n");
    }
    vkDestroyDebugUtilsMessengerEXT(g_instance, g_debugMessenger, nullptr);
    vkDestroyInstance(g_instance, nullptr);
}

using uint = uint32_t;

glm::vec2 hammersleySquare(uint i, const uint N) {
    glm::vec2 P;
    P.x = float(i) * (1.0 / float(N));

    i = (i << 16u) | (i >> 16u);
    i = ((i & 0x55555555u) << 1u) | ((i & 0xAAAAAAAAu) >> 1u);
    i = ((i & 0x33333333u) << 2u) | ((i & 0xCCCCCCCCu) >> 2u);
    i = ((i & 0x0F0F0F0Fu) << 4u) | ((i & 0xF0F0F0F0u) >> 4u);
    i = ((i & 0x00FF00FFu) << 8u) | ((i & 0xFF00FF00u) >> 8u);
    P.y = float(i) * 2.3283064365386963e-10; // / 0x100000000

    return P;
}

using namespace glm;

void computeOrthonormalBasis(vec3 N, vec3& Nt, vec3& Nb)
{

    Nt = normalize(((abs(N.z) > 0.99999f) ? vec3(-N.x * N.y, 1.0f - N.y * N.y, -N.y * N.z) :
                    vec3(-N.x * N.z, -N.y * N.z, 1.0f - N.z * N.z)));
    Nb = cross(Nt, N);
}

void othonormalBasis(vec3& normal, vec3& tangent, vec3& binormal){
    normal = normalize(normal);
    vec3 a;
    if(abs(normal.x) > 0.9){
        a = vec3(0, 1, 0);
    }else {
        a = vec3(1, 0, 0);
    }
    binormal = normalize(cross(normal, a));
    tangent = cross(normal, binormal);
}

vec3 remap(vec3 value, vec3 oldMin, vec3 oldMax, vec3 newMin, vec3 newMax){
    return (((value - oldMin) / (oldMax - oldMin)) * (newMax - newMin)) + newMin;
}

const float gridSpacing = 0.2;
const ivec3 resolution = ivec3(10, 10, 1);

ivec3 getBucketIndex(vec3 point){
    ivec3 bucketIndex;

    bucketIndex.x = int(floor(point.x/gridSpacing));
    bucketIndex.y = int(floor(point.y/gridSpacing));
    bucketIndex.z = int(floor(point.z/gridSpacing));

    return bucketIndex;
}

ivec3 wrapAround(ivec3 bucketIndex){
    auto wrapped = ivec3(mod(vec3(bucketIndex), vec3(resolution)));

    if(wrapped.x < 0) wrapped.x += resolution.x;
    if(wrapped.y < 0) wrapped.y += resolution.y;
    if(wrapped.z < 0) wrapped.z += resolution.z;

    return wrapped;
}

int toHashKey(ivec3 bucketIndex){
    auto wrapped = ivec3(mod(vec3(bucketIndex), vec3(resolution)));

    if(wrapped.x < 0) wrapped.x += resolution.x;
    if(wrapped.y < 0) wrapped.y += resolution.y;
    if(wrapped.z < 0) wrapped.z += resolution.z;

    return (wrapped.z * resolution.y + wrapped.y) * resolution.x + wrapped.x;
//    return (wrapped.y * resolution.x + wrapped.x) * resolution.y + wrapped.z;
}


int main() {
    std::array<std::tuple<vec3, vec3, vec3>, 100> buckets;
    for(float r = -0.9; r < 1; r += 0.2 ){
        for(float c = -0.9; c < 1; c += 0.2 ){
            vec3 point{c, r, 0};
            auto bucketIndex = getBucketIndex(point);
            auto wrappedIndex = wrapAround(bucketIndex);
            auto key = toHashKey(bucketIndex);
            buckets[key] = std::make_tuple(point, vec3(bucketIndex), vec3(wrappedIndex));
        }
    }

    for(int i = 0; i < 100; i++){
        auto [point, bucketIndex, wrappedIndex] = buckets[i];
        fmt::print("key: {} -> point{}, bucketIndex{}, wrappedIndex{}\n", i, point, bucketIndex, wrappedIndex);
    }

}