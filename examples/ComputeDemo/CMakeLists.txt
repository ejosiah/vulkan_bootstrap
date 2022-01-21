
include_directories(include)

file(GLOB_RECURSE CPP_FILES ${CMAKE_CURRENT_LIST_DIR}/source/*.cpp)
file(GLOB_RECURSE HPP_FILES ${CMAKE_CURRENT_LIST_DIR}/include/*.*)

add_executable(ComputeDemo ${CPP_FILES} ${HPP_FILES})
target_link_libraries(ComputeDemo VulkanBase ImGuiPlugin)