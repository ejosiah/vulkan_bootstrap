#pragma once

#ifndef NDEBUG
constexpr bool debugMode = true;
#else
constexpr bool debugMode = false;
#endif

#include <string>
#include <string_view>
#include <array>
#include <vector>
#include <set>
#include <algorithm>
#include <optional>
#include <stdexcept>
#include <thread>
#include <chrono>
#include <memory>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vulkan/vulkan.h>
#include <spdlog/spdlog.h>
#include <chrono>
#include <vk_mem_alloc.h>

namespace chrono = std::chrono;

constexpr uint32_t WIDTH = 800;
constexpr uint32_t HEIGHT = 600;
constexpr std::chrono::seconds ONE_SECOND = std::chrono::seconds(1);

#define ASSERT(result) assert(result == VK_SUCCESS)
#define COUNT(sequence) static_cast<uint32_t>(sequence.size())

using cstring = const char*;

template<typename VkObject, typename Provider>
inline std::vector<VkObject> get(Provider&& provider){
    uint32_t size;
    provider(&size, static_cast<VkObject*>(nullptr));
    std::vector<VkObject> objects(size);
    provider(&size, objects.data());
    return objects;
}

template<typename VkObject, typename Provider>
inline std::vector<VkObject> enumerate(Provider&& provider){
    uint32_t size;
    provider(&size, static_cast<VkObject*>(nullptr));
    std::vector<VkObject> objects(size);
    VkResult result;
    do {
        result = provider(&size, objects.data());
    }while(result == VK_INCOMPLETE);
    ASSERT(result);
    return objects;
}

template<typename Func>
Func instanceProc(const std::string& procName, VkInstance instance){
    auto proc = reinterpret_cast<Func>(vkGetInstanceProcAddr(instance, procName.c_str()));
    if(!proc) throw std::runtime_error{procName + "Not found"};
    return proc;
}

#define DISABLE_COPY(TypeName) \
TypeName(const TypeName&) = delete; \
TypeName& operator=(const TypeName&) = delete;