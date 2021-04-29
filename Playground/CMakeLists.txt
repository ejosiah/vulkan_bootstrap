file(GLOB_RECURSE CPP_FILES ${CMAKE_CURRENT_LIST_DIR}/*.cpp)

add_executable(Playground EXCLUDE_FROM_ALL ${CPP_FILES})
target_link_libraries(Playground VulkanBase)