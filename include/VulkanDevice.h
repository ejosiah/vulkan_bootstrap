#pragma once

#include "common.h"
#include "VulkanResource.h"

struct VulkanDevice{

    struct {
        std::optional<uint32_t> graphics;
        std::optional<uint32_t> compute;
        std::optional<uint32_t> transfer;
        std::optional<uint32_t> present;
    } queueFamilyIndex;

    struct {
        VkQueue graphics;
        VkQueue compute;
        VkQueue transfer;
        VkQueue present;
    } queues{};

    std::set<uint32_t> uniqueQueueIndices;

    VulkanDevice() = default;

    explicit VulkanDevice(VkInstance instance, VkPhysicalDevice pDevice)
    : instance(instance)
    , physicalDevice(pDevice)
    {
    }

    VulkanDevice(const VulkanDevice&) = delete;

    VulkanDevice(VulkanDevice&& source) noexcept{
        operator=(static_cast<VulkanDevice&&>(source));
    }

    VulkanDevice& operator=(const VulkanDevice&) = delete;

    VulkanDevice& operator=(VulkanDevice&& source) noexcept{
        physicalDevice = source.physicalDevice;
        logicalDevice = source.logicalDevice;
        queueFamilyIndex = source.queueFamilyIndex;
        queues = source.queues;
        allocator = source.allocator;
        instance = source.instance;

        source.physicalDevice = VK_NULL_HANDLE;
        source.logicalDevice = VK_NULL_HANDLE;
        source.allocator = VK_NULL_HANDLE;
        source.instance = VK_NULL_HANDLE;

        return *this;
    }

    ~VulkanDevice(){
        if(logicalDevice){
            vmaDestroyAllocator(allocator);
            vkDestroyDevice(logicalDevice, nullptr);
        }
    }

    inline void initQueueFamilies(VkQueueFlags queueFlags, VkSurfaceKHR surface = VK_NULL_HANDLE){
        auto queueFamily = getQueueFamilyProperties();
        for(uint32_t i = 0; i < queueFamily.size(); i++){
            if(!queueFamilyIndex.graphics && (queueFamily[i].queueFlags & queueFlags) == VK_QUEUE_GRAPHICS_BIT){
                queueFamilyIndex.graphics = i;
            }
            if(!queueFamilyIndex.compute && (queueFamily[i].queueFlags & queueFlags) == VK_QUEUE_COMPUTE_BIT){
               queueFamilyIndex.compute = i;
            }
            if(!queueFamilyIndex.transfer && (queueFamily[i].queueFlags & queueFlags) == VK_QUEUE_TRANSFER_BIT){
                queueFamilyIndex.transfer = i;
            }

            if(surface && !queueFamilyIndex.present) {
                VkBool32 present;
                vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &present);
                if (present) {
                    queueFamilyIndex.present = i;
                }
            }
            uniqueQueueIndices.insert(i);
        }
    }

    inline void createLogicalDevice(const VkPhysicalDeviceFeatures& enabledFeatures,
                                    const std::vector<const char*>& enabledExtensions,
                                    const std::vector<const char*>& enabledLayers = {},
                                    VkSurfaceKHR surface = VK_NULL_HANDLE,
                                    VkQueueFlags queueFlags = VK_QUEUE_GRAPHICS_BIT,
                                    void* pNext = VK_NULL_HANDLE){
                initQueueFamilies(queueFlags, surface);

        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        for(auto queueIndex : uniqueQueueIndices){
            float priority = 1.0f;
            VkDeviceQueueCreateInfo qCreateInfo{};
            qCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            qCreateInfo.queueFamilyIndex = queueIndex;
            qCreateInfo.queueCount = 1;
            qCreateInfo.pQueuePriorities = &priority;
            queueCreateInfos.push_back((qCreateInfo));
        }

        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.pNext = pNext;
        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pQueueCreateInfos = queueCreateInfos.data();
        createInfo.enabledLayerCount = static_cast<uint32_t>(enabledLayers.size());
        createInfo.ppEnabledLayerNames = enabledLayers.data();
        createInfo.enabledExtensionCount = static_cast<uint32_t>(enabledExtensions.size());
        createInfo.ppEnabledExtensionNames = enabledExtensions.data();
        createInfo.pEnabledFeatures = &enabledFeatures;

        ASSERT(vkCreateDevice(physicalDevice, &createInfo, nullptr, &logicalDevice));
        initQueues();

        VmaAllocatorCreateInfo allocatorInfo{};
        allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_2;
        allocatorInfo.instance = instance;
        allocatorInfo.physicalDevice = physicalDevice;
        allocatorInfo.device = logicalDevice;

        ASSERT(vmaCreateAllocator(&allocatorInfo, &allocator));
    }

    inline void initQueues(){
        if(queueFamilyIndex.graphics.has_value()){
            vkGetDeviceQueue(logicalDevice, *queueFamilyIndex.graphics, 0, &queues.graphics);
        }
        if(queueFamilyIndex.compute.has_value()) {
            vkGetDeviceQueue(logicalDevice, *queueFamilyIndex.compute, 0, &queues.compute);
        }
        if(queueFamilyIndex.transfer.has_value()) {
            vkGetDeviceQueue(logicalDevice, *queueFamilyIndex.transfer, 0, &queues.transfer);
        }
        if(queueFamilyIndex.present.has_value()) {
            vkGetDeviceQueue(logicalDevice, *queueFamilyIndex.present, 0, &queues.present);
        }
    }

    std::string name() const {
        return getProperties().deviceName;
    }

    inline uint32_t score() const {
        auto deviceProps = getProperties();
        uint32_t score = deviceProps.limits.maxImageDimension2D;
        if(deviceProps.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU){
            score += 1000;
        }else if(deviceProps.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU){
            score += 500;
        }else if(deviceProps.deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU){
            score += 100;
        }
        return score;
    }

    inline VkPhysicalDeviceProperties getProperties() const{
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(physicalDevice, &props);
        return props;
    }

    inline std::vector<VkQueueFamilyProperties> getQueueFamilyProperties() const {
       return get<VkQueueFamilyProperties>([&](uint32_t* size, VkQueueFamilyProperties* propsPtr){
          vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, size, propsPtr);
       });
    }

    std::vector<VkExtensionProperties> getExtensions(const char* layer = nullptr) const {
        return enumerate<VkExtensionProperties>([&](uint32_t* count, VkExtensionProperties* ptr){
            return vkEnumerateDeviceExtensionProperties(physicalDevice, layer, count, ptr);
        });
    }

    VkPhysicalDeviceMemoryProperties getMemoryProperties() const {
        VkPhysicalDeviceMemoryProperties memoryProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);
        return memoryProperties;
    }

    bool supportsMemoryType(VkMemoryPropertyFlags flags){
        auto memoryProps = getMemoryProperties();
        for(auto i = 0; i < memoryProps.memoryTypeCount; i++){
            if((memoryProps.memoryTypes[i].propertyFlags & flags)){
                return true;
            }
        }
        return false;
    }

    uint32_t findMemoryType(uint32_t memoryTypeBits, VkMemoryPropertyFlags memoryPropertyFlags){
        auto memoryProperties = getMemoryProperties();
        for(uint32_t memoryIndex = 0u; memoryIndex < memoryProperties.memoryTypeCount; memoryIndex++){
            if((( 1 << memoryIndex) & memoryTypeBits) && (memoryProperties.memoryTypes[memoryIndex].propertyFlags & memoryPropertyFlags) == memoryPropertyFlags){
                return memoryIndex;
            }
        }
        assert(false);
    }

    inline bool extensionSupported(const char* extension) noexcept {
        auto extensions = getExtensions();
        return std::any_of(begin(extensions), end(extensions), [&](auto& ext){
            return strcmp(extension, ext.extensionName) == 0;
        });
    }

    VulkanBuffer createBuffer(VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage, VkDeviceSize size, std::set<uint32_t> queueIndices = {}){
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        if(!queueIndices.empty()){
            std::vector<uint32_t> pIndices{queueIndices.begin(), queueIndices.end()};
            bufferInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
            bufferInfo.queueFamilyIndexCount = queueIndices.size();
            bufferInfo.pQueueFamilyIndices = pIndices.data();
        }else{
            bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }
        VkBuffer buffer;

        VmaAllocationCreateInfo allocInfo{};
        allocInfo.usage = memoryUsage;
        VmaAllocation allocation;

        ASSERT(vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &buffer, &allocation, nullptr));

        return VulkanBuffer{allocator, buffer, allocation, size};
    }

    operator VkDevice() const {
        return logicalDevice;
    }

    operator VkPhysicalDevice() const {
        return physicalDevice;
    }

    VkInstance instance = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice logicalDevice = VK_NULL_HANDLE;
    VmaAllocator allocator = VK_NULL_HANDLE;
};