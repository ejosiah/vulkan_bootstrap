#pragma once
#include <stdexcept>
#include "common.h"
#include "VulkanExtensions.h"


struct VulkanDebug{

    VulkanDebug() = default;

    VulkanDebug(VkInstance instance): instance(instance){
        auto createInfo = debugCreateInfo();
        auto vkCreateDebugUtilsMessenger = instanceProc<PFN_vkCreateDebugUtilsMessengerEXT>("vkCreateDebugUtilsMessengerEXT", instance);
        auto res = vkCreateDebugUtilsMessenger(instance, &createInfo, nullptr, &debugMessenger);
        if(res != VK_SUCCESS) throw std::runtime_error{"Failed to create Debug messenger"};
    }

    VulkanDebug(const VulkanDebug&) = delete;

    VulkanDebug(VulkanDebug&& source) noexcept{
        operator=(static_cast<VulkanDebug&&>(source));
    }

    VulkanDebug& operator=(const VulkanDebug&) = delete;

    VulkanDebug& operator=(VulkanDebug&& source) noexcept{
        instance = source.instance;
        debugMessenger = source.debugMessenger;

        source.debugMessenger = VK_NULL_HANDLE;

        return *this;
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



    ~VulkanDebug(){
        if(debugMessenger) {
            auto DestroyDebugUtilsMessenger = instanceProc<PFN_vkDestroyDebugUtilsMessengerEXT>("vkDestroyDebugUtilsMessengerEXT", instance);
            DestroyDebugUtilsMessenger(instance, debugMessenger, nullptr);
        }
    }

    static void setObjectName(VkDevice& device, const uint64_t object, const std::string& name, VkObjectType type) {
        if constexpr (debugMode) {

            VkDebugUtilsObjectNameInfoEXT s{VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT, nullptr, type, object, name.c_str()};
            vkSetDebugUtilsObjectNameEXT(device, &s);

        }
    }

    static void setObjectName(VkDevice device, VkBuffer buffer, const std::string& name){
        setObjectName(device, (uint64_t)buffer, name, VK_OBJECT_TYPE_BUFFER);
    }

    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
    VkInstance instance = VK_NULL_HANDLE;
};