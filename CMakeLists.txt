cmake_minimum_required(VERSION 3.17)
project(vulkan_bootstrap VERSION 0.0.0)

set(CMAKE_CXX_STANDARD 17)

if(CMAKE_GENERATOR STREQUAL "Xcode")
    add_compile_options(-Wno-nullability-completeness)
endif()

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

add_definitions(
        -DGLM_FORCE_RADIANS
        -DGLFW_INCLUDE_VULKAN
        -DGLM_FORCE_SWIZZLE
)


include_directories(3rdParty/include)
include_directories(VulkanBase/include)
include_directories(ImGuiPlugin/include)

add_subdirectory(3rdParty)
add_subdirectory(VulkanBase)
add_subdirectory(ImGuiPlugin)
add_subdirectory(VulkanCube)
#add_subdirectory(VulkanCubeInstanced)
add_subdirectory(FontTest)
add_subdirectory(MeshLoading)
add_subdirectory(Primitives)
add_subdirectory(Demo)
add_subdirectory(ImGuiDemo)
add_subdirectory(ComputeDemo)
add_subdirectory(Cloth)
add_subdirectory(Playground)
add_subdirectory(RayTracerDemo)