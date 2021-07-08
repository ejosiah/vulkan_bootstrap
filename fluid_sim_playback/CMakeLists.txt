include_directories(${CMAKE_CURRENT_LIST_DIR}/include)

file(GLOB_RECURSE CPP_FILES ${CMAKE_CURRENT_LIST_DIR}/include/*.*)
file(GLOB_RECURSE HPP_FILES ${CMAKE_CURRENT_LIST_DIR}/source/*.cpp)

add_executable(fluid_sim_playback ${CPP_FILES} ${HPP_FILES})
target_link_libraries(fluid_sim_playback VulkanBase ImGuiPlugin)