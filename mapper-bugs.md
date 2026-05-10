# Known Mapper Bugs

## Mapper 2 (UxROM) — FIXED

**Out-of-bounds bank switching** — Fixed by adding `% cart->pg_rom_size` to
the switchable bank address calculation. Writing bank 15 on a 4-bank ROM now
wraps to bank 3, matching hardware behavior (unused high address pins disconnected).

## Mapper 66 (GxROM) — FIXED

**CHR out-of-bounds banking** — Fixed by adding `% chr_size` to the CHR address
calculation. Writing CHR bank 3 on a 1-bank ROM now wraps to bank 0.

## Mapper 66 (GxROM) — OPEN

**Untested against real games** — per source comment. Basic banking works in unit
tests but not verified with commercial ROMs.

## Mapper 69 (FME-7 / Sunsoft 5B) — OPEN

**Not working** — per source comment. Suspect register initialization and
IRQ counter timing.
