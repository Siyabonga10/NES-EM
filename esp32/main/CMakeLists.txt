idf_component_register(SRCS "main.c"
                       INCLUDE_DIRS ".")

add_prebuilt_library(my_static_lib_target "lib/libcore.a")
littlefs_create_partition_image(games games FLASH_IN_PROJECT)
target_link_libraries(${COMPONENT_LIB} PRIVATE my_static_lib_target)

