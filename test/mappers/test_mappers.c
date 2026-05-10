#include "unity.h"
#include "core/cartriadge.h"
#include "core/rom_loader.h"
#include "core/bus.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Build an iNES ROM buffer. After calling, write test patterns directly
   into the PRG and CHR regions before passing to load_cartridge_from_memory. */
static unsigned char *build_rom(int mapper, int prg_banks, int chr_banks,
                                int *out_len) {
    int header = 16;
    int prg_size = prg_banks * 0x4000;
    int chr_size = chr_banks * 0x2000;
    int total = header + prg_size + chr_size;

    unsigned char *rom = calloc(1, total);
    rom[0] = 'N'; rom[1] = 'E'; rom[2] = 'S'; rom[3] = 0x1A;
    rom[4] = prg_banks;
    rom[5] = chr_banks;
    rom[6] = (mapper & 0x0F) << 4;
    rom[7] = (mapper & 0xF0);

    *out_len = total;
    return rom;
}

void setUp(void) {}
void tearDown(void) {}

/* ================================================================
   Mapper 0 (NROM) — 1×16KB or 2×16KB PRG, 1×8KB CHR
   ================================================================ */

void test_mapper_0_16k_prg(void) {
    int len;
    unsigned char *rom = build_rom(0, 1, 1, &len);
    /* Write unique values at both ends of the single 16KB bank */
    rom[16 + 0x0000] = 0x51;
    rom[16 + 0x3FFF] = 0xA3;

    Cartriadge cart;
    int rc = load_cartridge_from_memory(rom, len, &cart);
    TEST_ASSERT_EQUAL_INT(0, rc);

    TEST_ASSERT_EQUAL_UINT8(0x51, read_byte(0x8000));
    TEST_ASSERT_EQUAL_UINT8(0xA3, read_byte(0xBFFF));

    free(rom);
    free(cart.pg_rom); free(cart.ch_rom);
    free(cart.chr_ram); free(cart.prg_ram);
}

void test_mapper_0_mirrors_16k(void) {
    int len;
    unsigned char *rom = build_rom(0, 1, 1, &len);
    rom[16] = 0x51;

    Cartriadge cart;
    load_cartridge_from_memory(rom, len, &cart);

    /* 16KB ROM mirrors into both $8000 and $C000 windows */
    TEST_ASSERT_EQUAL_UINT8(0x51, read_byte(0x8000));
    TEST_ASSERT_EQUAL_UINT8(0x51, read_byte(0xC000));

    free(rom);
    free(cart.pg_rom); free(cart.ch_rom);
    free(cart.chr_ram); free(cart.prg_ram);
}

void test_mapper_0_32k_prg(void) {
    int len;
    unsigned char *rom = build_rom(0, 2, 1, &len);
    rom[16 + 0x0000] = 0xCC;
    rom[16 + 0x4000] = 0xDD;

    Cartriadge cart;
    load_cartridge_from_memory(rom, len, &cart);

    TEST_ASSERT_EQUAL_UINT8(0xCC, read_byte(0x8000));
    TEST_ASSERT_EQUAL_UINT8(0xDD, read_byte(0xC000));

    free(rom);
    free(cart.pg_rom); free(cart.ch_rom);
    free(cart.chr_ram); free(cart.prg_ram);
}

void test_mapper_0_chr_read(void) {
    int len;
    unsigned char *rom = build_rom(0, 1, 1, &len);
    int chr_off = 16 + 1 * 0x4000;
    rom[chr_off + 0x0000] = 0x7E;
    rom[chr_off + 0x1FFF] = 0xE7;

    Cartriadge cart;
    load_cartridge_from_memory(rom, len, &cart);

    TEST_ASSERT_EQUAL_UINT8(0x7E, read_byte_ppu(0x0000));
    TEST_ASSERT_EQUAL_UINT8(0xE7, read_byte_ppu(0x1FFF));

    free(rom);
    free(cart.pg_rom); free(cart.ch_rom);
    free(cart.chr_ram); free(cart.prg_ram);
}

/* ================================================================
   Mapper 2 (UxROM) — switchable 16KB at $8000, last bank fixed at $C000
   ================================================================ */

void test_mapper_2_last_bank_fixed(void) {
    int len;
    unsigned char *rom = build_rom(2, 4, 1, &len);
    for (int b = 0; b < 4; b++)
        rom[16 + b * 0x4000] = (unsigned char)(0xA0 + b);

    Cartriadge cart;
    int rc = load_cartridge_from_memory(rom, len, &cart);
    TEST_ASSERT_EQUAL_INT(0, rc);

    /* Last bank (3) always at $C000 */
    TEST_ASSERT_EQUAL_UINT8(0xA3, read_byte(0xC000));

    free(rom);
    free(cart.pg_rom); free(cart.ch_rom);
    free(cart.chr_ram); free(cart.prg_ram);
}

void test_mapper_2_switchable_bank(void) {
    int len;
    unsigned char *rom = build_rom(2, 4, 1, &len);
    for (int b = 0; b < 4; b++)
        rom[16 + b * 0x4000] = (unsigned char)(0xB0 + b);

    Cartriadge cart;
    load_cartridge_from_memory(rom, len, &cart);

    write_byte(0x8000, 0);
    TEST_ASSERT_EQUAL_UINT8(0xB0, read_byte(0x8000));

    write_byte(0x8000, 2);
    TEST_ASSERT_EQUAL_UINT8(0xB2, read_byte(0x8000));

    write_byte(0x8000, 3);
    TEST_ASSERT_EQUAL_UINT8(0xB3, read_byte(0x8000));

    /* Last bank unchanged */
    TEST_ASSERT_EQUAL_UINT8(0xB3, read_byte(0xC000));

    free(rom);
    free(cart.pg_rom); free(cart.ch_rom);
    free(cart.chr_ram); free(cart.prg_ram);
}

/* Edge case: out-of-bounds bank. Should wrap modulo bank_count (hardware),
   currently segfaults. Keep last so other tests still run. */
void test_mapper_2_bank_wrap(void) {
    int len;
    unsigned char *rom = build_rom(2, 4, 1, &len);
    for (int b = 0; b < 4; b++)
        rom[16 + b * 0x4000] = (unsigned char)(0xC0 + b);

    Cartriadge cart;
    int rc = load_cartridge_from_memory(rom, len, &cart);
    TEST_ASSERT_EQUAL_INT(0, rc);

    /* 15 & 0x0F = 15, should wrap to bank 3 on real hardware */
    write_byte(0x8000, 0x1F);
    TEST_ASSERT_EQUAL_UINT8(0xC3, read_byte(0x8000));

    free(rom);
    free(cart.pg_rom); free(cart.ch_rom);
    free(cart.chr_ram); free(cart.prg_ram);
}

/* ================================================================
   Mapper 3 (CNROM) — fixed 16/32KB PRG, switchable 8KB CHR
   ================================================================ */

void test_mapper_3_chr_banking(void) {
    int len;
    unsigned char *rom = build_rom(3, 1, 4, &len);
    int chr_off = 16 + 1 * 0x4000;
    for (int b = 0; b < 4; b++)
        rom[chr_off + b * 0x2000] = (unsigned char)(0x80 + b);

    Cartriadge cart;
    int rc = load_cartridge_from_memory(rom, len, &cart);
    TEST_ASSERT_EQUAL_INT(0, rc);

    /* Default: bank 0 */
    TEST_ASSERT_EQUAL_UINT8(0x80, read_byte_ppu(0x0000));

    write_byte(0x8000, 2);
    TEST_ASSERT_EQUAL_UINT8(0x82, read_byte_ppu(0x0000));

    write_byte(0x8000, 3);
    TEST_ASSERT_EQUAL_UINT8(0x83, read_byte_ppu(0x0000));

    write_byte(0x8000, 0);
    TEST_ASSERT_EQUAL_UINT8(0x80, read_byte_ppu(0x0000));

    free(rom);
    free(cart.pg_rom); free(cart.ch_rom);
    free(cart.chr_ram); free(cart.prg_ram);
}

/* ================================================================
   Mapper 66 (GxROM) — 32KB PRG + 8KB CHR, both switchable
   ================================================================ */

void test_mapper_66_prg_banking(void) {
    int len;
    unsigned char *rom = build_rom(66, 4, 1, &len);
    /* 4 iNES PRG banks = 64KB = 2 × 32KB mapper-66 banks */
    rom[16 + 0x0000] = 0x61;
    rom[16 + 0x8000] = 0x62;

    Cartriadge cart;
    int rc = load_cartridge_from_memory(rom, len, &cart);
    TEST_ASSERT_EQUAL_INT(0, rc);

    /* Bank select: upper nibble = PRG bank */
    write_byte(0x8000, 0 << 4);
    TEST_ASSERT_EQUAL_UINT8(0x61, read_byte(0x8000));

    write_byte(0x8000, 1 << 4);
    TEST_ASSERT_EQUAL_UINT8(0x62, read_byte(0x8000));

    free(rom);
    free(cart.pg_rom); free(cart.ch_rom);
    free(cart.chr_ram); free(cart.prg_ram);
}

/* ================================================================
   Edge case: last byte of every bank, distinct value per bank
   ================================================================ */

void test_mapper_0_bank_last_bytes(void) {
    int len;
    unsigned char *rom = build_rom(0, 2, 1, &len);
    /* bank 0, last byte at offset 0x3FFF */
    rom[16 + 0x3FFF] = 0xA1;
    /* bank 1, last byte at offset 0x7FFF */
    rom[16 + 0x7FFF] = 0xA2;

    Cartriadge cart;
    int rc = load_cartridge_from_memory(rom, len, &cart);
    TEST_ASSERT_EQUAL_INT(0, rc);

    TEST_ASSERT_EQUAL_UINT8(0xA1, read_byte(0xBFFF));
    TEST_ASSERT_EQUAL_UINT8(0xA2, read_byte(0xFFFF));

    free(rom);
    free(cart.pg_rom); free(cart.ch_rom);
    free(cart.chr_ram); free(cart.prg_ram);
}

void test_mapper_2_bank_last_bytes(void) {
    int len;
    unsigned char *rom = build_rom(2, 4, 1, &len);
    rom[16 + 0x3FFF]       = 0xF0;
    rom[16 + 0x7FFF]       = 0xF1;
    rom[16 + 0xBFFF]       = 0xF2;
    rom[16 + 0xFFFF]       = 0xF3;

    Cartriadge cart;
    int rc = load_cartridge_from_memory(rom, len, &cart);
    TEST_ASSERT_EQUAL_INT(0, rc);

    /* Last byte of fixed bank (bank 3) at $C000-$FFFF */
    TEST_ASSERT_EQUAL_UINT8(0xF3, read_byte(0xFFFF));

    /* Switch to bank 0, read last byte at $BFFF */
    write_byte(0x8000, 0);
    TEST_ASSERT_EQUAL_UINT8(0xF0, read_byte(0xBFFF));

    /* Switch to bank 1 */
    write_byte(0x8000, 1);
    TEST_ASSERT_EQUAL_UINT8(0xF1, read_byte(0xBFFF));

    /* Fixed bank unchanged */
    TEST_ASSERT_EQUAL_UINT8(0xF3, read_byte(0xFFFF));

    free(rom);
    free(cart.pg_rom); free(cart.ch_rom);
    free(cart.chr_ram); free(cart.prg_ram);
}

void test_mapper_3_chr_last_bytes(void) {
    int len;
    unsigned char *rom = build_rom(3, 1, 4, &len);
    int chr_off = 16 + 1 * 0x4000;
    rom[chr_off + 0x1FFF]       = 0x91;
    rom[chr_off + 0x3FFF]       = 0x92;
    rom[chr_off + 0x5FFF]       = 0x93;
    rom[chr_off + 0x7FFF]       = 0x94;

    Cartriadge cart;
    int rc = load_cartridge_from_memory(rom, len, &cart);
    TEST_ASSERT_EQUAL_INT(0, rc);

    write_byte(0x8000, 0);
    TEST_ASSERT_EQUAL_UINT8(0x91, read_byte_ppu(0x1FFF));

    write_byte(0x8000, 2);
    TEST_ASSERT_EQUAL_UINT8(0x93, read_byte_ppu(0x1FFF));

    write_byte(0x8000, 3);
    TEST_ASSERT_EQUAL_UINT8(0x94, read_byte_ppu(0x1FFF));

    write_byte(0x8000, 1);
    TEST_ASSERT_EQUAL_UINT8(0x92, read_byte_ppu(0x1FFF));

    free(rom);
    free(cart.pg_rom); free(cart.ch_rom);
    free(cart.chr_ram); free(cart.prg_ram);
}

void test_mapper_66_prg_last_bytes(void) {
    int len;
    unsigned char *rom = build_rom(66, 4, 1, &len);
    /* Two 32KB mapper banks — last byte of each */
    rom[16 + 0x7FFF]  = 0xDE;
    rom[16 + 0xFFFF]  = 0xDF;

    Cartriadge cart;
    int rc = load_cartridge_from_memory(rom, len, &cart);
    TEST_ASSERT_EQUAL_INT(0, rc);

    write_byte(0x8000, 0 << 4);
    TEST_ASSERT_EQUAL_UINT8(0xDE, read_byte(0xFFFF));

    write_byte(0x8000, 1 << 4);
    TEST_ASSERT_EQUAL_UINT8(0xDF, read_byte(0xFFFF));

    free(rom);
    free(cart.pg_rom); free(cart.ch_rom);
    free(cart.chr_ram); free(cart.prg_ram);
}

/* Mapper 66 CHR: bank value wraps modulo available CHR banks.
   Writing chr_bank=3 on a 1-bank ROM reads bank 0 (3 % 1 = 0). */
void test_mapper_66_chr_bank_oob(void) {
    int len;
    unsigned char *rom = build_rom(66, 4, 1, &len);
    int chr_off = 16 + 4 * 0x4000;
    rom[chr_off] = 0x7A;

    Cartriadge cart;
    int rc = load_cartridge_from_memory(rom, len, &cart);
    TEST_ASSERT_EQUAL_INT(0, rc);

    /* Low nibble = CHR bank, value 3 wraps to bank 0 on 1-bank ROM */
    write_byte(0x8000, 3);
    TEST_ASSERT_EQUAL_UINT8(0x7A, read_byte_ppu(0x0000));

    free(rom);
    free(cart.pg_rom); free(cart.ch_rom);
    free(cart.chr_ram); free(cart.prg_ram);
}

/* ================================================================
   Mapper 7 (AxROM) — 32KB PRG switchable, single-screen mirroring
   ================================================================ */

void test_mapper_7_prg_banking(void) {
    int len;
    unsigned char *rom = build_rom(7, 8, 1, &len);
    /* 8 iNES PRG banks = 4 × 32KB mapper-7 banks */
    rom[16 + 0x0000]  = 0x71;
    rom[16 + 0x8000]  = 0x72;
    rom[16 + 0x10000] = 0x73;
    rom[16 + 0x18000] = 0x74;

    Cartriadge cart;
    int rc = load_cartridge_from_memory(rom, len, &cart);
    TEST_ASSERT_EQUAL_INT(0, rc);

    write_byte(0x8000, 0);
    TEST_ASSERT_EQUAL_UINT8(0x71, read_byte(0x8000));

    write_byte(0x8000, 1);
    TEST_ASSERT_EQUAL_UINT8(0x72, read_byte(0x8000));

    write_byte(0x8000, 2);
    TEST_ASSERT_EQUAL_UINT8(0x73, read_byte(0x8000));

    write_byte(0x8000, 3);
    TEST_ASSERT_EQUAL_UINT8(0x74, read_byte(0x8000));

    free(rom);
    free(cart.pg_rom); free(cart.ch_rom);
    free(cart.chr_ram); free(cart.prg_ram);
}

void test_mapper_7_last_bytes(void) {
    int len;
    unsigned char *rom = build_rom(7, 8, 1, &len);
    rom[16 + 0x7FFF]  = 0xD1;
    rom[16 + 0xFFFF]  = 0xD2;
    rom[16 + 0x17FFF] = 0xD3;
    rom[16 + 0x1FFFF] = 0xD4;

    Cartriadge cart;
    int rc = load_cartridge_from_memory(rom, len, &cart);
    TEST_ASSERT_EQUAL_INT(0, rc);

    write_byte(0x8000, 0);
    TEST_ASSERT_EQUAL_UINT8(0xD1, read_byte(0xFFFF));

    write_byte(0x8000, 2);
    TEST_ASSERT_EQUAL_UINT8(0xD3, read_byte(0xFFFF));

    write_byte(0x8000, 3);
    TEST_ASSERT_EQUAL_UINT8(0xD4, read_byte(0xFFFF));

    write_byte(0x8000, 1);
    TEST_ASSERT_EQUAL_UINT8(0xD2, read_byte(0xFFFF));

    free(rom);
    free(cart.pg_rom); free(cart.ch_rom);
    free(cart.chr_ram); free(cart.prg_ram);
}

void test_mapper_7_mirroring(void) {
    int len;
    unsigned char *rom = build_rom(7, 8, 1, &len);

    Cartriadge cart;
    int rc = load_cartridge_from_memory(rom, len, &cart);
    TEST_ASSERT_EQUAL_INT(0, rc);

    write_byte(0x8000, 0x10);
    TEST_ASSERT_EQUAL_INT(1, cart.mirroring_mode);

    write_byte(0x8000, 0x00);
    TEST_ASSERT_EQUAL_INT(0, cart.mirroring_mode);

    free(rom);
    free(cart.pg_rom); free(cart.ch_rom);
    free(cart.chr_ram); free(cart.prg_ram);
}

int main(void) {
    setbuf(stdout, NULL);
    UNITY_BEGIN();

    /* Mapper 0 */
    RUN_TEST(test_mapper_0_16k_prg);
    RUN_TEST(test_mapper_0_mirrors_16k);
    RUN_TEST(test_mapper_0_32k_prg);
    RUN_TEST(test_mapper_0_chr_read);
    RUN_TEST(test_mapper_0_bank_last_bytes);

    /* Mapper 2 */
    RUN_TEST(test_mapper_2_last_bank_fixed);
    RUN_TEST(test_mapper_2_switchable_bank);
    RUN_TEST(test_mapper_2_bank_last_bytes);
    RUN_TEST(test_mapper_2_bank_wrap);

    /* Mapper 3 */
    RUN_TEST(test_mapper_3_chr_banking);
    RUN_TEST(test_mapper_3_chr_last_bytes);

    /* Mapper 66 */
    RUN_TEST(test_mapper_66_prg_banking);
    RUN_TEST(test_mapper_66_prg_last_bytes);
    RUN_TEST(test_mapper_66_chr_bank_oob);

    /* Mapper 7 */
    RUN_TEST(test_mapper_7_prg_banking);
    RUN_TEST(test_mapper_7_last_bytes);
    RUN_TEST(test_mapper_7_mirroring);

    return UNITY_END();
}
