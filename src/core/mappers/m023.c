#include "m023.h"
#include <stdbool.h>

static unsigned char prg_banks[4] = {0, 1, 2, 3}; // 8KB PRG banks for $8000, $A000, $C000, $E000
static unsigned char chr_banks[8] = {0, 1, 2, 3, 4, 5, 6, 7}; // 1KB CHR banks
static bool vrc4_mode = false; // VRC4 adds IRQ

void M023_Write(Cartriadge *cart, int addr, unsigned char value)
{
    // VRC2/4 register mapping based on address lines
    // Typical mapping: address bits determine register
    // For simplicity, we'll use a common VRC2 mapping
    // $8000-$8FFF: PRG bank 0 (8KB at $8000)
    // $9000-$9FFF: PRG bank 1 (8KB at $A000)
    // $A000-$AFFF: PRG bank 2 (8KB at $C000)
    // $B000-$BFFF: PRG bank 3 (8KB at $E000)
    // $C000-$CFFF: CHR bank 0 (1KB)
    // $D000-$DFFF: CHR bank 1
    // $E000-$EFFF: CHR bank 2
    // $F000-$FFFF: CHR bank 3
    
    int reg = (addr >> 12) & 0x07; // Get register from address bits 12-14
    
    if (reg < 4) {
        // PRG banks 0-3 (8KB each)
        prg_banks[reg] = value & 0x0F; // 16 possible 8KB banks
    } else if (reg < 8) {
        // CHR banks 0-3 (1KB each)
        chr_banks[reg - 4] = value & 0x1F; // 32 possible 1KB banks
    }
    // Note: Real VRC2 has more CHR banks (8 total) and different address mapping
    // This is a simplified implementation
}

int M023(Cartriadge *cart, int addr)
{
    if (addr < 0x8000)
        return addr - 0x6000;
    
    // Determine which 8KB window
    int window = (addr >> 13) & 0x03; // 0: $8000, 1: $A000, 2: $C000, 3: $E000
    int bank = prg_banks[window];
    
    // For $E000-$FFFF (window 3), always use last bank if fixed mode
    // VRC2 typically fixes last 8KB bank
    if (window == 3) {
        int last_bank = (cart->pg_rom_size / 0x2000) - 1;
        bank = last_bank;
    }
    
    int offset = addr & 0x1FFF; // 8KB offset
    return (bank * 0x2000) + offset + 0x2000;
}

int M023_PPU(Cartriadge *cart, int addr)
{
    // CHR banking: 1KB banks
    int bank = chr_banks[addr >> 10]; // 1KB = 1024 bytes = 0x400
    int offset = addr & 0x03FF;
    return cart->pg_rom_size + 0x2000 + (bank * 0x0400) + offset;
}
