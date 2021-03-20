find_package("Vulkan")
add_definitions(
        #        -DVK_USE_PLATFORM_WIN32_KHR
        -DGLFW_INCLUDE_VULKAN
        #        -DGLM_FORCE_DEFAULT_ALIGNED_GENTYPES
)

file(GLOB_RECURSE CPP_FILES ${CMAKE_CURRENT_LIST_DIR}/source/*.cpp)
file(GLOB_RECURSE HPP_FILES ${CMAKE_CURRENT_LIST_DIR}/include/*.*)

#add_executable(VulkanBase ${CPP_FILES} ${HPP_FILES})
add_library(VulkanBase ${CPP_FILES} ${HPP_FILES})
target_link_libraries(VulkanBase Vulkan::Vulkan ${CONAN_LIBS})

message(STATUS "calling PARENT_FOO from ${CMAKE_CURRENT_LIST_DIR} ${PARENT_FOO}")
