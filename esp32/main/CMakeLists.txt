idf_component_register(SRCS "main.c"
                       INCLUDE_DIRS "."
                       REQUIRES core)

littlefs_create_partition_image(games games/ FLASH_IN_PROJECT)
