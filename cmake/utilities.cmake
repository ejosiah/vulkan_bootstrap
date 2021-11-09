include_guard(GLOBAL)

function(compile_glsl)
    set(noValues "")
    set(singleValues SRC_FILE OUT_FILE SPV_VERSION)
    set(multiValues "")

    cmake_parse_arguments(COMPILE "${noValues}" "${singleValues}" "${multiValues}" ${ARGN})

    if(NOT COMPILE_SPV_VERSION)
        set(COMPILE_SPV_VERSION spv1.5)
    endif()

    if(NOT EXISTS ${COMPILE_SRC_FILE})
        message(FATAL_ERROR "${COMPILE_SRC_FILE} not found")
    endif()

    set(GLSLC_COMMAND ${GLSLC} --target-spv=${COMPILE_SPV_VERSION} ${COMPILE_SRC_FILE} -o ${COMPILE_OUT_FILE})


    execute_process(
        COMMAND ${GLSLC} --target-spv=${COMPILE_SPV_VERSION} ${COMPILE_SRC_FILE} -o ${COMPILE_OUT_FILE}
        RESULT_VARIABLE GLSLC_COMPILE_OUTPUT
    )
    get_filename_component(SHADER_SRC_FILE ${COMPILE_SRC_FILE} NAME)
    if(${GLSLC_COMPILE_OUTPUT})
        message(FATAL_ERROR "compile failed for ${SHADER_SRC_FILE}, reason: ${GLSLC_COMPILE_OUTPUT}")
    endif()

endfunction()

function(compile_glsl_directory)
    set(noValues "")
    set(singleValues SRC_DIR OUT_DIR)
    set(multiValues "")

    cmake_parse_arguments(COMPILE "${noValues}" "${singleValues}" "${multiValues}" ${ARGN})

    file(GLOB GLSL_SOURCE_FILES
        "${COMPILE_SRC_DIR}/*.vert"
        "${COMPILE_SRC_DIR}/*.frag"
        "${COMPILE_SRC_DIR}/*.geom"
        "${COMPILE_SRC_DIR}/*.comp"
    )

    foreach(SHADER_SOURCE IN ITEMS ${GLSL_SOURCE_FILES})
        get_filename_component(SHADER_FILE_NAME ${SHADER_SOURCE} NAME)
        set(SPV_FILE "${COMPILE_OUT_DIR}/${SHADER_FILE_NAME}.spv")
        compile_glsl(SRC_FILE ${SHADER_SOURCE} OUT_FILE ${SPV_FILE})
    endforeach()

endfunction()