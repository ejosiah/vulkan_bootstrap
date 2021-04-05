find_package("Vulkan")

file(GLOB_RECURSE CPP_FILES ${CMAKE_CURRENT_LIST_DIR}/source/*.cpp)
file(GLOB_RECURSE HPP_FILES ${CMAKE_CURRENT_LIST_DIR}/include/*.*)

add_library(3rdParty ${CPP_FILES} ${HPP_FILES})
target_link_libraries(3rdParty Vulkan::Vulkan ${CONAN_LIBS})