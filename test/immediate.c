#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "unity.h"
#include "cpu.h"
#include "bus.h"
#include "instructions.h"
#include "mapper.h"
#include "cartriadge.h"
#include "statusFlag.h"
#include "raylib.h"
#include <stdarg.h>

static Cartriadge test_cart;
static unsigned char test_rom_data[0xFFFF - 0x6000];
static int memPtr;

void setUp(void) {
    SetTraceLogLevel(LOG_NONE);
    memPtr = 0x8000 - 0x6000;
    memset(test_rom_data, 0, sizeof(0xFFFF - 0x6000));
    test_cart.mem = test_rom_data;
    test_cart.size = sizeof(test_rom_data);
    test_cart.mapper = M000;
    
    test_rom_data[0xFFFC - 0x6000] = 0x00; 
    test_rom_data[0xFFFD - 0x6000] = 0x80; 
    
    connectCartriadgeToBus(&test_cart);
    bootCPU(false);
}

void tearDown(void) {
    shutdownCPU();
}

void place_n_bytes(int n, ...) {
    va_list args;
    va_start(args, n); 
    for(int i = 0; i < n; i++)
    {
        test_rom_data[memPtr] = va_arg(args, int); 
        memPtr += 1;
    }
    va_end(args);
}

void execute_next_instruction(void) {
    ExecutionInfo info = getNextInstruction();
    executeInstruction(info);
}

// Helper function to verify flags are in expected state
void verify_flags(int carry, int zero, int interrupt, int decimal, int overflow, int negative) {
    TEST_ASSERT_EQUAL(zero, getCPUStatusFlag(ZERO));
    TEST_ASSERT_EQUAL(interrupt, getCPUStatusFlag(INTERRUPT));
    TEST_ASSERT_EQUAL(decimal, getCPUStatusFlag(DECIMAL));
    TEST_ASSERT_EQUAL(negative, getCPUStatusFlag(NEGATIVE));
    TEST_ASSERT_EQUAL(carry, getCPUStatusFlag(CARRY));
    TEST_ASSERT_EQUAL(overflow, getCPUStatusFlag(CPU_OVERFLOW));
}

// Helper to check all flags are clear (initial state)
void verify_all_flags_clear(void) {
    verify_flags(0, 0, 0, 0, 0, 0);
}

// ============================================================================
// IMMEDIATE ADDRESSING TESTS
// ============================================================================

void test_lda_immediate(void) {
    place_n_bytes(2, 0xA9, 0x42);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x42, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 0);
}

void test_lda_immediate_zero(void) {
    place_n_bytes(2, 0xA9, 0x00);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x00, readByte(getCPU_Accumulator()));
    verify_flags(0, 1, 0, 0, 0, 0);
}

void test_lda_immediate_negative(void) {
    place_n_bytes(2, 0xA9, 0x80);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x80, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 1);
}

void test_lda_immediate_max_positive(void) {
    place_n_bytes(2, 0xA9, 0x7F);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x7F, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 0);
}

void test_ldx_immediate(void) {
    place_n_bytes(2, 0xA2, 0x55);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x55, readByte(getCPU_XRegister()));
    verify_flags(0, 0, 0, 0, 0, 0);
}

void test_ldx_immediate_zero(void) {
    place_n_bytes(2, 0xA2, 0x00);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x00, readByte(getCPU_XRegister()));
    verify_flags(0, 1, 0, 0, 0, 0);
}

void test_ldx_immediate_negative(void) {
    place_n_bytes(2, 0xA2, 0xF0);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0xF0, readByte(getCPU_XRegister()));
    verify_flags(0, 0, 0, 0, 0, 1);
}
 
void test_ldy_immediate(void) {
    place_n_bytes(2, 0xA0, 0x33);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x33, readByte(getCPU_YRegister()));
    verify_flags(0, 0, 0, 0, 0, 0);
}

void test_ldy_immediate_zero(void) {
    place_n_bytes(2, 0xA0, 0x00);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x00, readByte(getCPU_YRegister()));
    verify_flags(0, 1, 0, 0, 0, 0);
}

void test_ldy_immediate_negative(void) {
    place_n_bytes(2, 0xA0, 0x88);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x88, readByte(getCPU_YRegister()));
    verify_flags(0, 0, 0, 0, 0, 1);
}

void test_adc_immediate(void) {
    place_n_bytes(2, 0xA9, 0x10);
    execute_next_instruction();
    
    place_n_bytes(2, 0x69, 0x20);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x30, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 0);
}

void test_adc_immediate_with_carry(void) {
    setCPUStatusFlag(CARRY, true);
    place_n_bytes(2, 0xA9, 0x10);
    execute_next_instruction();
    
    place_n_bytes(2, 0x69, 0x20);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x31, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 0);
}

void test_adc_immediate_carry_generated(void) {
    place_n_bytes(2, 0xA9, 0xFF);
    execute_next_instruction();
    
    place_n_bytes(2, 0x69, 0x01);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x00, readByte(getCPU_Accumulator()));
    verify_flags(1, 1, 0, 0, 0, 0);
}

void test_adc_immediate_overflow(void) {
    place_n_bytes(2, 0xA9, 0x7F);
    execute_next_instruction();
    
    place_n_bytes(2, 0x69, 0x01);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x80, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 1, 1);
}

void test_sbc_immediate_overflow(void) {
    place_n_bytes(2, 0xA9, 0x50);
    execute_next_instruction();
    
    place_n_bytes(1, 0x38);
    execute_next_instruction();
    
    place_n_bytes(2, 0xE9, 0xB0);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0xA0, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 1, 1);
}

void test_sbc_immediate_equal_with_carry_clear(void) {
    place_n_bytes(2, 0xA9, 0x50);
    execute_next_instruction();
    
    place_n_bytes(2, 0xE9, 0x50);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0xFF, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 1);
}

void test_sbc_immediate_with_carry_clear(void) {
    place_n_bytes(2, 0xA9, 0x50);
    execute_next_instruction();
    
    place_n_bytes(2, 0xE9, 0x20);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x2F, readByte(getCPU_Accumulator()));
    verify_flags(1, 0, 0, 0, 0, 0);
}

void test_sbc_immediate_equal_with_carry(void) {
    place_n_bytes(2, 0xA9, 0x50);
    execute_next_instruction();
    
    place_n_bytes(1, 0x38);
    execute_next_instruction();
    
    place_n_bytes(2, 0xE9, 0x50);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x00, readByte(getCPU_Accumulator()));
    verify_flags(1, 1, 0, 0, 0, 0);
}

void test_sbc_immediate_borrow_from_set(void) {
    place_n_bytes(2, 0xA9, 0x20);
    execute_next_instruction();
    
    place_n_bytes(1, 0x38);
    execute_next_instruction();
    
    place_n_bytes(2, 0xE9, 0x50);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0xD0, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 1);
}

void test_sbc_immediate_normal(void) {
    place_n_bytes(2, 0xA9, 0x50);
    execute_next_instruction();
    
    place_n_bytes(1, 0x38);
    execute_next_instruction();
    
    place_n_bytes(2, 0xE9, 0x20);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x30, readByte(getCPU_Accumulator()));
    verify_flags(1, 0, 0, 0, 0, 0);
}

void test_ora_immediate(void) {
    place_n_bytes(2, 0xA9, 0x0F);
    execute_next_instruction();
    
    place_n_bytes(2, 0x09, 0xF0);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0xFF, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 1);
}

void test_ora_immediate_zero(void) {
    place_n_bytes(2, 0xA9, 0x00);
    execute_next_instruction();
    
    place_n_bytes(2, 0x09, 0x00);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x00, readByte(getCPU_Accumulator()));
    verify_flags(0, 1, 0, 0, 0, 0);
}

void test_and_immediate(void) {
    place_n_bytes(2, 0xA9, 0xFF);
    execute_next_instruction();
    
    place_n_bytes(2, 0x29, 0x0F);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x0F, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 0);
}

void test_and_immediate_zero(void) {
    place_n_bytes(2, 0xA9, 0xF0);
    execute_next_instruction();
    
    place_n_bytes(2, 0x29, 0x0F);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x00, readByte(getCPU_Accumulator()));
    verify_flags(0, 1, 0, 0, 0, 0);
}

void test_eor_immediate(void) {
    place_n_bytes(2, 0xA9, 0xAA);
    execute_next_instruction();
    
    place_n_bytes(2, 0x49, 0x0F);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0xA5, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 1);
}

void test_eor_immediate_zero(void) {
    place_n_bytes(2, 0xA9, 0xFF);
    execute_next_instruction();
    
    place_n_bytes(2, 0x49, 0xFF);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x00, readByte(getCPU_Accumulator()));
    verify_flags(0, 1, 0, 0, 0, 0);
}

void test_cmp_immediate(void) {
    place_n_bytes(2, 0xA9, 0x50);
    execute_next_instruction();
    
    place_n_bytes(2, 0xC9, 0x50);
    execute_next_instruction();
    
    verify_flags(1, 1, 0, 0, 0, 0);
}

void test_cmp_immediate_greater(void) {
    place_n_bytes(2, 0xA9, 0x60);
    execute_next_instruction();
    
    place_n_bytes(2, 0xC9, 0x50);
    execute_next_instruction();
    
    verify_flags(1, 0, 0, 0, 0, 0);
}

void test_cmp_immediate_less(void) {
    place_n_bytes(2, 0xA9, 0x40);
    execute_next_instruction();
    
    place_n_bytes(2, 0xC9, 0x50);
    execute_next_instruction();
    
    verify_flags(0, 0, 0, 0, 0, 1);
}

void test_cmp_immediate_negative_result(void) {
    place_n_bytes(2, 0xA9, 0x80);
    execute_next_instruction();
    
    place_n_bytes(2, 0xC9, 0x7F);
    execute_next_instruction();
    
    verify_flags(1, 0, 0, 0, 0, 0);
}

void test_cpx_immediate(void) {
    place_n_bytes(2, 0xA2, 0x30);
    execute_next_instruction();
    
    place_n_bytes(2, 0xE0, 0x30);
    execute_next_instruction();
    
    verify_flags(1, 1, 0, 0, 0, 0);
}

void test_cpx_immediate_greater(void) {
    place_n_bytes(2, 0xA2, 0x40);
    execute_next_instruction();
    
    place_n_bytes(2, 0xE0, 0x30);
    execute_next_instruction();
    
    verify_flags(1, 0, 0, 0, 0, 0);
}

void test_cpy_immediate(void) {
    place_n_bytes(2, 0xA0, 0x25);
    execute_next_instruction();
    
    place_n_bytes(2, 0xC0, 0x25);
    execute_next_instruction();
    
    verify_flags(1, 1, 0, 0, 0, 0);
}

void test_cpy_immediate_greater(void) {
    place_n_bytes(2, 0xA0, 0x35);
    execute_next_instruction();
    
    place_n_bytes(2, 0xC0, 0x25);
    execute_next_instruction();
    
    verify_flags(1, 0, 0, 0, 0, 0);
}

// ============================================================================
// IMPLIED ADDRESSING TESTS - ACCUMULATOR OPERATIONS
// ============================================================================

void test_rol_accumulator(void) {
    place_n_bytes(2, 0xA9, 0x81);
    execute_next_instruction();
    
    place_n_bytes(1, 0x2A);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x02, readByte(getCPU_Accumulator()));
    verify_flags(1, 0, 0, 0, 0, 0);
}

void test_rol_accumulator_zero(void) {
    place_n_bytes(2, 0xA9, 0x80);
    execute_next_instruction();
    
    place_n_bytes(1, 0x2A);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x00, readByte(getCPU_Accumulator()));
    verify_flags(1, 1, 0, 0, 0, 0);
}

void test_asl_accumulator(void) {
    place_n_bytes(2, 0xA9, 0x42);
    execute_next_instruction();
    
    place_n_bytes(1, 0x0A);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x84, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 1);
}

void test_ror_accumulator(void) {
    place_n_bytes(2, 0xA9, 0x02);
    execute_next_instruction();
    
    place_n_bytes(1, 0x6A);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x01, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 0);
}

void test_ror_accumulator_with_carry(void) {
    setCPUStatusFlag(CARRY, true);
    place_n_bytes(2, 0xA9, 0x00);
    execute_next_instruction();
    
    place_n_bytes(1, 0x6A);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x80, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 1);
}

void test_lsr_accumulator(void) {
    place_n_bytes(2, 0xA9, 0x84);
    execute_next_instruction();
    
    place_n_bytes(1, 0x4A);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x42, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 0);
}

void test_lsr_accumulator_carry(void) {
    place_n_bytes(2, 0xA9, 0x85);
    execute_next_instruction();
    
    place_n_bytes(1, 0x4A);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x42, readByte(getCPU_Accumulator()));
    verify_flags(1, 0, 0, 0, 0, 0);
}

// ============================================================================
// IMPLIED ADDRESSING TESTS - TRANSFER OPERATIONS
// ============================================================================

void test_txa(void) {
    place_n_bytes(2, 0xA2, 0x55);
    execute_next_instruction();
    
    place_n_bytes(1, 0x8A);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x55, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 0);
}

void test_txa_zero(void) {
    place_n_bytes(2, 0xA2, 0x00);
    execute_next_instruction();
    
    place_n_bytes(1, 0x8A);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x00, readByte(getCPU_Accumulator()));
    verify_flags(0, 1, 0, 0, 0, 0);
}

void test_tya(void) {
    place_n_bytes(2, 0xA0, 0x33);
    execute_next_instruction();
    
    place_n_bytes(1, 0x98);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x33, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 0);
}

void test_tax(void) {
    place_n_bytes(2, 0xA9, 0x77);
    execute_next_instruction();
    
    place_n_bytes(1, 0xAA);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x77, readByte(getCPU_XRegister()));
    verify_flags(0, 0, 0, 0, 0, 0);
}

void test_tay(void) {
    place_n_bytes(2, 0xA9, 0x99);
    execute_next_instruction();
    
    place_n_bytes(1, 0xA8);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x99, readByte(getCPU_YRegister()));
    verify_flags(0, 0, 0, 0, 0, 1);
}

void test_tay_positive(void) {
    place_n_bytes(2, 0xA9, 0x45);
    execute_next_instruction();
    
    place_n_bytes(1, 0xA8);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x45, readByte(getCPU_YRegister()));
    verify_flags(0, 0, 0, 0, 0, 0);
}

// ============================================================================
// IMPLIED ADDRESSING TESTS - INCREMENT/DECREMENT OPERATIONS
// ============================================================================

void test_inx(void) {
    place_n_bytes(2, 0xA2, 0x10);
    execute_next_instruction();
    
    place_n_bytes(1, 0xE8);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x11, readByte(getCPU_XRegister()));
    verify_flags(0, 0, 0, 0, 0, 0);
}

void test_inx_overflow(void) {
    place_n_bytes(2, 0xA2, 0xFF);
    execute_next_instruction();
    
    place_n_bytes(1, 0xE8);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x00, readByte(getCPU_XRegister()));
    verify_flags(0, 1, 0, 0, 0, 0);
}

void test_iny(void) {
    place_n_bytes(2, 0xA0, 0x20);
    execute_next_instruction();
    
    place_n_bytes(1, 0xC8);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x21, readByte(getCPU_YRegister()));
    verify_flags(0, 0, 0, 0, 0, 0);
}

void test_dex(void) {
    place_n_bytes(2, 0xA2, 0x01);
    execute_next_instruction();
    
    place_n_bytes(1, 0xCA);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x00, readByte(getCPU_XRegister()));
    verify_flags(0, 1, 0, 0, 0, 0);
}

void test_dex_underflow(void) {
    place_n_bytes(2, 0xA2, 0x00);
    execute_next_instruction();
    
    place_n_bytes(1, 0xCA);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0xFF, readByte(getCPU_XRegister()));
    verify_flags(0, 0, 0, 0, 0, 1);
}

void test_dey(void) {
    place_n_bytes(2, 0xA0, 0x01);
    execute_next_instruction();
    
    place_n_bytes(1, 0x88);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x00, readByte(getCPU_YRegister()));
    verify_flags(0, 1, 0, 0, 0, 0);
}

// ============================================================================
// IMPLIED ADDRESSING TESTS - FLAG OPERATIONS
// ============================================================================

void test_sec(void) {
    place_n_bytes(1, 0x38);
    execute_next_instruction();
    
    verify_flags(1, 0, 0, 0, 0, 0);
}

void test_clc(void) {
    setCPUStatusFlag(CARRY, true);
    place_n_bytes(1, 0x18);
    execute_next_instruction();
    
    verify_flags(0, 0, 0, 0, 0, 0);
}

void test_sed(void) {
    place_n_bytes(1, 0xF8);
    execute_next_instruction();
    
    verify_flags(0, 0, 0, 1, 0, 0);
}

void test_cld(void) {
    setCPUStatusFlag(DECIMAL, true);
    place_n_bytes(1, 0xD8);
    execute_next_instruction();
    
    verify_flags(0, 0, 0, 0, 0, 0);
}

void test_sei(void) {
    place_n_bytes(1, 0x78);
    execute_next_instruction();
    
    verify_flags(0, 0, 1, 0, 0, 0);
}

void test_cli(void) {
    setCPUStatusFlag(INTERRUPT, true);
    place_n_bytes(1, 0x58);
    execute_next_instruction();
    
    verify_flags(0, 0, 0, 0, 0, 0);
}

void test_clv(void) {
    setCPUStatusFlag(CPU_OVERFLOW, true);
    place_n_bytes(1, 0xB8);
    execute_next_instruction();
    
    verify_flags(0, 0, 0, 0, 0, 0);
}

void test_nop(void) {
    unsigned char initial_a = readByte(getCPU_Accumulator());
    unsigned char initial_x = readByte(getCPU_XRegister());
    unsigned char initial_y = readByte(getCPU_YRegister());
    unsigned char initial_status = readByte(getCPU_StatusRegister());
    
    place_n_bytes(1, 0xEA);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(initial_a, readByte(getCPU_Accumulator()));
    TEST_ASSERT_EQUAL_HEX(initial_x, readByte(getCPU_XRegister()));
    TEST_ASSERT_EQUAL_HEX(initial_y, readByte(getCPU_YRegister()));
    TEST_ASSERT_EQUAL_HEX(initial_status, readByte(getCPU_StatusRegister()));
}

// ============================================================================
// STACK OPERATION TESTS
// ============================================================================

void test_pha_pla(void) {
    place_n_bytes(2, 0xA9, 0x42);
    execute_next_instruction();
    
    place_n_bytes(1, 0x48); // PHA
    execute_next_instruction();
    
    place_n_bytes(2, 0xA9, 0x00); // Clear accumulator
    execute_next_instruction();
    
    place_n_bytes(1, 0x68); // PLA
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x42, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 0);
}

void test_pha_pla_zero(void) {
    place_n_bytes(2, 0xA9, 0x00);
    execute_next_instruction();
    
    place_n_bytes(1, 0x48); // PHA
    execute_next_instruction();
    
    place_n_bytes(2, 0xA9, 0xFF); // Set accumulator to non-zero
    execute_next_instruction();
    
    place_n_bytes(1, 0x68); // PLA
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x00, readByte(getCPU_Accumulator()));
    verify_flags(0, 1, 0, 0, 0, 0);
}

void test_pha_pla_negative(void) {
    place_n_bytes(2, 0xA9, 0x80);
    execute_next_instruction();
    
    place_n_bytes(1, 0x48); // PHA
    execute_next_instruction();
    
    place_n_bytes(2, 0xA9, 0x00); // Clear accumulator
    execute_next_instruction();
    
    place_n_bytes(1, 0x68); // PLA
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x80, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 1);
}

void test_php_plp(void) {
    setCPUStatusFlag(CARRY, true);
    setCPUStatusFlag(INTERRUPT, true);
    
    place_n_bytes(1, 0x08); // PHP
    execute_next_instruction();
    
    setCPUStatusFlag(CARRY, false);
    setCPUStatusFlag(INTERRUPT, false);
    
    place_n_bytes(1, 0x28); // PLP
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL(1, getCPUStatusFlag(CARRY));
    TEST_ASSERT_EQUAL(1, getCPUStatusFlag(INTERRUPT));
}

void test_tsx(void) {
    // Stack pointer starts at 0xFF
    place_n_bytes(1, 0xBA); // TSX
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0xFF, readByte(getCPU_XRegister()));
    verify_flags(0, 0, 0, 0, 0, 1);
}

void test_txs(void) {
    place_n_bytes(2, 0xA2, 0x55);
    execute_next_instruction();
    
    place_n_bytes(1, 0x9A); // TXS
    execute_next_instruction();
    
    // Stack pointer should now be 0x55
    // Note: Stack pointer is at address STACK_ADDR in CPU memory
    TEST_ASSERT_EQUAL_HEX(0x55, readByte(getCPU_Stack()));
}

// ============================================================================
// BRANCHING INSTRUCTION TESTS
// ============================================================================

void test_bcc_taken(void) {
    // Start at 0x8000
    place_n_bytes(2, 0x90, 0x10); // BCC +0x10 (should be taken)
    // Next instruction should be at 0x8012 (0x8000 + 2 + 0x10)
    
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x8012, getPC());
}

void test_bcc_not_taken(void) {
    setCPUStatusFlag(CARRY, true);
    place_n_bytes(2, 0x90, 0x10); // BCC +0x10 (should NOT be taken)
    
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x8002, getPC()); // Should just go to next instruction
}

void test_bcs_taken(void) {
    setCPUStatusFlag(CARRY, true);
    place_n_bytes(2, 0xB0, 0x10); // BCS +0x10 (should be taken)
    
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x8012, getPC());
}

void test_bcs_not_taken(void) {
    place_n_bytes(2, 0xB0, 0x10); // BCS +0x10 (should NOT be taken)
    
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x8002, getPC());
}

void test_beq_taken(void) {
    setCPUStatusFlag(ZERO, true);
    place_n_bytes(2, 0xF0, 0x10); // BEQ +0x10 (should be taken)
    
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x8012, getPC());
}

void test_beq_not_taken(void) {
    place_n_bytes(2, 0xF0, 0x10); // BEQ +0x10 (should NOT be taken)
    
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x8002, getPC());
}

void test_bne_taken(void) {
    place_n_bytes(2, 0xD0, 0x10); // BNE +0x10 (should be taken)
    
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x8012, getPC());
}

void test_bne_not_taken(void) {
    setCPUStatusFlag(ZERO, true);
    place_n_bytes(2, 0xD0, 0x10); // BNE +0x10 (should NOT be taken)
    
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x8002, getPC());
}

void test_bmi_taken(void) {
    setCPUStatusFlag(NEGATIVE, true);
    place_n_bytes(2, 0x30, 0x10); // BMI +0x10 (should be taken)
    
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x8012, getPC());
}

void test_bmi_not_taken(void) {
    place_n_bytes(2, 0x30, 0x10); // BMI +0x10 (should NOT be taken)
    
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x8002, getPC());
}

void test_bpl_taken(void) {
    place_n_bytes(2, 0x10, 0x10); // BPL +0x10 (should be taken)
    
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x8012, getPC());
}

void test_bpl_not_taken(void) {
    setCPUStatusFlag(NEGATIVE, true);
    place_n_bytes(2, 0x10, 0x10); // BPL +0x10 (should NOT be taken)
    
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x8002, getPC());
}

void test_bvc_taken(void) {
    place_n_bytes(2, 0x50, 0x10); // BVC +0x10 (should be taken)
    
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x8012, getPC());
}

void test_bvc_not_taken(void) {
    setCPUStatusFlag(CPU_OVERFLOW, true);
    place_n_bytes(2, 0x50, 0x10); // BVC +0x10 (should NOT be taken)
    
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x8002, getPC());
}

void test_bvs_taken(void) {
    setCPUStatusFlag(CPU_OVERFLOW, true);
    place_n_bytes(2, 0x70, 0x10); // BVS +0x10 (should be taken)
    
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x8012, getPC());
}

void test_bvs_not_taken(void) {
    place_n_bytes(2, 0x70, 0x10); // BVS +0x10 (should NOT be taken)
    
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x8002, getPC());
}

void test_branch_backward(void) {
    // Test negative branch (backward)
    place_n_bytes(2, 0x90, 0xFC); // BCC -4 (should be taken)
    // This should branch to 0x7FFE (0x8000 + 2 - 4)
    
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x7FFE, getPC());
}

// ============================================================================
// JUMP AND SUBROUTINE TESTS
// ============================================================================

void test_jmp_absolute(void) {
    place_n_bytes(3, 0x4C, 0x34, 0x12); // JMP $1234
    
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x1234, getPC());
}

void test_jsr_rts(void) {
    // JSR to subroutine at 0x1234
    place_n_bytes(3, 0x20, 0x34, 0x12); // JSR $1234
    
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x1234, getPC());
    
    // Now at subroutine, execute RTS to return
    writeByte(0x1234, 0x60); //RTS
    
    execute_next_instruction();
    
    // Should return to instruction after JSR (0x8003)
    TEST_ASSERT_EQUAL_HEX(0x8003, getPC());
}

void test_rti(void) {
    // Set up stack with status and return address
    pushToStack(0x80); // High byte of return address
    pushToStack(0x00); // Low byte of return address  
    pushToStack(0xFF); // Status register with all flags set
    
    place_n_bytes(1, 0x40); // RTI
    
    execute_next_instruction();
    
    // Should return to address 0x8000 with status 0xFF
    TEST_ASSERT_EQUAL_HEX(0x8000, getPC());
    TEST_ASSERT_EQUAL_HEX(0xFF, readByte(getCPU_StatusRegister()));
}

// ============================================================================
// ZERO PAGE ADDRESSING TESTS
// ============================================================================

void test_lda_zero_page(void) {
    writeByte(0x80, 0x42);
    place_n_bytes(2, 0xA5, 0x80);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x42, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 0);
}

void test_lda_zero_page_zero(void) {
    writeByte(0x81, 0x00);
    place_n_bytes(2, 0xA5, 0x81);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x00, readByte(getCPU_Accumulator()));
    verify_flags(0, 1, 0, 0, 0, 0);
}

void test_lda_zero_page_negative(void) {
    writeByte(0x82, 0x80);
    place_n_bytes(2, 0xA5, 0x82);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x80, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 1);
}

void test_ldx_zero_page(void) {
    writeByte(0x83, 0x55);
    place_n_bytes(2, 0xA6, 0x83);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x55, readByte(getCPU_XRegister()));
    verify_flags(0, 0, 0, 0, 0, 0);
}

void test_ldy_zero_page(void) {
    writeByte(0x84, 0x33);
    place_n_bytes(2, 0xA4, 0x84);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x33, readByte(getCPU_YRegister()));
    verify_flags(0, 0, 0, 0, 0, 0);
}

void test_sta_zero_page(void) {
    place_n_bytes(2, 0xA9, 0x77);
    execute_next_instruction();
    
    place_n_bytes(2, 0x85, 0x85);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x77, readByte(0x85));
}

void test_stx_zero_page(void) {
    place_n_bytes(2, 0xA2, 0x88);
    execute_next_instruction();
    
    place_n_bytes(2, 0x86, 0x86);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x88, readByte(0x86));
}

void test_sty_zero_page(void) {
    place_n_bytes(2, 0xA0, 0x99);
    execute_next_instruction();
    
    place_n_bytes(2, 0x84, 0x87);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x99, readByte(0x87));
}

void test_inc_zero_page(void) {
    writeByte(0x88, 0x41);
    place_n_bytes(2, 0xE6, 0x88);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x42, readByte(0x88));
    verify_flags(0, 0, 0, 0, 0, 0);
}

void test_inc_zero_page_overflow(void) {
    writeByte(0x89, 0xFF);
    place_n_bytes(2, 0xE6, 0x89);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x00, readByte(0x89));
    verify_flags(0, 1, 0, 0, 0, 0);
}

void test_dec_zero_page(void) {
    writeByte(0x8A, 0x43);
    place_n_bytes(2, 0xC6, 0x8A);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x42, readByte(0x8A));
    verify_flags(0, 0, 0, 0, 0, 0);
}

void test_dec_zero_page_zero(void) {
    writeByte(0x8B, 0x01);
    place_n_bytes(2, 0xC6, 0x8B);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x00, readByte(0x8B));
    verify_flags(0, 1, 0, 0, 0, 0);
}

void test_asl_zero_page(void) {
    writeByte(0x8C, 0x42);
    place_n_bytes(2, 0x06, 0x8C);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x84, readByte(0x8C));
    verify_flags(0, 0, 0, 0, 0, 1);
}

void test_asl_zero_page_carry(void) {
    writeByte(0x8D, 0x81);
    place_n_bytes(2, 0x06, 0x8D);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x02, readByte(0x8D));
    verify_flags(1, 0, 0, 0, 0, 0);
}

void test_lsr_zero_page(void) {
    writeByte(0x8E, 0x84);
    place_n_bytes(2, 0x46, 0x8E);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x42, readByte(0x8E));
    verify_flags(0, 0, 0, 0, 0, 0);
}

void test_lsr_zero_page_carry(void) {
    writeByte(0x8F, 0x85);
    place_n_bytes(2, 0x46, 0x8F);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x42, readByte(0x8F));
    verify_flags(1, 0, 0, 0, 0, 0);
}

void test_rol_zero_page(void) {
    writeByte(0x90, 0x81);
    place_n_bytes(2, 0x26, 0x90);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x02, readByte(0x90));
    verify_flags(1, 0, 0, 0, 0, 0);
}

void test_rol_zero_page_with_carry(void) {
    setCPUStatusFlag(CARRY, true);
    writeByte(0x91, 0x00);
    place_n_bytes(2, 0x26, 0x91);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x01, readByte(0x91));
    verify_flags(0, 0, 0, 0, 0, 0);
}

void test_ror_zero_page(void) {
    writeByte(0x92, 0x02);
    place_n_bytes(2, 0x66, 0x92);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x01, readByte(0x92));
    verify_flags(0, 0, 0, 0, 0, 0);
}

void test_ror_zero_page_with_carry(void) {
    setCPUStatusFlag(CARRY, true);
    writeByte(0x93, 0x00);
    place_n_bytes(2, 0x66, 0x93);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x80, readByte(0x93));
    verify_flags(0, 0, 0, 0, 0, 1);
}

void test_adc_zero_page(void) {
    writeByte(0x94, 0x20);
    place_n_bytes(2, 0xA9, 0x10);
    execute_next_instruction();
    
    place_n_bytes(2, 0x65, 0x94);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x30, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 0);
}

void test_sbc_zero_page(void) {
    writeByte(0x95, 0x20);
    place_n_bytes(2, 0xA9, 0x50);
    execute_next_instruction();
    
    place_n_bytes(2, 0xE5, 0x95);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x2F, readByte(getCPU_Accumulator()));
    verify_flags(1, 0, 0, 0, 0, 0);
}

void test_ora_zero_page(void) {
    writeByte(0x96, 0xF0);
    place_n_bytes(2, 0xA9, 0x0F);
    execute_next_instruction();
    
    place_n_bytes(2, 0x05, 0x96);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0xFF, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 1);
}

void test_and_zero_page(void) {
    writeByte(0x97, 0x0F);
    place_n_bytes(2, 0xA9, 0xFF);
    execute_next_instruction();
    
    place_n_bytes(2, 0x25, 0x97);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x0F, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 0);
}

void test_eor_zero_page(void) {
    writeByte(0x98, 0x0F);
    place_n_bytes(2, 0xA9, 0xAA);
    execute_next_instruction();
    
    place_n_bytes(2, 0x45, 0x98);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0xA5, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 1);
}

void test_bit_zero_page(void) {
    writeByte(0x99, 0xC0);
    place_n_bytes(2, 0xA9, 0x3F);
    execute_next_instruction();
    
    place_n_bytes(2, 0x24, 0x99);
    execute_next_instruction();
    
    verify_flags(0, 1, 0, 0, 1, 1);
}

void test_cmp_zero_page(void) {
    writeByte(0x9A, 0x50);
    place_n_bytes(2, 0xA9, 0x50);
    execute_next_instruction();
    
    place_n_bytes(2, 0xC5, 0x9A);
    execute_next_instruction();
    
    verify_flags(1, 1, 0, 0, 0, 0);
}

void test_cpx_zero_page(void) {
    writeByte(0x9B, 0x30);
    place_n_bytes(2, 0xA2, 0x30);
    execute_next_instruction();
    
    place_n_bytes(2, 0xE4, 0x9B);
    execute_next_instruction();
    
    verify_flags(1, 1, 0, 0, 0, 0);
}

void test_cpy_zero_page(void) {
    writeByte(0x9C, 0x25);
    place_n_bytes(2, 0xA0, 0x25);
    execute_next_instruction();
    
    place_n_bytes(2, 0xC4, 0x9C);
    execute_next_instruction();
    
    verify_flags(1, 1, 0, 0, 0, 0);
}

// ============================================================================
// ZERO PAGE,X AND ZERO PAGE,Y ADDRESSING TESTS
// ============================================================================

void test_lda_zero_page_x(void) {
    writeByte(0x90, 0x42);
    place_n_bytes(2, 0xA2, 0x10);
    execute_next_instruction();
    
    place_n_bytes(2, 0xB5, 0x80);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x42, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 0);
}

void test_lda_zero_page_x_wrap(void) {
    writeByte(0x0F, 0x77);
    place_n_bytes(2, 0xA2, 0x10);
    execute_next_instruction();
    
    place_n_bytes(2, 0xB5, 0xFF);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x77, readByte(getCPU_Accumulator()));
}

void test_ldy_zero_page_x(void) {
    writeByte(0x90, 0x33);
    place_n_bytes(2, 0xA2, 0x10);
    execute_next_instruction();
    
    place_n_bytes(2, 0xB4, 0x80);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x33, readByte(getCPU_YRegister()));
    verify_flags(0, 0, 0, 0, 0, 0);
}

void test_ldx_zero_page_y(void) {
    writeByte(0xA0, 0x55);
    place_n_bytes(2, 0xA0, 0x20);
    execute_next_instruction();
    
    place_n_bytes(2, 0xB6, 0x80);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x55, readByte(getCPU_XRegister()));
    verify_flags(0, 0, 0, 0, 0, 0);
}

void test_sta_zero_page_x(void) {
    place_n_bytes(2, 0xA9, 0x77);
    execute_next_instruction();
    
    place_n_bytes(2, 0xA2, 0x10);
    execute_next_instruction();
    
    place_n_bytes(2, 0x95, 0x80);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x77, readByte(0x90));
}

void test_sty_zero_page_x(void) {
    place_n_bytes(2, 0xA0, 0x99);
    execute_next_instruction();
    
    place_n_bytes(2, 0xA2, 0x10);
    execute_next_instruction();
    
    place_n_bytes(2, 0x94, 0x80);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x99, readByte(0x90));
}

void test_stx_zero_page_y(void) {
    place_n_bytes(2, 0xA2, 0x88);
    execute_next_instruction();
    
    place_n_bytes(2, 0xA0, 0x20);
    execute_next_instruction();
    
    place_n_bytes(2, 0x96, 0x80);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x88, readByte(0xA0));
}

void test_inc_zero_page_x(void) {
    writeByte(0x90, 0x41);
    place_n_bytes(2, 0xA2, 0x10);
    execute_next_instruction();
    
    place_n_bytes(2, 0xF6, 0x80);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x42, readByte(0x90));
    verify_flags(0, 0, 0, 0, 0, 0);
}

void test_dec_zero_page_x(void) {
    writeByte(0x90, 0x43);
    place_n_bytes(2, 0xA2, 0x10);
    execute_next_instruction();
    
    place_n_bytes(2, 0xD6, 0x80);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x42, readByte(0x90));
    verify_flags(0, 0, 0, 0, 0, 0);
}

void test_asl_zero_page_x(void) {
    writeByte(0x90, 0x42);
    place_n_bytes(2, 0xA2, 0x10);
    execute_next_instruction();
    
    place_n_bytes(2, 0x16, 0x80);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x84, readByte(0x90));
    verify_flags(0, 0, 0, 0, 0, 1);
}

void test_lsr_zero_page_x(void) {
    writeByte(0x90, 0x84);
    place_n_bytes(2, 0xA2, 0x10);
    execute_next_instruction();
    
    place_n_bytes(2, 0x56, 0x80);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x42, readByte(0x90));
    verify_flags(0, 0, 0, 0, 0, 0);
}

void test_rol_zero_page_x(void) {
    writeByte(0x90, 0x81);
    place_n_bytes(2, 0xA2, 0x10);
    execute_next_instruction();
    
    place_n_bytes(2, 0x36, 0x80);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x02, readByte(0x90));
    verify_flags(1, 0, 0, 0, 0, 0);
}

void test_ror_zero_page_x(void) {
    writeByte(0x90, 0x02);
    place_n_bytes(2, 0xA2, 0x10);
    execute_next_instruction();
    
    place_n_bytes(2, 0x76, 0x80);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x01, readByte(0x90));
    verify_flags(0, 0, 0, 0, 0, 0);
}

void test_adc_zero_page_x(void) {
    writeByte(0x90, 0x20);
    place_n_bytes(2, 0xA9, 0x10);
    execute_next_instruction();
    
    place_n_bytes(2, 0xA2, 0x10);
    execute_next_instruction();
    
    place_n_bytes(2, 0x75, 0x80);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x30, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 0);
}

void test_sbc_zero_page_x(void) {
    writeByte(0x90, 0x20);
    place_n_bytes(2, 0xA9, 0x50);
    execute_next_instruction();
    
    place_n_bytes(2, 0xA2, 0x10);
    execute_next_instruction();
    
    place_n_bytes(2, 0xF5, 0x80);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x2F, readByte(getCPU_Accumulator()));
    verify_flags(1, 0, 0, 0, 0, 0);
}

void test_ora_zero_page_x(void) {
    writeByte(0x90, 0xF0);
    place_n_bytes(2, 0xA9, 0x0F);
    execute_next_instruction();
    
    place_n_bytes(2, 0xA2, 0x10);
    execute_next_instruction();
    
    place_n_bytes(2, 0x15, 0x80);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0xFF, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 1);
}

void test_and_zero_page_x(void) {
    writeByte(0x90, 0x0F);
    place_n_bytes(2, 0xA9, 0xFF);
    execute_next_instruction();
    
    place_n_bytes(2, 0xA2, 0x10);
    execute_next_instruction();
    
    place_n_bytes(2, 0x35, 0x80);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x0F, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 0);
}

void test_eor_zero_page_x(void) {
    writeByte(0x90, 0x0F);
    place_n_bytes(2, 0xA9, 0xAA);
    execute_next_instruction();
    
    place_n_bytes(2, 0xA2, 0x10);
    execute_next_instruction();
    
    place_n_bytes(2, 0x55, 0x80);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0xA5, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 1);
}

void test_cmp_zero_page_x(void) {
    writeByte(0x90, 0x50);
    place_n_bytes(2, 0xA9, 0x50);
    execute_next_instruction();
    
    place_n_bytes(2, 0xA2, 0x10);
    execute_next_instruction();
    
    place_n_bytes(2, 0xD5, 0x80);
    execute_next_instruction();
    
    verify_flags(1, 1, 0, 0, 0, 0);
}

// ============================================================================
// ABSOLUTE ADDRESSING TESTS
// ============================================================================

void test_lda_absolute(void) {
    writeByte(0x1234, 0x42);
    place_n_bytes(3, 0xAD, 0x34, 0x12);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x42, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 0);
}

void test_lda_absolute_zero(void) {
    writeByte(0x1235, 0x00);
    place_n_bytes(3, 0xAD, 0x35, 0x12);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x00, readByte(getCPU_Accumulator()));
    verify_flags(0, 1, 0, 0, 0, 0);
}

void test_lda_absolute_negative(void) {
    writeByte(0x1236, 0x80);
    place_n_bytes(3, 0xAD, 0x36, 0x12);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x80, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 1);
}

void test_ldx_absolute(void) {
    writeByte(0x1237, 0x55);
    place_n_bytes(3, 0xAE, 0x37, 0x12);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x55, readByte(getCPU_XRegister()));
    verify_flags(0, 0, 0, 0, 0, 0);
}

void test_ldy_absolute(void) {
    writeByte(0x1238, 0x33);
    place_n_bytes(3, 0xAC, 0x38, 0x12);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x33, readByte(getCPU_YRegister()));
    verify_flags(0, 0, 0, 0, 0, 0);
}

void test_sta_absolute(void) {
    place_n_bytes(2, 0xA9, 0x77);
    execute_next_instruction();
    
    place_n_bytes(3, 0x8D, 0x39, 0x12);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x77, readByte(0x1239));
}

void test_stx_absolute(void) {
    place_n_bytes(2, 0xA2, 0x88);
    execute_next_instruction();
    
    place_n_bytes(3, 0x8E, 0x3A, 0x12);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x88, readByte(0x123A));
}

void test_sty_absolute(void) {
    place_n_bytes(2, 0xA0, 0x99);
    execute_next_instruction();
    
    place_n_bytes(3, 0x8C, 0x3B, 0x12);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x99, readByte(0x123B));
}

void test_inc_absolute(void) {
    writeByte(0x123C, 0x41);
    place_n_bytes(3, 0xEE, 0x3C, 0x12);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x42, readByte(0x123C));
    verify_flags(0, 0, 0, 0, 0, 0);
}

void test_inc_absolute_overflow(void) {
    writeByte(0x123D, 0xFF);
    place_n_bytes(3, 0xEE, 0x3D, 0x12);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x00, readByte(0x123D));
    verify_flags(0, 1, 0, 0, 0, 0);
}

void test_dec_absolute(void) {
    writeByte(0x123E, 0x43);
    place_n_bytes(3, 0xCE, 0x3E, 0x12);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x42, readByte(0x123E));
    verify_flags(0, 0, 0, 0, 0, 0);
}

void test_dec_absolute_zero(void) {
    writeByte(0x123F, 0x01);
    place_n_bytes(3, 0xCE, 0x3F, 0x12);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x00, readByte(0x123F));
    verify_flags(0, 1, 0, 0, 0, 0);
}

void test_asl_absolute(void) {
    writeByte(0x1240, 0x42);
    place_n_bytes(3, 0x0E, 0x40, 0x12);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x84, readByte(0x1240));
    verify_flags(0, 0, 0, 0, 0, 1);
}

void test_asl_absolute_carry(void) {
    writeByte(0x1241, 0x81);
    place_n_bytes(3, 0x0E, 0x41, 0x12);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x02, readByte(0x1241));
    verify_flags(1, 0, 0, 0, 0, 0);
}

void test_lsr_absolute(void) {
    writeByte(0x1242, 0x84);
    place_n_bytes(3, 0x4E, 0x42, 0x12);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x42, readByte(0x1242));
    verify_flags(0, 0, 0, 0, 0, 0);
}

void test_lsr_absolute_carry(void) {
    writeByte(0x1243, 0x85);
    place_n_bytes(3, 0x4E, 0x43, 0x12);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x42, readByte(0x1243));
    verify_flags(1, 0, 0, 0, 0, 0);
}

void test_rol_absolute(void) {
    writeByte(0x1244, 0x81);
    place_n_bytes(3, 0x2E, 0x44, 0x12);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x02, readByte(0x1244));
    verify_flags(1, 0, 0, 0, 0, 0);
}

void test_rol_absolute_with_carry(void) {
    setCPUStatusFlag(CARRY, true);
    writeByte(0x1245, 0x00);
    place_n_bytes(3, 0x2E, 0x45, 0x12);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x01, readByte(0x1245));
    verify_flags(0, 0, 0, 0, 0, 0);
}

void test_ror_absolute(void) {
    writeByte(0x1246, 0x02);
    place_n_bytes(3, 0x6E, 0x46, 0x12);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x01, readByte(0x1246));
    verify_flags(0, 0, 0, 0, 0, 0);
}

void test_ror_absolute_with_carry(void) {
    setCPUStatusFlag(CARRY, true);
    writeByte(0x1247, 0x00);
    place_n_bytes(3, 0x6E, 0x47, 0x12);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x80, readByte(0x1247));
    verify_flags(0, 0, 0, 0, 0, 1);
}

void test_adc_absolute(void) {
    writeByte(0x1248, 0x20);
    place_n_bytes(2, 0xA9, 0x10);
    execute_next_instruction();
    
    place_n_bytes(3, 0x6D, 0x48, 0x12);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x30, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 0);
}

void test_sbc_absolute(void) {
    writeByte(0x1249, 0x20);
    place_n_bytes(2, 0xA9, 0x50);
    execute_next_instruction();
    
    place_n_bytes(3, 0xED, 0x49, 0x12);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x2F, readByte(getCPU_Accumulator()));
    verify_flags(1, 0, 0, 0, 0, 0);
}

void test_ora_absolute(void) {
    writeByte(0x124A, 0xF0);
    place_n_bytes(2, 0xA9, 0x0F);
    execute_next_instruction();
    
    place_n_bytes(3, 0x0D, 0x4A, 0x12);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0xFF, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 1);
}

void test_and_absolute(void) {
    writeByte(0x124B, 0x0F);
    place_n_bytes(2, 0xA9, 0xFF);
    execute_next_instruction();
    
    place_n_bytes(3, 0x2D, 0x4B, 0x12);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x0F, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 0);
}

void test_eor_absolute(void) {
    writeByte(0x124C, 0x0F);
    place_n_bytes(2, 0xA9, 0xAA);
    execute_next_instruction();
    
    place_n_bytes(3, 0x4D, 0x4C, 0x12);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0xA5, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 1);
}

void test_cmp_absolute(void) {
    writeByte(0x124D, 0x50);
    place_n_bytes(2, 0xA9, 0x50);
    execute_next_instruction();
    
    place_n_bytes(3, 0xCD, 0x4D, 0x12);
    execute_next_instruction();
    
    verify_flags(1, 1, 0, 0, 0, 0);
}

void test_bit_absolute(void) {
    writeByte(0x124E, 0xC0);
    place_n_bytes(2, 0xA9, 0x3F);
    execute_next_instruction();
    
    place_n_bytes(3, 0x2C, 0x4E, 0x12);
    execute_next_instruction();
    
    verify_flags(0, 1, 0, 0, 1, 1);
}

void test_cpx_absolute(void) {
    writeByte(0x124F, 0x30);
    place_n_bytes(2, 0xA2, 0x30);
    execute_next_instruction();
    
    place_n_bytes(3, 0xEC, 0x4F, 0x12);
    execute_next_instruction();
    
    verify_flags(1, 1, 0, 0, 0, 0);
}

void test_cpy_absolute(void) {
    writeByte(0x1250, 0x25);
    place_n_bytes(2, 0xA0, 0x25);
    execute_next_instruction();
    
    place_n_bytes(3, 0xCC, 0x50, 0x12);
    execute_next_instruction();
    
    verify_flags(1, 1, 0, 0, 0, 0);
}

// ============================================================================
// ABSOLUTE,X AND ABSOLUTE,Y ADDRESSING TESTS
// ============================================================================

void test_lda_absolute_x(void) {
    writeByte(0x1244, 0x42);
    place_n_bytes(2, 0xA2, 0x10);
    execute_next_instruction();
    
    place_n_bytes(3, 0xBD, 0x34, 0x12);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x42, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 0);
}

void test_lda_absolute_y(void) {
    writeByte(0x1254, 0x77);
    place_n_bytes(2, 0xA0, 0x20);
    execute_next_instruction();
    
    place_n_bytes(3, 0xB9, 0x34, 0x12);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x77, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 0);
}

void test_ldy_absolute_x(void) {
    writeByte(0x1244, 0x33);
    place_n_bytes(2, 0xA2, 0x10);
    execute_next_instruction();
    
    place_n_bytes(3, 0xBC, 0x34, 0x12);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x33, readByte(getCPU_YRegister()));
    verify_flags(0, 0, 0, 0, 0, 0);
}

void test_ldx_absolute_y(void) {
    writeByte(0x1254, 0x55);
    place_n_bytes(2, 0xA0, 0x20);
    execute_next_instruction();
    
    place_n_bytes(3, 0xBE, 0x34, 0x12);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x55, readByte(getCPU_XRegister()));
    verify_flags(0, 0, 0, 0, 0, 0);
}

void test_sta_absolute_x(void) {
    place_n_bytes(2, 0xA9, 0x77);
    execute_next_instruction();
    
    place_n_bytes(2, 0xA2, 0x10);
    execute_next_instruction();
    
    place_n_bytes(3, 0x9D, 0x34, 0x12);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x77, readByte(0x1244));
}

void test_sta_absolute_y(void) {
    place_n_bytes(2, 0xA9, 0x88);
    execute_next_instruction();
    
    place_n_bytes(2, 0xA0, 0x20);
    execute_next_instruction();
    
    place_n_bytes(3, 0x99, 0x34, 0x12);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x88, readByte(0x1254));
}

void test_inc_absolute_x(void) {
    writeByte(0x1244, 0x41);
    place_n_bytes(2, 0xA2, 0x10);
    execute_next_instruction();
    
    place_n_bytes(3, 0xFE, 0x34, 0x12);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x42, readByte(0x1244));
    verify_flags(0, 0, 0, 0, 0, 0);
}

void test_dec_absolute_x(void) {
    writeByte(0x1244, 0x43);
    place_n_bytes(2, 0xA2, 0x10);
    execute_next_instruction();
    
    place_n_bytes(3, 0xDE, 0x34, 0x12);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x42, readByte(0x1244));
    verify_flags(0, 0, 0, 0, 0, 0);
}

void test_asl_absolute_x(void) {
    writeByte(0x1244, 0x42);
    place_n_bytes(2, 0xA2, 0x10);
    execute_next_instruction();
    
    place_n_bytes(3, 0x1E, 0x34, 0x12);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x84, readByte(0x1244));
    verify_flags(0, 0, 0, 0, 0, 1);
}

void test_lsr_absolute_x(void) {
    writeByte(0x1244, 0x84);
    place_n_bytes(2, 0xA2, 0x10);
    execute_next_instruction();
    
    place_n_bytes(3, 0x5E, 0x34, 0x12);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x42, readByte(0x1244));
    verify_flags(0, 0, 0, 0, 0, 0);
}

void test_rol_absolute_x(void) {
    writeByte(0x1244, 0x81);
    place_n_bytes(2, 0xA2, 0x10);
    execute_next_instruction();
    
    place_n_bytes(3, 0x3E, 0x34, 0x12);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x02, readByte(0x1244));
    verify_flags(1, 0, 0, 0, 0, 0);
}

void test_ror_absolute_x(void) {
    writeByte(0x1244, 0x02);
    place_n_bytes(2, 0xA2, 0x10);
    execute_next_instruction();
    
    place_n_bytes(3, 0x7E, 0x34, 0x12);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x01, readByte(0x1244));
    verify_flags(0, 0, 0, 0, 0, 0);
}

void test_adc_absolute_x(void) {
    writeByte(0x1244, 0x20);
    place_n_bytes(2, 0xA9, 0x10);
    execute_next_instruction();
    
    place_n_bytes(2, 0xA2, 0x10);
    execute_next_instruction();
    
    place_n_bytes(3, 0x7D, 0x34, 0x12);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x30, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 0);
}

void test_adc_absolute_y(void) {
    writeByte(0x1254, 0x20);
    place_n_bytes(2, 0xA9, 0x10);
    execute_next_instruction();
    
    place_n_bytes(2, 0xA0, 0x20);
    execute_next_instruction();
    
    place_n_bytes(3, 0x79, 0x34, 0x12);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x30, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 0);
}

void test_sbc_absolute_x(void) {
    writeByte(0x1244, 0x20);
    place_n_bytes(2, 0xA9, 0x50);
    execute_next_instruction();
    
    place_n_bytes(2, 0xA2, 0x10);
    execute_next_instruction();
    
    place_n_bytes(3, 0xFD, 0x34, 0x12);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x2F, readByte(getCPU_Accumulator()));
    verify_flags(1, 0, 0, 0, 0, 0);
}

void test_sbc_absolute_y(void) {
    writeByte(0x1254, 0x20);
    place_n_bytes(2, 0xA9, 0x50);
    execute_next_instruction();
    
    place_n_bytes(2, 0xA0, 0x20);
    execute_next_instruction();
    
    place_n_bytes(3, 0xF9, 0x34, 0x12);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x2F, readByte(getCPU_Accumulator()));
    verify_flags(1, 0, 0, 0, 0, 0);
}

void test_ora_absolute_x(void) {
    writeByte(0x1244, 0xF0);
    place_n_bytes(2, 0xA9, 0x0F);
    execute_next_instruction();
    
    place_n_bytes(2, 0xA2, 0x10);
    execute_next_instruction();
    
    place_n_bytes(3, 0x1D, 0x34, 0x12);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0xFF, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 1);
}

void test_ora_absolute_y(void) {
    writeByte(0x1254, 0xF0);
    place_n_bytes(2, 0xA9, 0x0F);
    execute_next_instruction();
    
    place_n_bytes(2, 0xA0, 0x20);
    execute_next_instruction();
    
    place_n_bytes(3, 0x19, 0x34, 0x12);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0xFF, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 1);
}

void test_and_absolute_x(void) {
    writeByte(0x1244, 0x0F);
    place_n_bytes(2, 0xA9, 0xFF);
    execute_next_instruction();
    
    place_n_bytes(2, 0xA2, 0x10);
    execute_next_instruction();
    
    place_n_bytes(3, 0x3D, 0x34, 0x12);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x0F, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 0);
}

void test_and_absolute_y(void) {
    writeByte(0x1254, 0x0F);
    place_n_bytes(2, 0xA9, 0xFF);
    execute_next_instruction();
    
    place_n_bytes(2, 0xA0, 0x20);
    execute_next_instruction();
    
    place_n_bytes(3, 0x39, 0x34, 0x12);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x0F, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 0);
}

void test_eor_absolute_x(void) {
    writeByte(0x1244, 0x0F);
    place_n_bytes(2, 0xA9, 0xAA);
    execute_next_instruction();
    
    place_n_bytes(2, 0xA2, 0x10);
    execute_next_instruction();
    
    place_n_bytes(3, 0x5D, 0x34, 0x12);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0xA5, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 1);
}

void test_eor_absolute_y(void) {
    writeByte(0x1254, 0x0F);
    place_n_bytes(2, 0xA9, 0xAA);
    execute_next_instruction();
    
    place_n_bytes(2, 0xA0, 0x20);
    execute_next_instruction();
    
    place_n_bytes(3, 0x59, 0x34, 0x12);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0xA5, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 1);
}

void test_cmp_absolute_x(void) {
    writeByte(0x1244, 0x50);
    place_n_bytes(2, 0xA9, 0x50);
    execute_next_instruction();
    
    place_n_bytes(2, 0xA2, 0x10);
    execute_next_instruction();
    
    place_n_bytes(3, 0xDD, 0x34, 0x12);
    execute_next_instruction();
    
    verify_flags(1, 1, 0, 0, 0, 0);
}

void test_cmp_absolute_y(void) {
    writeByte(0x1254, 0x50);
    place_n_bytes(2, 0xA9, 0x50);
    execute_next_instruction();
    
    place_n_bytes(2, 0xA0, 0x20);
    execute_next_instruction();
    
    place_n_bytes(3, 0xD9, 0x34, 0x12);
    execute_next_instruction();
    
    verify_flags(1, 1, 0, 0, 0, 0);
}

// ============================================================================
// MAIN TEST RUNNER
// ============================================================================

int main(void) {
    UNITY_BEGIN();
    
    // Immediate addressing tests
    RUN_TEST(test_lda_immediate);
    RUN_TEST(test_lda_immediate_zero);
    RUN_TEST(test_lda_immediate_negative);
    RUN_TEST(test_lda_immediate_max_positive);
    RUN_TEST(test_ldx_immediate);
    RUN_TEST(test_ldx_immediate_zero);
    RUN_TEST(test_ldx_immediate_negative);
    RUN_TEST(test_ldy_immediate);
    RUN_TEST(test_ldy_immediate_zero);
    RUN_TEST(test_ldy_immediate_negative);
    RUN_TEST(test_adc_immediate);
    RUN_TEST(test_adc_immediate_with_carry);
    RUN_TEST(test_adc_immediate_carry_generated);
    RUN_TEST(test_adc_immediate_overflow);
    RUN_TEST(test_sbc_immediate_borrow_from_set);
    RUN_TEST(test_sbc_immediate_equal_with_carry);
    RUN_TEST(test_sbc_immediate_equal_with_carry_clear);
    RUN_TEST(test_sbc_immediate_normal);
    RUN_TEST(test_sbc_immediate_overflow);
    RUN_TEST(test_sbc_immediate_with_carry_clear);
    RUN_TEST(test_ora_immediate);
    RUN_TEST(test_ora_immediate_zero);
    RUN_TEST(test_and_immediate);
    RUN_TEST(test_and_immediate_zero);
    RUN_TEST(test_eor_immediate);
    RUN_TEST(test_eor_immediate_zero);
    RUN_TEST(test_cmp_immediate);
    RUN_TEST(test_cmp_immediate_greater);
    RUN_TEST(test_cmp_immediate_less);
    RUN_TEST(test_cmp_immediate_negative_result);
    RUN_TEST(test_cpx_immediate);
    RUN_TEST(test_cpx_immediate_greater);
    RUN_TEST(test_cpy_immediate);
    RUN_TEST(test_cpy_immediate_greater);
    
    // Implied addressing tests - Accumulator operations
    RUN_TEST(test_rol_accumulator);
    RUN_TEST(test_rol_accumulator_zero);
    RUN_TEST(test_asl_accumulator);
    RUN_TEST(test_ror_accumulator);
    RUN_TEST(test_ror_accumulator_with_carry);
    RUN_TEST(test_lsr_accumulator);
    RUN_TEST(test_lsr_accumulator_carry);
    
    // Implied addressing tests - Transfer operations
    RUN_TEST(test_txa);
    RUN_TEST(test_txa_zero);
    RUN_TEST(test_tya);
    RUN_TEST(test_tax);
    RUN_TEST(test_tay);
    RUN_TEST(test_tay_positive);
    
    // Implied addressing tests - Increment/Decrement operations
    RUN_TEST(test_inx);
    RUN_TEST(test_inx_overflow);
    RUN_TEST(test_iny);
    RUN_TEST(test_dex);
    RUN_TEST(test_dex_underflow);
    RUN_TEST(test_dey);
    
    // Implied addressing tests - Flag operations
    RUN_TEST(test_sec);
    RUN_TEST(test_clc);
    RUN_TEST(test_sed);
    RUN_TEST(test_cld);
    RUN_TEST(test_sei);
    RUN_TEST(test_cli);
    RUN_TEST(test_clv);
    
    // Implied addressing tests - No operation
    RUN_TEST(test_nop);

    // Stack operation tests
    RUN_TEST(test_pha_pla);
    RUN_TEST(test_pha_pla_zero);
    RUN_TEST(test_pha_pla_negative);
    RUN_TEST(test_php_plp);
    RUN_TEST(test_tsx);
    RUN_TEST(test_txs);

    // Branching instruction tests
    RUN_TEST(test_bcc_taken);
    RUN_TEST(test_bcc_not_taken);
    RUN_TEST(test_bcs_taken);
    RUN_TEST(test_bcs_not_taken);
    RUN_TEST(test_beq_taken);
    RUN_TEST(test_beq_not_taken);
    RUN_TEST(test_bne_taken);
    RUN_TEST(test_bne_not_taken);
    RUN_TEST(test_bmi_taken);
    RUN_TEST(test_bmi_not_taken);
    RUN_TEST(test_bpl_taken);
    RUN_TEST(test_bpl_not_taken);
    RUN_TEST(test_bvc_taken);
    RUN_TEST(test_bvc_not_taken);
    RUN_TEST(test_bvs_taken);
    RUN_TEST(test_bvs_not_taken);
    RUN_TEST(test_branch_backward);

    // Jump and subroutine tests
    RUN_TEST(test_jmp_absolute);
    RUN_TEST(test_jsr_rts);
    RUN_TEST(test_rti);

    // Zero Page addressing tests
    RUN_TEST(test_lda_zero_page);
    RUN_TEST(test_lda_zero_page_zero);
    RUN_TEST(test_lda_zero_page_negative);
    RUN_TEST(test_ldx_zero_page);
    RUN_TEST(test_ldy_zero_page);
    RUN_TEST(test_sta_zero_page);
    RUN_TEST(test_stx_zero_page);
    RUN_TEST(test_sty_zero_page);
    RUN_TEST(test_inc_zero_page);
    RUN_TEST(test_inc_zero_page_overflow);
    RUN_TEST(test_dec_zero_page);
    RUN_TEST(test_dec_zero_page_zero);
    RUN_TEST(test_asl_zero_page);
    RUN_TEST(test_asl_zero_page_carry);
    RUN_TEST(test_lsr_zero_page);
    RUN_TEST(test_lsr_zero_page_carry);
    RUN_TEST(test_rol_zero_page);
    RUN_TEST(test_rol_zero_page_with_carry);
    RUN_TEST(test_ror_zero_page);
    RUN_TEST(test_ror_zero_page_with_carry);
    RUN_TEST(test_adc_zero_page);
    RUN_TEST(test_sbc_zero_page);
    RUN_TEST(test_ora_zero_page);
    RUN_TEST(test_and_zero_page);
    RUN_TEST(test_eor_zero_page);
    RUN_TEST(test_bit_zero_page);
    RUN_TEST(test_cmp_zero_page);
    RUN_TEST(test_cpx_zero_page);
    RUN_TEST(test_cpy_zero_page);

    // Zero Page,X and Zero Page,Y addressing tests
    RUN_TEST(test_lda_zero_page_x);
    RUN_TEST(test_lda_zero_page_x_wrap);
    RUN_TEST(test_ldy_zero_page_x);
    RUN_TEST(test_ldx_zero_page_y);
    RUN_TEST(test_sta_zero_page_x);
    RUN_TEST(test_sty_zero_page_x);
    RUN_TEST(test_stx_zero_page_y);
    RUN_TEST(test_inc_zero_page_x);
    RUN_TEST(test_dec_zero_page_x);
    RUN_TEST(test_asl_zero_page_x);
    RUN_TEST(test_lsr_zero_page_x);
    RUN_TEST(test_rol_zero_page_x);
    RUN_TEST(test_ror_zero_page_x);
    RUN_TEST(test_adc_zero_page_x);
    RUN_TEST(test_sbc_zero_page_x);
    RUN_TEST(test_ora_zero_page_x);
    RUN_TEST(test_and_zero_page_x);
    RUN_TEST(test_eor_zero_page_x);
    RUN_TEST(test_cmp_zero_page_x);

    // Absolute addressing tests
    RUN_TEST(test_lda_absolute);
    RUN_TEST(test_lda_absolute_zero);
    RUN_TEST(test_lda_absolute_negative);
    RUN_TEST(test_ldx_absolute);
    RUN_TEST(test_ldy_absolute);
    RUN_TEST(test_sta_absolute);
    RUN_TEST(test_stx_absolute);
    RUN_TEST(test_sty_absolute);
    RUN_TEST(test_inc_absolute);
    RUN_TEST(test_inc_absolute_overflow);
    RUN_TEST(test_dec_absolute);
    RUN_TEST(test_dec_absolute_zero);
    RUN_TEST(test_asl_absolute);
    RUN_TEST(test_asl_absolute_carry);
    RUN_TEST(test_lsr_absolute);
    RUN_TEST(test_lsr_absolute_carry);
    RUN_TEST(test_rol_absolute);
    RUN_TEST(test_rol_absolute_with_carry);
    RUN_TEST(test_ror_absolute);
    RUN_TEST(test_ror_absolute_with_carry);
    RUN_TEST(test_adc_absolute);
    RUN_TEST(test_sbc_absolute);
    RUN_TEST(test_ora_absolute);
    RUN_TEST(test_and_absolute);
    RUN_TEST(test_eor_absolute);
    RUN_TEST(test_cmp_absolute);
    RUN_TEST(test_bit_absolute);
    RUN_TEST(test_cpx_absolute);
    RUN_TEST(test_cpy_absolute);

    // Absolute,X and Absolute,Y addressing tests
    RUN_TEST(test_lda_absolute_x);
    RUN_TEST(test_lda_absolute_y);
    RUN_TEST(test_ldy_absolute_x);
    RUN_TEST(test_ldx_absolute_y);
    RUN_TEST(test_sta_absolute_x);
    RUN_TEST(test_sta_absolute_y);
    RUN_TEST(test_inc_absolute_x);
    RUN_TEST(test_dec_absolute_x);
    RUN_TEST(test_asl_absolute_x);
    RUN_TEST(test_lsr_absolute_x);
    RUN_TEST(test_rol_absolute_x);
    RUN_TEST(test_ror_absolute_x);
    RUN_TEST(test_adc_absolute_x);
    RUN_TEST(test_adc_absolute_y);
    RUN_TEST(test_sbc_absolute_x);
    RUN_TEST(test_sbc_absolute_y);
    RUN_TEST(test_ora_absolute_x);
    RUN_TEST(test_ora_absolute_y);
    RUN_TEST(test_and_absolute_x);
    RUN_TEST(test_and_absolute_y);
    RUN_TEST(test_eor_absolute_x);
    RUN_TEST(test_eor_absolute_y);
    RUN_TEST(test_cmp_absolute_x);
    RUN_TEST(test_cmp_absolute_y);
    
    return UNITY_END();
}