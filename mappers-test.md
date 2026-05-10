# Mapper Testing Plan

## Architecture

Three CMake targets:

```
NesEmulator    — main emulator (src/main.c + core sources + raylib)
test_mappers   — unit tests (test/mappers/*.c + core sources, no main, no raylib)
mapper_runner  — integration tests (test/mapper_runner.c + core sources + raylib)
```

## Target 1: Unit Tests (`test_mappers`)

Compiled with `-DMAPPER_TEST` so mount functions become non-static and callable from test code.

Each test:
1. Allocates a `Cartriadge` + `iNesOneRomInfo`
2. Writes known test patterns into fake ROM buffers
3. Calls the mapper's mount function to wire up function pointers
4. Exercises bank switching via the write function and verifies mapped addresses
5. Asserts expected ROM contents at mapped addresses

No CPU, PPU, audio, or raylib. Purely tests that banking/mirroring logic is correct and edge cases are handled (wrapping, boundaries, invalid bank numbers, mode switching where applicable).

Test infrastructure:
- `test/mappers/test_helpers.h` — `create_test_cartridge()` helper: allocates Cartriadge, sets up fake ROM data with known bank patterns, returns ready-to-use struct
- `test/mappers/test_mapper_*.c` — per-mapper test file (or all in one file initially)

Cannot test: IRQ timing, scanline behavior, CPU/PPU interaction.

## Target 2: Integration Runner (`mapper_runner`)

Full emulator linked with raylib. Designed as a slideshow runner:

1. Loads mapper test ROMs from `test-roms/mappers/` sequentially
2. Boots the emulator with each ROM
3. ROM runs for N frames (the Holy Diver Batman ROM self-tests and displays results)
4. User reads result on screen
5. Space → next ROM, ESC → quit
6. If ROM returns `-2` (unsupported mapper), displays "UNSUPPORTED" overlay

Uses the mapper test ROMs from Holy Diver Batman:
- M0, M1 (multiple variants), M2, M3, M4, M7, M9, M10, M28, M34, M66, M69, M78.3, M118, M180

Also includes blargg's MMC3-specific tests (`mmc3_test/`):
- IRQ clocking, A12 clocking, scanline timing

## Adding a New Mapper — Workflow

1. Create `src/core/mappers/mXXX.c` with:
   - Mapper functions (static)
   - Mount function (static, cleared by `-DMAPPER_TEST`)
   - `REGISTER_MAPPER(mount_fn, XXX)` at bottom

2. Create unit test in `test/mappers/`:
   - Verify PRG banking in all modes
   - Verify CHR banking in all modes
   - Verify mirroring control
   - Test edge cases (bank wrap, invalid writes, etc.)

3. Run unit test: `./build/test_mappers`

4. Add ROM to integration runner's playlist

5. Run integration runner: `./build/mapper_runner` — visually verify test ROM output

6. Run 2-3 actual commercial games using that mapper

## Mapper Implementation Queue (Simplest → Hardest)

| Priority | Mapper | Effort | Depends On |
|---|---|---|---|
| 1 | 7 (AxROM) | 1h | Nothing — single PRG register |
| 2 | 34 (BNROM) | 1h | Nothing — same as 7 |
| 3 | 180 (UNROM 7408) | 30min | Trivial variant of mapper 2 |
| 4 | 9 (MMC2) | 2h | CHR latch logic |
| 5 | 10 (MMC4) | 30min | Based on MMC2, minor latch difference |
| 6 | 28 (INL-ROM) | 3h | Many banking modes |
| 7 | 78.3 (IF-12) | 3h | IRQ similar to MMC3 |
| 8 | 118 (TxSROM) | 1h | MMC3 variant |
| 9 | 5 (MMC5) | Days | Vertical split, multiplier — hardest mapper |

## Coverage Estimate

| Stage | Mappers | Coverage |
|---|---|---|
| Already done | 0, 1, 2, 3, 4, 66, (69 broken) | ~83% |
| + Queue 1-3 | 7, 34, 180 | ~88% |
| + Queue 4-6 | 9, 10, 28 | ~91% |
| + Queue 7-8 + fix 66,69 | 78.3, 118 | ~93% |
| + MMC5 | 5 | ~99% |
