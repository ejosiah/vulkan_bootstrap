project(VulkanBase VERSION 0.0.0)

find_package("Vulkan")
add_definitions(
        -DGLM_FORCE_RADIANS
        -DGLFW_INCLUDE_VULKAN
        -DGLM_FORCE_SWIZZLE
)

file(GLOB_RECURSE CPP_FILES ${CMAKE_CURRENT_LIST_DIR}/source/*.cpp)
file(GLOB_RECURSE HPP_FILES ${CMAKE_CURRENT_LIST_DIR}/include/*.*)

add_library(VulkanBase ${CPP_FILES} ${HPP_FILES})
target_link_libraries(VulkanBase 3rdParty Vulkan::Vulkan ${CONAN_LIBS})
