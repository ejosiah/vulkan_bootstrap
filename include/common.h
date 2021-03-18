#pragma once

#ifndef NDEBUG
constexpr bool debugMode = true;
#else
constexpr bool debugMode = false;
#endif

// TODO revite this include
#include <string>
#include <string_view>
#include <array>
#include <vector>
#include <map>
#include <set>
#include <atomic>
#include <queue>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <limits>
#include <algorithm>
#include <optional>
#include <stdexcept>
#include <thread>
#include <chrono>
#include <memory>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <vulkan/vulkan.h>
#include <spdlog/spdlog.h>
#include <chrono>
#include <vk_mem_alloc.h>
namespace chrono = std::chrono;

using Flags = unsigned int;

constexpr float EPSILON = 0.000001;
constexpr uint32_t WIDTH = 800;
constexpr uint32_t HEIGHT = 600;
constexpr std::chrono::seconds ONE_SECOND = std::chrono::seconds(1);

#define ASSERT(result) assert(result == VK_SUCCESS)
#define COUNT(sequence) static_cast<uint32_t>(sequence.size())

using cstring = const char*;

inline bool closeEnough(float x, float y) { return abs(x - y) <= EPSILON * (abs(x) + abs(y) + 1.0f); }

inline glm::quat fromAxisAngle(const glm::vec3& axis, const float angle) {
    float w = cos(glm::radians(angle) / 2);
    glm::vec3 xyz = axis * sin(glm::radians(angle) / 2);
    return glm::quat(w, xyz);
}


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

template <typename T>
inline void dispose(T& t){
    T temp = std::move(t);
}

#define DISABLE_COPY(TypeName) \
TypeName(const TypeName&) = delete; \
TypeName& operator=(const TypeName&) = delete;