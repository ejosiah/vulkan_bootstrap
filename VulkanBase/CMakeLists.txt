project(VulkanBase VERSION 0.0.0)

find_package(Vulkan)

add_definitions(
        -DGLM_FORCE_RADIANS
        -DGLFW_INCLUDE_VULKAN
        -DGLM_FORCE_SWIZZLE
)

file(GLOB_RECURSE CPP_FILES ${CMAKE_CURRENT_LIST_DIR}/source/*.cpp)
file(GLOB_RECURSE HPP_FILES ${CMAKE_CURRENT_LIST_DIR}/include/*.*)
file(GLOB_RECURSE TEST_FILES ${CMAKE_CURRENT_LIST_DIR}/tests/*.*)
file(GLOB_RECURSE GTEST_LIBS ${CONAN_LIB_DIRS_GTEST}/*.lib)

add_library(VulkanBase ${CPP_FILES} ${HPP_FILES})
target_link_libraries(VulkanBase Vulkan::Vulkan ${CONAN_LIBS})

include(GoogleTest)
add_executable(VulkanBaseTest ${HPP_FILES} ${TEST_FILES})
target_link_libraries(VulkanBaseTest PRIVATE VulkanBase Vulkan::Vulkan ${GTEST_LIBS} ${CONAN_LIBS})
gtest_add_tests(TARGET VulkanBaseTest)
