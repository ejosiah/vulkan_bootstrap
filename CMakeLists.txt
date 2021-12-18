cmake_minimum_required(VERSION 3.17)
project(vulkan_bootstrap VERSION 0.0.0)

enable_testing()

set(CMAKE_CXX_STANDARD 17)
find_program(GLSLC glslc REQUIRED)

if(CMAKE_GENERATOR STREQUAL "Xcode")
    add_compile_options(-Wno-nullability-completeness)
endif()

message(STATUS "setting up conan")
include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

include(cmake/utilities.cmake)

add_definitions(
        -DGLM_FORCE_RADIANS
        -DGLFW_INCLUDE_VULKAN
        -DGLM_FORCE_SWIZZLE
)

option(BUILD_EXAMPLES "build project examples" TRUE)


include_directories(3rdParty/include)
include_directories(VulkanBase/include)
include_directories(ImGuiPlugin/include)
include_directories(BulletPhysicsPlugin/include)
#include_directories(camera/include)

if(${BUILD_EXAMPLES})
    add_subdirectory(examples)
endif()

add_subdirectory(VulkanBase)
add_subdirectory(ImGuiPlugin)
add_subdirectory(BulletPhysicsPlugin)
#add_subdirectory(camera)