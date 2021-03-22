cmake_minimum_required(VERSION 3.17)
project(vulkan_bootstrap VERSION 0.0.0)

set(CMAKE_CXX_STANDARD 17)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()


include_directories(VulkanBase/include)

add_subdirectory(VulkanBase)
add_subdirectory(VulkanCube)
#add_subdirectory(VulkanCubeInstanced)
add_subdirectory(FontTest)
