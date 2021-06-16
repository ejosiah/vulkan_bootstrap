include_directories(\${CMAKE_CURRENT_LIST_DIR}/include)

file(GLOB_RECURSE CPP_FILES \${CMAKE_CURRENT_LIST_DIR}/include/*.*)
file(GLOB_RECURSE HPP_FILES \${CMAKE_CURRENT_LIST_DIR}/source/*.cpp)

add_executable($name$ \${CPP_FILES} \${HPP_FILES})
target_link_libraries($name$ VulkanBase ImGuiPlugin)