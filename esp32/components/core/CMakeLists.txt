file(GLOB CORE_SRCS
    "${CMAKE_CURRENT_LIST_DIR}/../../../src/core/*.c"
    "${CMAKE_CURRENT_LIST_DIR}/../../../src/core/mappers/*.c")

idf_component_register(SRCS ${CORE_SRCS}
                       INCLUDE_DIRS "${CMAKE_CURRENT_LIST_DIR}/../../../src/core"
                                    "${CMAKE_CURRENT_LIST_DIR}/../../../src/core/mappers"
                       WHOLE_ARCHIVE)

target_compile_options(${COMPONENT_LIB} PRIVATE "-Wno-error")
