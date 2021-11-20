include_directories(${CMAKE_CURRENT_LIST_DIR}/include)

file(GLOB_RECURSE CPP_FILES ${CMAKE_CURRENT_LIST_DIR}/include/*.*)
file(GLOB_RECURSE HPP_FILES ${CMAKE_CURRENT_LIST_DIR}/source/*.cpp)

add_library(camera ${CPP_FILES} ${HPP_FILES})
