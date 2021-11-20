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


include_directories(3rdParty/include)
include_directories(VulkanBase/include)
include_directories(ImGuiPlugin/include)
include_directories(BulletPhysicsPlugin/include)
include_directories(camera/include)

add_subdirectory(VulkanBase)
add_subdirectory(ImGuiPlugin)
add_subdirectory(VulkanCube)
add_subdirectory(BulletPhysicsPlugin)
add_subdirectory(FontTest)
add_subdirectory(MeshLoading)
add_subdirectory(Primitives)
add_subdirectory(Demo)
add_subdirectory(ImGuiDemo)
add_subdirectory(ComputeDemo)
add_subdirectory(Cloth)
add_subdirectory(Playground)
add_subdirectory(RayTracerDemo)
add_subdirectory(RayTracingImplicits)
add_subdirectory(MarchingCubes)
add_subdirectory(sph_fluid_sim)
add_subdirectory(fluid_sim_playback)
add_subdirectory(collision_detection)
add_subdirectory(examples)
add_subdirectory(camera)