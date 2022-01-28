if (NOT NO_OPENCL)
    find_package(OpenCL QUIET)
endif()

include_directories(include)

file(GLOB_RECURSE CPP_FILES ${CMAKE_CURRENT_LIST_DIR}/include/*.*)
file(GLOB_RECURSE HPP_FILES ${CMAKE_CURRENT_LIST_DIR}/source/*.cpp)

add_executable(Cloth ${CPP_FILES} ${HPP_FILES})
target_link_libraries(Cloth VulkanBase ImGuiPlugin)

if (OpenCL_FOUND)
    include_directories("${OpenCL_INCLUDE_DIRS}")
    add_definitions( -DOPENCL_FOUND=1 )
    target_link_libraries(Cloth ${OpenCL_LIBRARIES})
endif()