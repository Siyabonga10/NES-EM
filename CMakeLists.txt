cmake_minimum_required(VERSION 3.10)
project(NesEmulator C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Debug)

include(FetchContent)

FetchContent_Declare(
    unity
    GIT_REPOSITORY https://github.com/ThrowTheSwitch/Unity.git
    GIT_TAG v2.5.2
)
FetchContent_MakeAvailable(unity)

FetchContent_Declare(
  raylib
  SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps/raylib
)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES    OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(raylib)

file(GLOB_RECURSE CORE_SOURCES "src/core/*.c")

# Main emulator
add_executable(NesEmulator src/main.c ${CORE_SOURCES})
target_include_directories(NesEmulator PRIVATE src)
target_link_libraries(NesEmulator PRIVATE raylib)

# Mapper unit tests — no raylib, no main.c, no audio
add_executable(test_mappers test/mappers/test_mappers.c ${CORE_SOURCES})
target_include_directories(test_mappers PRIVATE src)
target_link_libraries(test_mappers PRIVATE unity)

# Mapper integration runner — full emulator + raylib
add_executable(mapper_runner test/mapper_runner.c ${CORE_SOURCES})
target_include_directories(mapper_runner PRIVATE src)
target_link_libraries(mapper_runner PRIVATE raylib)

add_custom_command(
    TARGET NesEmulator POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/test-roms
        ${CMAKE_BINARY_DIR}/test-roms
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/res
        ${CMAKE_BINARY_DIR}/res
    COMMENT "Copying resource files to build directory"
)

add_custom_command(
    TARGET mapper_runner POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/test-roms
        ${CMAKE_BINARY_DIR}/test-roms
    COMMENT "Copying test ROMs for mapper runner"
)

enable_testing()
add_test(NAME test_mappers COMMAND test_mappers)
