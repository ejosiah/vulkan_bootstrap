if (NOT NO_OPENCL)
    find_package(OpenCL QUIET)
endif()

include_directories(${CMAKE_CURRENT_LIST_DIR}/include)

file(GLOB_RECURSE CPP_FILES ${CMAKE_CURRENT_LIST_DIR}/include/*.*)
file(GLOB_RECURSE HPP_FILES ${CMAKE_CURRENT_LIST_DIR}/source/*.cpp)
file(GLOB_RECURSE HPP_FILES_TEST ${CMAKE_CURRENT_LIST_DIR}/source/*.cpp)

file(GLOB_RECURSE TEST_FILES ${CMAKE_CURRENT_LIST_DIR}/tests/*.*)
list(REMOVE_ITEM HPP_FILES_TEST ${CMAKE_CURRENT_LIST_DIR}/source/main.cpp)

file(GLOB_RECURSE GTEST_LIBS ${CONAN_LIB_DIRS_GTEST}/*.lib)

set(GLSL_SOURCE_DIR "shaders")
set(SPV_DIR "${CMAKE_CURRENT_LIST_DIR}/spv")
compile_glsl_directory(SRC_DIR "${GLSL_SOURCE_DIR}" OUT_DIR "${SPV_DIR}" INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/data/shaders")

add_executable(fluid_dynamics ${CPP_FILES} ${HPP_FILES})
target_link_libraries(fluid_dynamics VulkanBase ImGuiPlugin)

if (OpenCL_FOUND)
    include_directories("${OpenCL_INCLUDE_DIRS}")
    add_definitions( -DOPENCL_FOUND=1 )
    target_link_libraries(fluid_dynamics ${OpenCL_LIBRARIES})
endif()


include(GoogleTest)
add_executable(fluid_sim_tests ${HPP_FILES_TEST} ${TEST_FILES})
target_link_libraries(fluid_sim_tests PRIVATE VulkanBase ImGuiPlugin  ${GTEST_LIBS} ${CONAN_LIBS})
gtest_discover_tests(fluid_sim_tests TEST_PREFIX FluidSim.)