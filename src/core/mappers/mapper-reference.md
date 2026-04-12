# NES Mapper Reference

---

## m000 ??? NROM

The simplest possible mapper ??? essentially no mapper at all. The cartridge has fixed PRG-ROM (program data) wired directly to the CPU address space, and fixed CHR-ROM (graphics data) wired to the PPU. There is no bank switching of any kind. Cartridges are limited to 32KB of PRG-ROM and 8KB of CHR-ROM. The CPU sees the ROM at $8000???$FFFF and that's it ??? what's there at boot is all that's ever there.

**Notable games:** Donkey Kong, Super Mario Bros. 1, Excitebike

---

## m001 ??? MMC1 (Nintendo MMC1)

One of the most common mappers. Uses a serial shift register ??? the CPU writes a single bit at a time (5 writes total) to a register at $8000???$FFFF to configure the mapper. This was done to save pins on the cartridge connector. Once all 5 bits are shifted in, the value is latched into one of four internal registers controlling:

- **Control** ??? mirroring mode and PRG/CHR bank size
- **CHR bank 0** ??? selects 4KB or 8KB CHR bank for $0000
- **CHR bank 1** ??? selects 4KB CHR bank for $1000 (ignored in 8KB mode)
- **PRG bank** ??? selects 16KB or 32KB PRG bank, with one bank optionally fixed to $8000 or $C000

Supports up to 512KB PRG-ROM and 128KB CHR-ROM. Nametable mirroring is software-controlled (horizontal, vertical, or single-screen).

**Notable games:** The Legend of Zelda, Metroid, Mega Man 2, Final Fantasy

---

## m002 ??? UxROM (Konami / Nintendo)

A simple bank-switching mapper. PRG-ROM is split into two 16KB banks:

- **$8000???$BFFF** ??? switchable, selected by writing to any address in $8000???$FFFF
- **$C000???$FFFF** ??? fixed to the **last** 16KB bank in the ROM

CHR is provided as 8KB of CHR-RAM (not ROM), so graphics are writable at runtime. No mirroring control ??? hardwired to the cartridge PCB. Supports up to 256KB PRG-ROM.

**Notable games:** Mega Man, Castlevania, DuckTales, Contra

---

## m003 ??? CNROM (Coleco / Nintendo)

PRG-ROM is fixed (same as NROM ??? 16KB mirrored or 32KB), but CHR-ROM is bank-switched. Writing to any address in $8000???$FFFF selects one of up to four 8KB CHR banks. That's the entire feature set. Very simple, very cheap to manufacture.

**Notable games:** Gradius, Arkanoid, Friday the 13th

---

## m004 ??? MMC3 (Nintendo MMC3)

The most sophisticated of Nintendo's common mappers. Provides fine-grained bank switching for both PRG and CHR, plus a scanline counter for mid-screen effects (critical for split-screen HUDs).

**PRG banking:** Two switchable 8KB banks at $8000 and $A000, with $C000 and $E000 fixed to the last two banks (or vice versa via a mode bit).

**CHR banking:** Can switch CHR in 2KB and 1KB chunks independently, enabling complex tile arrangements.

**IRQ counter:** Clocked by PPU A12 transitions (i.e., when the PPU fetches tiles). The CPU receives an IRQ when the counter hits zero, allowing the game to change scroll registers or palettes mid-frame. This is how games like SMB3 achieve their status bar separation.

Supports up to 512KB PRG-ROM and 256KB CHR-ROM. Software-controlled mirroring (H/V).

**Notable games:** Super Mario Bros. 2 & 3, Mega Man 3???6, Kirby's Adventure, Tetris

---

## m005 ??? MMC5 (Nintendo MMC5)

The most powerful licensed NES mapper. Rarely used due to cost. Key features:

- **ExRAM** ??? 1KB of extra nametable RAM on the mapper chip itself
- **Extended nametable modes** ??? can use ExRAM as a nametable or attribute extension, allowing per-tile attribute data (normally attributes cover 2??2 tile blocks; MMC5 can do per-tile color)
- **PRG banking** ??? highly flexible; supports 8KB, 16KB, or 32KB modes with multiple fixed/switchable combinations; also supports up to 1MB PRG-ROM
- **CHR banking** ??? can switch in 1KB, 2KB, 4KB, or 8KB chunks with two independent sets of registers
- **Scanline IRQ** ??? like MMC3 but based on scanline number, not A12
- **Built-in multiplier** ??? an 8??8 multiply unit in hardware for faster math
- **PCM audio channel** ??? an extra audio channel on the mapper chip

**Notable games:** Castlevania III (US), Just Breed, Laser Invasion

---

## m007 ??? AxROM (Rare / Nintendo)

Very simple. Switches the entire 32KB PRG bank by writing to $8000???$FFFF. Uses single-screen nametable mirroring, switchable between the two physical nametable pages (upper bit of the bank register). CHR is always RAM (8KB). No CHR bank switching. Max 512KB PRG-ROM.

**Notable games:** Battletoads, Marble Madness, Cobra Triangle, Wizards & Warriors

---

## m009 ??? MMC2 (Nintendo MMC2)

Designed specifically for Punch-Out!!. Provides automatic CHR bank switching driven by the PPU's latch ??? the mapper monitors which tile the PPU fetches and switches CHR banks on the fly when specific tile IDs ($FD or $FE) are fetched. This is called a **latch-based** bank switch.

Two CHR banks are maintained (one for $0000???$0FFF and one for $1000???$1FFF), each with two possible 4KB pages. The latch determines which of the two pages is active. PRG is switched in 8KB chunks at $8000, with $A000???$FFFF fixed to the last three 8KB banks.

**Notable games:** Mike Tyson's Punch-Out!!

---

## m010 ??? MMC4 (Nintendo MMC4)

Essentially MMC2 with a larger PRG bank switch. PRG is now switched in 16KB chunks at $8000, with $C000???$FFFF fixed to the last 16KB bank. CHR latch behavior is identical to MMC2. Supports up to 256KB PRG-ROM.

**Notable games:** Fire Emblem, Fire Emblem Gaiden (Famicom exclusives)

---

## m011 ??? Color Dreams

An unlicensed mapper used by Color Dreams (and their religious-themed sub-label Wisdom Tree). A single register write sets both PRG (32KB) and CHR (8KB) banks simultaneously ??? the low 4 bits select the CHR bank, and the high 4 bits select the PRG bank. Extremely simple and cheap. Games using this mapper could not be manufactured by Nintendo due to the lockout chip bypass required.

**Notable games:** Crystal Mines, Bible Adventures (Wisdom Tree)

---

## m016 ??? Bandai FCG (Bandai)

Used by Bandai on Famicom cartridges. Features standard PRG and CHR bank switching but also includes an **EEPROM** or **battery-backed RAM** for save data, and notably a **barcode reader interface** on some variants ??? the Datach series could read barcodes from cards to load character data into games. IRQ counter support is also included.

**Notable games:** Dragon Ball Z, Rokudenashi Blues, Datach series (Famicom)

---

## m021 ??? VRC2/VRC4a (Konami VRC2/4)

Part of Konami's VRC (Virtual ROM Controller) series. VRC2 is the simpler variant, providing CHR in 1KB switchable banks and PRG in 8KB switchable banks with the last 8KB fixed. VRC4 extends this with a scanline IRQ counter. The "a/b/c" suffix variants differ only in which address lines carry the bank select bits ??? a hardware quirk that resulted in multiple iNES mapper numbers for what is functionally identical silicon.

**Notable games:** Gradius II (Famicom)

---

## m023 ??? VRC2/VRC4b (Konami VRC2/4)

Functionally identical to m021 but with different address line assignments for the register selects. Same capability: 1KB CHR banking, 8KB PRG banking, optional scanline IRQ on VRC4 variants.

**Notable games:** Contra (Famicom), Teenage Mutant Ninja Turtles (Famicom)

---

## m024 ??? VRC6a (Konami VRC6)

Konami's VRC6 chip adds two significant capabilities on top of standard bank switching: **three extra audio channels** (two pulse waves and one sawtooth wave) mapped into the CPU address space. These channels produce noticeably richer sound than the stock NES APU. Also provides 1KB CHR bank switching and 8KB PRG bank switching with scanline IRQ. The "a" vs "b" distinction again comes down to address line swapping.

**Notable games:** Akumajou Densetsu (Famicom Castlevania III), Madara, Esper Dream 2

---

## m026 ??? VRC6b (Konami VRC6)

Identical to m024 in function. The address lines A0 and A1 are swapped relative to m024, requiring a separate mapper ID. Same audio expansion, same banking capabilities.

**Notable games:** Castlevania III: Dracula's Curse (Famicom version), Lagrange Point

---

## m034 ??? BNROM / NINA-001

Switches the entire 32KB PRG bank, similar to AxROM, by writing to $8000???$FFFF (BNROM) or to $7FFD???$7FFF (NINA-001). CHR is RAM. No mirroring control ??? hardwired. Very simple design used by a small number of publishers.

**Notable games:** Deadly Towers (BNROM), Impossible Mission II (NINA-001 variant)

---

## m064 ??? RAMBO-1 (Tengen)

Tengen's (Atari Games') clone of the MMC3, with some differences. Provides similar PRG and CHR bank switching and a scanline IRQ, but the IRQ counter behavior differs slightly ??? it can be clocked either by PPU A12 (like MMC3) or by CPU cycles, selectable via a register bit. Also supports an extra switchable 8KB PRG bank at $6000???$7FFF. Tengen manufactured their own cartridges without Nintendo's license, bypassing the lockout chip.

**Notable games:** Rad Racer II, Mendel Palace, Tengen Tetris

---

## m066 ??? GxROM (Nintendo)

A simple two-dimensional bank switch. A single register write selects both a 32KB PRG bank (bits 5???4) and an 8KB CHR bank (bits 1???0) simultaneously. No IRQ, no mirroring control ??? hardwired. Used for multi-game cartridges and simple titles.

**Notable games:** Super Mario Bros. + Duck Hunt, Gumshoe, Doraemon (Famicom)

---

## m069 ??? Sunsoft FME-7 (Sunsoft)

A capable mapper with several notable features. Provides 8KB PRG bank switching across four windows ($8000, $A000, $C000, $E000), 1KB CHR bank switching, software-controlled mirroring, and a scanline IRQ. Also supports battery-backed RAM at $6000???$7FFF.

Most notably, FME-7 cartridges (specifically the YM2149-based variant, sometimes called 5B) include a **Yamaha YM2149 sound chip** which adds three square wave channels and one noise channel ??? significantly enhancing the audio capabilities.

**Notable games:** Batman: Return of the Joker, Gimmick!, Hebereke (Famicom)

---

---

## Implementation Status

| Mapper | Name | Implemented | Notes |
|--------|------|-------------|-------|
| m000 | NROM | ? | Fully implemented |
| m001 | MMC1 | ? | Fully implemented |
| m002 | UxROM | ? | Fully implemented |
| m003 | CNROM | ? | Fully implemented |
| m004 | MMC3 | ? | Fully implemented |
| m005 | MMC5 | ? | Fully implemented |
| m006 | *Unknown* | ? | Implemented but not in reference list |
| m007 | AxROM | ? | Fully implemented |
| m009 | MMC2 | ? | |
| m010 | MMC4 | ? | |
| m011 | Color Dreams | ✓ | |
| m016 | Bandai FCG | ? | |
| m019 | Namco 163 | ? | |
| m021 | VRC2/4a | ? | |
| m023 | VRC2/4b | ? | Implemented |
| m024 | VRC6a | ? | |
| m026 | VRC6b | ? | |
| m034 | BNROM / NINA-001 | ✓ | |
| m064 | RAMBO-1 | ? | |
| m066 | GxROM | ✓ | |
| m069 | Sunsoft FME-7 | ? | |

*Mapper numbering follows the iNES/NES 2.0 format standard used by most emulators.*
