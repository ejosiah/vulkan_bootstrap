cmake_minimum_required(VERSION 3.17)
project(vulkan_bootstrap)

set(CMAKE_CXX_STANDARD 17)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

add_definitions(
        -DVK_USE_PLATFORM_WIN32_KHR
        -DGLFW_INCLUDE_VULKAN
#        -DGLM_FORCE_DEFAULT_ALIGNED_GENTYPES
)

file(GLOB_RECURSE CPP_FILES ${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp)
file(GLOB_RECURSE HPP_FILES ${CMAKE_CURRENT_SOURCE_DIR}/include/*.*)

find_package("Vulkan")

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

add_executable(vulkan_bootstrap ${CPP_FILES} ${HPP_FILES})
target_link_libraries(vulkan_bootstrap Vulkan::Vulkan ${CONAN_LIBS})