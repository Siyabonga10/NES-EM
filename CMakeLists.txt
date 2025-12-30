cmake_minimum_required(VERSION 3.10)
project(NesEmulator C)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Include FetchContent for downloading raylib
include(FetchContent)

FetchContent_Declare(
    unity
    GIT_REPOSITORY https://github.com/ThrowTheSwitch/Unity.git
    GIT_TAG v2.5.2
)
FetchContent_MakeAvailable(unity)
# Declare raylib dependency

FetchContent_Declare(
  raylib
  SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps/raylib
)
# Configure raylib build options
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE) # Don't build the examples
set(BUILD_GAMES    OFF CACHE BOOL "" FORCE) # Don't build the games

# Make raylib available
FetchContent_MakeAvailable(raylib)

# Find all source files in the src directory
file(GLOB_RECURSE SOURCES "src/*.c")
add_executable(NesEmulator ${SOURCES})
target_include_directories(NesEmulator PRIVATE src)
# Link against raylib
target_link_libraries(NesEmulator PRIVATE raylib)

file(GLOB_RECURSE TEST_SOURCES "test/*.c")
list(FILTER SOURCES EXCLUDE REGEX ".*main\\.c$")
add_executable(test_executable ${TEST_SOURCES} ${SOURCES})
target_include_directories(test_executable PRIVATE src)
target_link_libraries(test_executable unity raylib)

# Copy test-roms directory to build directory after build
add_custom_command(
    TARGET NesEmulator POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/test-roms
        ${CMAKE_BINARY_DIR}/test-roms
    COMMENT "Copying test-roms directory to build directory"
)

enable_testing()
add_test(NAME test_executable COMMAND test_executable)