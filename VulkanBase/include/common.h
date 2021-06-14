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
#include <future>
#include <limits>
#include <algorithm>
#include <numeric>
#include <optional>
#include <variant>
#include <stdexcept>
#include <thread>
#include <chrono>
#include <random>
#include <functional>
#include <type_traits>
#include <memory>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <vulkan/vulkan.h>
#include <fmt/chrono.h>
#include <fmt/ranges.h>
#include <spdlog/spdlog.h>
#include <chrono>
#include "vk_mem_alloc.h"
#include "xforms.h"
#include "glm_format.h"
#include <filesystem>

namespace chrono = std::chrono;
namespace fs = std::filesystem;

using Flags = unsigned int;

constexpr float EPSILON = 0.000001;
constexpr float MAX_FLOAT = std::numeric_limits<float>::max();
constexpr float MIN_FLOAT = std::numeric_limits<float>::min();
constexpr std::chrono::seconds ONE_SECOND = std::chrono::seconds(1);


#define ASSERT(result) assert(result == VK_SUCCESS)
#define COUNT(sequence) static_cast<uint32_t>(sequence.size())

using cstring = const char*;

inline bool closeEnough(float x, float y, float epsilon = 1E-3) {
    return fabs(x - y) <= epsilon * (abs(x) + abs(y) + 1.0f);
}

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

inline bool hasStencil(VkFormat format) {
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
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

#define DISABLE_MOVE(TypeName) \
TypeName(TypeName&&) = delete; \
TypeName& operator=(TypeName&&) = delete;

constexpr uint32_t alignedSize(uint32_t value, uint32_t alignment){
    return (value + alignment - 1) & ~(alignment - 1);
}

// helper type for the visitor #4
template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
// explicit deduction guide (not needed as of C++20)
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;