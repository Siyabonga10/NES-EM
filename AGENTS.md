# Agent Instructions for NesEmulator

## Build (CMake)
- Generate and build: `cmake -B build -S . && cmake --build build`
- Executable: `build/NesEmulator` (Windows: `build/NesEmulator.exe`)
- Run emulator: `./build/NesEmulator` (Windows: `build\NesEmulator.exe`)
- Test executable: `build/test_executable`
- Run tests: `cd build && ctest` (or run `./test_executable`)
- Post-build copies `test-roms/` and `res/` to build directory

## Build (Make)
- Build raylib for desktop: `make raylib`
- Build core DLL for Windows: `make core-windows`
- Build main executable: `make c-windows` (requires `build/core.dll`)
- Build raylib for WebAssembly: `make raylib-wasm`
- Build core for WebAssembly: `make core-wasm`
- Output: `web/output.js` and `web/output.wasm`
- The Makefile uses `mingw32-make` for Windows desktop builds

## Testing
- Primary test suite: 15 test ROMs in `test-roms/` (01-implied.nes through 15-special.nes)
- The main executable currently loads `test-roms/tetris2.nes`
- Unity test framework is configured but no tests are implemented yet (`test/test.c` is empty)
- To run the test ROMs, you may need to modify `main.c` to iterate over `test_files` array

## Project Structure
- `src/core/`: Core emulator components (CPU, PPU, APU, bus, mapper, etc.)
- `src/main.c`: Entry point with graphics loop using raylib
- `deps/raylib/`: Raylib graphics library (already fetched by CMake)
- `test-roms/`: Test ROMs for CPU instruction tests
- `res/`: Resource files (if any)
- `web/`: Web build output and frontend files

## Conventions
- Spelling: "cartriadge" (not "cartridge") used throughout codebase
- Platform: Desktop (Windows/Linux/macOS) and Web (Emscripten) targets
- Dependencies: Raylib (included as subdirectory); no external package manager

## Web Build Requirements
- Emscripten SDK must be installed and active (`emcc`, `emmake` in PATH)
- Run `make raylib-wasm` before `make core-wasm`
- The web build exports specific functions (e.g., `bootCPU`, `tickCPU`) for JavaScript integration
- Serve `web/` directory with a local HTTP server (e.g., `python -m http.server`)


