find_package("Vulkan")

message(STATUS "setting up conan")
include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

file(GLOB_RECURSE CPP_FILES ${CMAKE_CURRENT_LIST_DIR}/source/*.cpp)
file(GLOB_RECURSE HPP_FILES ${CMAKE_CURRENT_LIST_DIR}/include/*.*)

add_library(3rdParty ${CPP_FILES} ${HPP_FILES})
target_link_libraries(3rdParty Vulkan::Vulkan ${CONAN_LIBS})