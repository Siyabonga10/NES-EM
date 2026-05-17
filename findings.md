# Battle Chess & Dragon Warrior 3: Root Cause Analysis

## Symptom Summary
- **Battle Chess**: solid black screen, no audio
- **Dragon Warrior 3**: solid green screen, no audio
- All other tested games (including other MMC1 games) work correctly
- Both games boot, pass VBlank waits, initialize PPU registers, and reach the main game loop
- Frame timing is correct (~29,780 CPU cycles per frame)
- VBlank fires reliably every frame
- NMI fires reliably every frame, vector points to correct handler

## Screen Colors Explained

Battle Chess fills the palette with `$0F` which is palette index 15 = RGB (0x00, 0x00, 0x00) = solid black. Dragon Warrior 3 fills the palette with `$08` which is palette index 8 = RGB (0x00, 0x52, 0x28) = dark green. These palette writes happen during the init phase and each frame from the NMI handler, and they succeed. The colors are visible because the PPU is configured correctly (`$2001=$18` enables background + sprites) and the nametable is cleared to all zeros, which means all tiles reference tile index 0. Since CHR RAM is never loaded with actual tile data, all tiles are transparent, and only the backdrop color (palette entry 0) is visible.

## Audio Silence Explained

Battle Chess never writes to any APU register except `$4016` (controller strobe). Dragon Warrior 3 writes `$4015=$0F` (enables pulse 1+2, triangle), `$4001=$08`, `$4005=$08` (sweep registers), `$4011=$7E` (DMC DAC), but never writes frequency/duty values to `$4000`/`$4002-$4003` etc. No tones are produced. Both games eventually reach code paths that would initialize audio — but those code paths are gated behind the same deadlock that prevents CHR data loading.

## MMC1 Configuration (Confirmed Correct)
- CTRL = `$0E` (PRG mode 3 = fix last bank at `$C000-$FFFF`, switch `$8000-$BFFF`; CHR 4KB mode; mirroring = vertical)
- PRGB = `$00` (switchable bank maps to first 16KB of PRG ROM)
- CHR0 = `$00`, CHR1 = `$00` (both CHR banks map to first 4KB of CHR RAM)
- FCEUX writes identical MMC1 values — mapper configuration is correct

## PPU Configuration (Confirmed Correct)
- `$2000` = `$B0` (NMI on, pattern table 1 at `$1000`, VRAM increment 32)
- `$2001` = `$18` (show background + sprites)
- `$2005` = `$00,$00` (scroll at 0,0)
- VBlank fires at scanline 241, dot 1 every ~29,780 CPU cycles
- NMI vector at `$FFFA/$FFFB` = `$C4EF` (correctly read from ROM)

## NMI Handler Behavior (Confirmed Correct)
- Handler at `$C4EF` starts with `PHA`
- Checks `$03E7` (=0) → falls through
- Checks `$0A` (=0 after first NMI clears it) → falls through
- Checks `$09 | $07` (=0 initially, then `$09` gets incremented by `INC $09`)
- On second and subsequent calls, `$09 != 0` → `BNE $C4E9` → `LDA #$00; STA $0A; PLA; RTI`
- Stack pointer consistently `$F9` at NMI entry — perfectly balanced, no corruption
- NMI handler always exits via `$C565: PLA; $C566: RTI`
- When flags are all zero, handler falls through to `$C4FF: INC $09`, then runs full handler body: saves X/Y, writes `$2001=$18`, reads `$26`, does `AND #$FC; ORA $24; ORA $25; STA $26`, writes palette via JSR to `$C6D0` which sets PPUADDR=`$3F00` and writes 32 bytes from `$03C4-$03E3`, then exits via `$C565/$C566`

## The Code Paths (Two Separate Flows)

### Path 1: Palette Loader (`$C9FD`) — Works
Called from `$C0B8: JSR $C9FD` during init. Loads two 16-byte tables from fixed-bank ROM into CPU RAM `$03C4-$03E3`, then writes them to PPU palette at `$3F00`. This is what fills the palette with `$0F` (BC) or `$08` (DW3). This path executes correctly.

### Path 2: CHR Data Loader (`$C931` → `$C9C7` → `$C97C`) — Never Reached
Called from `$EB32: JSR $C931` in the per-frame loop. Loads a data-block descriptor from a table at `$CB41`, initializes a zero-page pointer `$32/$33` to point to compressed tile data in the switchable PRG bank (address `$B7CE+`), and enters the RLE decompressor at `$C97C`. The decompressor reads tile bytes via `LDA ($32),Y`, processes them, and writes to `$2007` with PPUADDR set to CHR RAM addresses (`$0000-$1FFF`).

Zero `$2007` writes to CHR addresses ever occur in 15+ seconds of full-speed execution. The CHR data loader is never entered.

## The Per-Frame Loop and the Deadlock

The per-frame loop at `$EB00`:

```
$EB25: JSR $C149     ; (1)
$EB28: JSR $C04C     ; (2)
$EB2B: JSR $C0D3     ; (3) — sets $2001, $26=$00, $2000
$EB2E: LDA #$01
$EB30: LDY #$01
$EB32: JSR $C931     ; (4) — CHR data loader
(loop back)
```

Steps (3) and (4) are gated behind step (2) returning. Step (2) calls `$C04C` which calls `JSR $C13E` — a two-stage VBlank wait loop (`$C13E: LDA $2002; BPL $C13E` then `$C143: LDA $2002; BPL $C143`). After both VBlank waits complete, `$C148: RTS` returns to `$C04F`.

Step (1) calls `$C149`:

```
$C149: LDA $26
$C14B: BPL $C13E     ; if $26 bit 7 clear → go to VBlank wait
$C14D: LDA $03E9     ; if $26 bit 7 set → check $03E9 flag
$C150: CMP $03E9
$C153: BEQ $C150     ; spin until $03E9 changes (NMI modifies it)
$C155: RTS           ; loop broken → return
```

When `$26` has bit 7 set (`$B0`): BPL NOT taken → falls through to `$C14D` → spin loop → NMI changes `$03E9` → loop breaks → `$C155: RTS` → returns to `$EB28` → step (2) proceeds.

When `$26` has bit 7 clear (`$00`): BPL taken → branches to `$C13E` VBlank wait → `$C148: RTS` returns to `$EB28` → step (2) proceeds either way.

After step (1) returns to `$EB28`, step (2) `JSR $C04C` calls the VBlank wait again. After the VBlank wait, `$C04F` runs the FULL re-init chain:

```
$C04F: LDA #$00
$C051: STA $2000      ; NMI DISABLED
$C054: STA $2001      ; rendering off
$C057: STA $26        ; $26 = $00
$C059: STA $27        ; $27 = $00
$C05B: LDA #$00
$C05D: JSR $C156      ; INC $07; STA $04; JSR $C160 (PRGB=0); DEC $07
$C060: JSR $C074      ; clear RAM vars ($22,$23,$07,$08,...)
$C063: JSR $C099      ; nametable clear (4096 zero writes via $C0A9 loop)
(inside $C099: JSR $C143 for VBlank wait)
    $C0B5: LDA #$00; TAX
    $C0B8: JSR $C9FD  ; PALETTE LOADER — writes palette to PPU
    $C0BB-$C0D2: MMC1 reset loop, OAM DMA, PPUADDR=$0000, RTS
$C066: JSR $CD8F      ; clear vars, JSR $C149
    (inside $C149: LDA $26 = $00 → BPL to VBlank wait → returns)
$C069: LDA #$01
$C06B: STA ...
$C06D: JSR $C0E0     ; PPU init: STA $2001=$18, STA $2000=$B0 (NMI ENABLED)
$C070: JSR $C149     ; LDA $26 = $00 → BPL to VBlank wait
$C073: RTS            ; return to $EB2B
```

## The Critical Variable: `$26`

`$26` is written at three points:
1. `$C057: STA $26` in the re-init chain — sets `$26=$00` (NMI path, BPL taken)
2. `$C517: STA $26` in the NMI handler — reads `$26`, does `AND #$FC; ORA $24; ORA $25`, stores result
3. `$C0D8: STA $26` in `$C0D3` subroutine — sets `$26=$00` (per-frame init)

The NMI handler's formula for `$26` is:
```
LDA $26      ; load current $26
AND #$FC     ; clear bits 0-1 (nametable select)
ORA $24      ; OR in scroll fine-X bits
ORA $25      ; OR in scroll coarse-X bits
STA $26      ; store result
```

If `$26` starts at `$00` before this code runs, the result depends entirely on `$24` and `$25`. If both are `$00`, `$26` stays `$00` forever. The BPL at `$C14B` will always branch to the VBlank wait, and the `$C14D-$C155` spin-wait path (which is required to reach `$EB2B`) is NEVER taken.

## The Power-On Random RAM Problem

On real NES hardware, CPU RAM (`$0000-$07FF`) powers up with random values. These random values are non-deterministic — some bits will be 0, some will be 1. The variables `$24` and `$25` are in zero-page RAM and are NOT explicitly initialized by the game code before the first NMI runs.

If `$24` or `$25` happens to have bit 7 set at power-on (a ~75% chance, since each bit has a 50% chance of being 1), the NMI handler's `ORA $24; ORA $25; STA $26` produces `$26` with bit 7 set. This causes `$C149` to take the `$C14D` spin-wait path, the per-frame loop breaks through, `$EB2B` is reached, and CHR data is loaded. The game works.

If BOTH `$24` AND `$25` happen to be `$00` at power-on (~25% chance), the NMI handler produces `$26=$00`. The game enters the permanent deadlock: BPL always branches to VBlank wait, `$C14D` is never reached, `$EB2B` is never reached, CHR data is never loaded, and the screen shows only the backdrop color.

In this emulator, `cpu_mem` (which includes zero-page RAM) is initialized with `memset(cpu_mem, 0, WRAM_SIZE + NO_OF_REGISTERS)` in `boot_cpu()`. This sets ALL zero-page variables to `$00`, including `$24` and `$25`. This guarantees the deadlock on every boot.

## Why Other MMC1 Games Work

Other MMC1 games that work in this emulator either:
1. Initialize `$24`/`$25` (or equivalent variables) to non-zero values before the NMI handler reads them
2. Use a different mechanism to break the per-frame spin loop (e.g., polling a register, using sprite-0 hit)
3. Do not depend on uninitialized RAM for their boot sequence

Battle Chess and Dragon Warrior 3 specifically rely on the random power-on state of `$24`/`$25` to eventually produce `$26` with bit 7 set. This is a known pattern in NES games — many commercial titles depend on power-on RAM values for random number generation or initial state. The games expect that after enough NMI iterations, the `ORA $24; ORA $25` chain will eventually produce a non-zero value due to other code paths modifying those variables, OR they rely on the initial power-on randomness.

## The Trace Evidence

TRACE points monitored (with fflush for guaranteed capture):
- `$C04F` — entry to re-init chain after `$C04C` returns: hit ONCE at cycle 116,755 (init path)
- `$C073` — RTS instruction that should jump to `$EB2B`: hit ONCE at cycle 478,222
- `$EB2B` — entry to `JSR $C0D3` (per-frame PPU init): NEVER hit
- `$C565/$C566` — NMI handler exit (PLA; RTI): hits EVERY frame, SP always `$F9`

The re-init chain at `$C04F-$C073` runs only during the first per-frame cycle. After that, the steady-state per-frame loop calls `$EB28: JSR $C04C` which enters `$C13E` VBlank wait — and `$C13E` never returns. Not because VBlank doesn't fire (it does), not because `$2002` doesn't report it (it does), not because the stack is corrupted (proven balanced). `$C13E` never returns because it IS returning — but the code path is different: when `$26=$00`, `$C149` takes BPL to `$C13E`, which returns via `$C148 RTS` back to `$EB28`, which calls `$C04C` again, which calls `$C13E` again, ad infinitum. The `$EB2B` instruction is never reached because the flow loops between `$C149 → BPL → $C13E → $C148 RTS → $EB28 → JSR $C04C → JSR $C13E` forever. 

For `$EB2B` to be reached, `$C149` must take the NON-BPL path (bit 7 of `$26` set), which requires `$26` to have been set to a value with bit 7 set via the NMI handler's `ORA $24; ORA $25` — which requires `$24` or `$25` to be non-zero. With zero-initialized RAM, they never are.

## The Fix

Initialize CPU RAM (specifically zero-page at minimum) with random values or a known pattern that includes non-zero bytes. Real NES power-on RAM is not all zeros. Setting variables like `$24` and `$25` to non-zero values (e.g., `$FF`) would allow the NMI handler to produce `$26` with bit 7 set, breaking the deadlock.

Alternatively, a "randomize RAM on boot" option would match real hardware behavior and fix all games that depend on power-on RAM state, not just these two.
