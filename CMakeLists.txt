cmake_minimum_required(VERSION 3.10)
project(NesEmulator C)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Include FetchContent for downloading raylib
include(FetchContent)

# Declare raylib dependency
FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 5.5
)

# Configure raylib build options
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE) # Don't build the examples
set(BUILD_GAMES    OFF CACHE BOOL "" FORCE) # Don't build the games

# Make raylib available
FetchContent_MakeAvailable(raylib)

# Find all source files in the src directory
file(GLOB_RECURSE SOURCES "src/*.c")

# Create executable from all source files
add_executable(NesEmulator ${SOURCES})

# Add include directories if needed
target_include_directories(NesEmulator PRIVATE src)

# Link against raylib
target_link_libraries(NesEmulator PRIVATE raylib)

# Copy test-roms directory to build directory after build
add_custom_command(
    TARGET NesEmulator POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/test-roms
        ${CMAKE_BINARY_DIR}/test-roms
    COMMENT "Copying test-roms directory to build directory"
)
