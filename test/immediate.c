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
#include "registerOffsets.h"
#include "addressingModes.h"
#include <stdarg.h>

static Cartriadge test_cart;
static unsigned char test_rom_data[0xFFFF - 0x6000];
static int memPtr;

void setUp(void) {
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
    TEST_ASSERT_EQUAL_HEX(0x8002, getPC());
}

void test_lda_immediate_zero(void) {
    place_n_bytes(2, 0xA9, 0x00);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x00, readByte(getCPU_Accumulator()));
    verify_flags(0, 1, 0, 0, 0, 0);
    TEST_ASSERT_EQUAL_HEX(0x8002, getPC());
}

void test_lda_immediate_negative(void) {
    place_n_bytes(2, 0xA9, 0x80);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x80, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 1);
    TEST_ASSERT_EQUAL_HEX(0x8002, getPC());
}

void test_lda_immediate_max_positive(void) {
    place_n_bytes(2, 0xA9, 0x7F);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x7F, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 0);
    TEST_ASSERT_EQUAL_HEX(0x8002, getPC());
}

void test_ldx_immediate(void) {
    place_n_bytes(2, 0xA2, 0x55);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x55, readByte(getCPU_XRegister()));
    verify_flags(0, 0, 0, 0, 0, 0);
    TEST_ASSERT_EQUAL_HEX(0x8002, getPC());
}

void test_ldx_immediate_zero(void) {
    place_n_bytes(2, 0xA2, 0x00);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x00, readByte(getCPU_XRegister()));
    verify_flags(0, 1, 0, 0, 0, 0);
    TEST_ASSERT_EQUAL_HEX(0x8002, getPC());
}

void test_ldx_immediate_negative(void) {
    place_n_bytes(2, 0xA2, 0xF0);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0xF0, readByte(getCPU_XRegister()));
    verify_flags(0, 0, 0, 0, 0, 1);
    TEST_ASSERT_EQUAL_HEX(0x8002, getPC());
}
 
void test_ldy_immediate(void) {
    place_n_bytes(2, 0xA0, 0x33);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x33, readByte(getCPU_YRegister()));
    verify_flags(0, 0, 0, 0, 0, 0);
    TEST_ASSERT_EQUAL_HEX(0x8002, getPC());
}

void test_ldy_immediate_zero(void) {
    place_n_bytes(2, 0xA0, 0x00);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x00, readByte(getCPU_YRegister()));
    verify_flags(0, 1, 0, 0, 0, 0);
    TEST_ASSERT_EQUAL_HEX(0x8002, getPC());
}

void test_ldy_immediate_negative(void) {
    place_n_bytes(2, 0xA0, 0x88);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x88, readByte(getCPU_YRegister()));
    verify_flags(0, 0, 0, 0, 0, 1);
    TEST_ASSERT_EQUAL_HEX(0x8002, getPC());
}

void test_adc_immediate(void) {
    place_n_bytes(2, 0xA9, 0x10);
    execute_next_instruction();
    
    place_n_bytes(2, 0x69, 0x20);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x30, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 0);
    TEST_ASSERT_EQUAL_HEX(0x8004, getPC());
}

void test_adc_immediate_with_carry(void) {
    setCPUStatusFlag(CARRY, true);
    place_n_bytes(2, 0xA9, 0x10);
    execute_next_instruction();
    
    place_n_bytes(2, 0x69, 0x20);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x31, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 0);
    TEST_ASSERT_EQUAL_HEX(0x8004, getPC());
}

void test_adc_immediate_carry_generated(void) {
    place_n_bytes(2, 0xA9, 0xFF);
    execute_next_instruction();
    
    place_n_bytes(2, 0x69, 0x01);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x00, readByte(getCPU_Accumulator()));
    verify_flags(1, 1, 0, 0, 0, 0);
    TEST_ASSERT_EQUAL_HEX(0x8004, getPC());
}

void test_adc_immediate_overflow(void) {
    place_n_bytes(2, 0xA9, 0x7F);
    execute_next_instruction();
    
    place_n_bytes(2, 0x69, 0x01);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x80, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 1, 1);
    TEST_ASSERT_EQUAL_HEX(0x8004, getPC());
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
    TEST_ASSERT_EQUAL_HEX(0x8005, getPC());
}

void test_sbc_immediate_equal_with_carry_clear(void) {
    place_n_bytes(2, 0xA9, 0x50);
    execute_next_instruction();
    
    place_n_bytes(2, 0xE9, 0x50);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0xFF, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 1);
    TEST_ASSERT_EQUAL_HEX(0x8004, getPC());
}

void test_sbc_immediate_with_carry_clear(void) {
    place_n_bytes(2, 0xA9, 0x50);
    execute_next_instruction();
    
    place_n_bytes(2, 0xE9, 0x20);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x2F, readByte(getCPU_Accumulator()));
    verify_flags(1, 0, 0, 0, 0, 0);
    TEST_ASSERT_EQUAL_HEX(0x8004, getPC());
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
    TEST_ASSERT_EQUAL_HEX(0x8005, getPC());
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
    TEST_ASSERT_EQUAL_HEX(0x8005, getPC());
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
    TEST_ASSERT_EQUAL_HEX(0x8005, getPC());
}

void test_ora_immediate(void) {
    place_n_bytes(2, 0xA9, 0x0F);
    execute_next_instruction();
    
    place_n_bytes(2, 0x09, 0xF0);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0xFF, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 1);
    TEST_ASSERT_EQUAL_HEX(0x8004, getPC());
}

void test_ora_immediate_zero(void) {
    place_n_bytes(2, 0xA9, 0x00);
    execute_next_instruction();
    
    place_n_bytes(2, 0x09, 0x00);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x00, readByte(getCPU_Accumulator()));
    verify_flags(0, 1, 0, 0, 0, 0);
    TEST_ASSERT_EQUAL_HEX(0x8004, getPC());
}

void test_and_immediate(void) {
    place_n_bytes(2, 0xA9, 0xFF);
    execute_next_instruction();
    
    place_n_bytes(2, 0x29, 0x0F);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x0F, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 0);
    TEST_ASSERT_EQUAL_HEX(0x8004, getPC());
}

void test_and_immediate_zero(void) {
    place_n_bytes(2, 0xA9, 0xF0);
    execute_next_instruction();
    
    place_n_bytes(2, 0x29, 0x0F);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x00, readByte(getCPU_Accumulator()));
    verify_flags(0, 1, 0, 0, 0, 0);
    TEST_ASSERT_EQUAL_HEX(0x8004, getPC());
}

void test_eor_immediate(void) {
    place_n_bytes(2, 0xA9, 0xAA);
    execute_next_instruction();
    
    place_n_bytes(2, 0x49, 0x0F);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0xA5, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 1);
    TEST_ASSERT_EQUAL_HEX(0x8004, getPC());
}

void test_eor_immediate_zero(void) {
    place_n_bytes(2, 0xA9, 0xFF);
    execute_next_instruction();
    
    place_n_bytes(2, 0x49, 0xFF);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x00, readByte(getCPU_Accumulator()));
    verify_flags(0, 1, 0, 0, 0, 0);
    TEST_ASSERT_EQUAL_HEX(0x8004, getPC());
}

void test_cmp_immediate(void) {
    place_n_bytes(2, 0xA9, 0x50);
    execute_next_instruction();
    
    place_n_bytes(2, 0xC9, 0x50);
    execute_next_instruction();
    
    verify_flags(1, 1, 0, 0, 0, 0);
    TEST_ASSERT_EQUAL_HEX(0x8004, getPC());
}

void test_cmp_immediate_greater(void) {
    place_n_bytes(2, 0xA9, 0x60);
    execute_next_instruction();
    
    place_n_bytes(2, 0xC9, 0x50);
    execute_next_instruction();
    
    verify_flags(1, 0, 0, 0, 0, 0);
    TEST_ASSERT_EQUAL_HEX(0x8004, getPC());
}

void test_cmp_immediate_less(void) {
    place_n_bytes(2, 0xA9, 0x40);
    execute_next_instruction();
    
    place_n_bytes(2, 0xC9, 0x50);
    execute_next_instruction();
    
    verify_flags(0, 0, 0, 0, 0, 1);
    TEST_ASSERT_EQUAL_HEX(0x8004, getPC());
}

void test_cmp_immediate_negative_result(void) {
    place_n_bytes(2, 0xA9, 0x80);
    execute_next_instruction();
    
    place_n_bytes(2, 0xC9, 0x7F);
    execute_next_instruction();
    
    verify_flags(1, 0, 0, 0, 0, 0);
    TEST_ASSERT_EQUAL_HEX(0x8004, getPC());
}

void test_cpx_immediate(void) {
    place_n_bytes(2, 0xA2, 0x30);
    execute_next_instruction();
    
    place_n_bytes(2, 0xE0, 0x30);
    execute_next_instruction();
    
    verify_flags(1, 1, 0, 0, 0, 0);
    TEST_ASSERT_EQUAL_HEX(0x8004, getPC());
}

void test_cpx_immediate_greater(void) {
    place_n_bytes(2, 0xA2, 0x40);
    execute_next_instruction();
    
    place_n_bytes(2, 0xE0, 0x30);
    execute_next_instruction();
    
    verify_flags(1, 0, 0, 0, 0, 0);
    TEST_ASSERT_EQUAL_HEX(0x8004, getPC());
}

void test_cpy_immediate(void) {
    place_n_bytes(2, 0xA0, 0x25);
    execute_next_instruction();
    
    place_n_bytes(2, 0xC0, 0x25);
    execute_next_instruction();
    
    verify_flags(1, 1, 0, 0, 0, 0);
    TEST_ASSERT_EQUAL_HEX(0x8004, getPC());
}

void test_cpy_immediate_greater(void) {
    place_n_bytes(2, 0xA0, 0x35);
    execute_next_instruction();
    
    place_n_bytes(2, 0xC0, 0x25);
    execute_next_instruction();
    
    verify_flags(1, 0, 0, 0, 0, 0);
    TEST_ASSERT_EQUAL_HEX(0x8004, getPC());
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
    TEST_ASSERT_EQUAL_HEX(0x8003, getPC());
}

void test_rol_accumulator_zero(void) {
    place_n_bytes(2, 0xA9, 0x80);
    execute_next_instruction();
    
    place_n_bytes(1, 0x2A);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x00, readByte(getCPU_Accumulator()));
    verify_flags(1, 1, 0, 0, 0, 0);
    TEST_ASSERT_EQUAL_HEX(0x8003, getPC());
}

void test_asl_accumulator(void) {
    place_n_bytes(2, 0xA9, 0x42);
    execute_next_instruction();
    
    place_n_bytes(1, 0x0A);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x84, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 1);
    TEST_ASSERT_EQUAL_HEX(0x8003, getPC());
}

void test_ror_accumulator(void) {
    place_n_bytes(2, 0xA9, 0x02);
    execute_next_instruction();
    
    place_n_bytes(1, 0x6A);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x01, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 0);
    TEST_ASSERT_EQUAL_HEX(0x8003, getPC());
}

void test_ror_accumulator_with_carry(void) {
    setCPUStatusFlag(CARRY, true);
    place_n_bytes(2, 0xA9, 0x00);
    execute_next_instruction();
    
    place_n_bytes(1, 0x6A);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x80, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 1);
    TEST_ASSERT_EQUAL_HEX(0x8003, getPC());
}

void test_lsr_accumulator(void) {
    place_n_bytes(2, 0xA9, 0x84);
    execute_next_instruction();
    
    place_n_bytes(1, 0x4A);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x42, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 0);
    TEST_ASSERT_EQUAL_HEX(0x8003, getPC());
}

void test_lsr_accumulator_carry(void) {
    place_n_bytes(2, 0xA9, 0x85);
    execute_next_instruction();
    
    place_n_bytes(1, 0x4A);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x42, readByte(getCPU_Accumulator()));
    verify_flags(1, 0, 0, 0, 0, 0);
    TEST_ASSERT_EQUAL_HEX(0x8003, getPC());
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
    TEST_ASSERT_EQUAL_HEX(0x8003, getPC());
}

void test_txa_zero(void) {
    place_n_bytes(2, 0xA2, 0x00);
    execute_next_instruction();
    
    place_n_bytes(1, 0x8A);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x00, readByte(getCPU_Accumulator()));
    verify_flags(0, 1, 0, 0, 0, 0);
    TEST_ASSERT_EQUAL_HEX(0x8003, getPC());
}

void test_tya(void) {
    place_n_bytes(2, 0xA0, 0x33);
    execute_next_instruction();
    
    place_n_bytes(1, 0x98);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x33, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 0);
    TEST_ASSERT_EQUAL_HEX(0x8003, getPC());
}

void test_tax(void) {
    place_n_bytes(2, 0xA9, 0x77);
    execute_next_instruction();
    
    place_n_bytes(1, 0xAA);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x77, readByte(getCPU_XRegister()));
    verify_flags(0, 0, 0, 0, 0, 0);
    TEST_ASSERT_EQUAL_HEX(0x8003, getPC());
}

void test_tay(void) {
    place_n_bytes(2, 0xA9, 0x99);
    execute_next_instruction();
    
    place_n_bytes(1, 0xA8);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x99, readByte(getCPU_YRegister()));
    verify_flags(0, 0, 0, 0, 0, 1);
    TEST_ASSERT_EQUAL_HEX(0x8003, getPC());
}

void test_tay_positive(void) {
    place_n_bytes(2, 0xA9, 0x45);
    execute_next_instruction();
    
    place_n_bytes(1, 0xA8);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x45, readByte(getCPU_YRegister()));
    verify_flags(0, 0, 0, 0, 0, 0);
    TEST_ASSERT_EQUAL_HEX(0x8003, getPC());
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
    TEST_ASSERT_EQUAL_HEX(0x8003, getPC());
}

void test_inx_overflow(void) {
    place_n_bytes(2, 0xA2, 0xFF);
    execute_next_instruction();
    
    place_n_bytes(1, 0xE8);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x00, readByte(getCPU_XRegister()));
    verify_flags(0, 1, 0, 0, 0, 0);
    TEST_ASSERT_EQUAL_HEX(0x8003, getPC());
}

void test_iny(void) {
    place_n_bytes(2, 0xA0, 0x20);
    execute_next_instruction();
    
    place_n_bytes(1, 0xC8);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x21, readByte(getCPU_YRegister()));
    verify_flags(0, 0, 0, 0, 0, 0);
    TEST_ASSERT_EQUAL_HEX(0x8003, getPC());
}

void test_dex(void) {
    place_n_bytes(2, 0xA2, 0x01);
    execute_next_instruction();
    
    place_n_bytes(1, 0xCA);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x00, readByte(getCPU_XRegister()));
    verify_flags(0, 1, 0, 0, 0, 0);
    TEST_ASSERT_EQUAL_HEX(0x8003, getPC());
}

void test_dex_underflow(void) {
    place_n_bytes(2, 0xA2, 0x00);
    execute_next_instruction();
    
    place_n_bytes(1, 0xCA);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0xFF, readByte(getCPU_XRegister()));
    verify_flags(0, 0, 0, 0, 0, 1);
    TEST_ASSERT_EQUAL_HEX(0x8003, getPC());
}

void test_dey(void) {
    place_n_bytes(2, 0xA0, 0x01);
    execute_next_instruction();
    
    place_n_bytes(1, 0x88);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x00, readByte(getCPU_YRegister()));
    verify_flags(0, 1, 0, 0, 0, 0);
    TEST_ASSERT_EQUAL_HEX(0x8003, getPC());
}

// ============================================================================
// IMPLIED ADDRESSING TESTS - FLAG OPERATIONS
// ============================================================================

void test_sec(void) {
    place_n_bytes(1, 0x38);
    execute_next_instruction();
    
    verify_flags(1, 0, 0, 0, 0, 0);
    TEST_ASSERT_EQUAL_HEX(0x8001, getPC());
}

void test_clc(void) {
    setCPUStatusFlag(CARRY, true);
    place_n_bytes(1, 0x18);
    execute_next_instruction();
    
    verify_flags(0, 0, 0, 0, 0, 0);
    TEST_ASSERT_EQUAL_HEX(0x8001, getPC());
}

void test_sed(void) {
    place_n_bytes(1, 0xF8);
    execute_next_instruction();
    
    verify_flags(0, 0, 0, 1, 0, 0);
    TEST_ASSERT_EQUAL_HEX(0x8001, getPC());
}

void test_cld(void) {
    setCPUStatusFlag(DECIMAL, true);
    place_n_bytes(1, 0xD8);
    execute_next_instruction();
    
    verify_flags(0, 0, 0, 0, 0, 0);
    TEST_ASSERT_EQUAL_HEX(0x8001, getPC());
}

void test_sei(void) {
    place_n_bytes(1, 0x78);
    execute_next_instruction();
    
    verify_flags(0, 0, 1, 0, 0, 0);
    TEST_ASSERT_EQUAL_HEX(0x8001, getPC());
}

void test_cli(void) {
    setCPUStatusFlag(INTERRUPT, true);
    place_n_bytes(1, 0x58);
    execute_next_instruction();
    
    verify_flags(0, 0, 0, 0, 0, 0);
    TEST_ASSERT_EQUAL_HEX(0x8001, getPC());
}

void test_clv(void) {
    setCPUStatusFlag(CPU_OVERFLOW, true);
    place_n_bytes(1, 0xB8);
    execute_next_instruction();
    
    verify_flags(0, 0, 0, 0, 0, 0);
    TEST_ASSERT_EQUAL_HEX(0x8001, getPC());
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
    TEST_ASSERT_EQUAL_HEX(0x8001, getPC());
}

// ============================================================================
// STACK OPERATION TESTS
// ============================================================================

void test_multiple_push_pop(void) {
    place_n_bytes(2, 0xA9, 0x11);
    execute_next_instruction();
    place_n_bytes(1, 0x48);
    execute_next_instruction();
    
    place_n_bytes(2, 0xA9, 0x22);
    execute_next_instruction();
    place_n_bytes(1, 0x48);
    execute_next_instruction();
    
    place_n_bytes(2, 0xA9, 0x33);
    execute_next_instruction();
    place_n_bytes(1, 0x48);
    execute_next_instruction();
    
    place_n_bytes(1, 0x68);
    execute_next_instruction();
    TEST_ASSERT_EQUAL_HEX(0x33, readByte(getCPU_Accumulator()));
    
    place_n_bytes(1, 0x68);
    execute_next_instruction();
    TEST_ASSERT_EQUAL_HEX(0x22, readByte(getCPU_Accumulator()));
    
    place_n_bytes(1, 0x68);
    execute_next_instruction();
    TEST_ASSERT_EQUAL_HEX(0x11, readByte(getCPU_Accumulator()));
    TEST_ASSERT_EQUAL_HEX(0x800C, getPC());
}

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
    TEST_ASSERT_EQUAL_HEX(0x8006, getPC());
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
    TEST_ASSERT_EQUAL_HEX(0x8006, getPC());
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
    TEST_ASSERT_EQUAL_HEX(0x8006, getPC());
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
    TEST_ASSERT_EQUAL_HEX(0x8002, getPC());
}

void test_tsx(void) {
    // Stack pointer starts at 0xFF
    place_n_bytes(1, 0xBA); // TSX
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0xFF, readByte(getCPU_XRegister()));
    verify_flags(0, 0, 0, 0, 0, 1);
    TEST_ASSERT_EQUAL_HEX(0x8001, getPC());
}

void test_txs(void) {
    place_n_bytes(2, 0xA2, 0x55);
    execute_next_instruction();
    
    place_n_bytes(1, 0x9A); // TXS
    execute_next_instruction();
    
    // Stack pointer should now be 0x55
    // Note: Stack pointer is at address STACK_ADDR in CPU memory
    TEST_ASSERT_EQUAL_HEX(0x55, readByte(getCPU_Stack()));
    TEST_ASSERT_EQUAL_HEX(0x8003, getPC());
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

void test_branch_max_forward(void) {
    place_n_bytes(2, 0x90, 0x7F);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x8081, getPC());
}

void test_branch_max_backward(void) {
    place_n_bytes(2, 0x90, 0x80);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x7F82, getPC());
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
    writeByte(0x1234, 0x60);
    
    place_n_bytes(3, 0x20, 0x34, 0x12);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x1234, getPC());
    
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x8003, getPC());
}

void test_jsr_stack_behavior(void) {
    unsigned char initial_sp = readByte(getCPU_Stack());
    
    writeByte(0x1234, 0x60);
    
    place_n_bytes(3, 0x20, 0x34, 0x12);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(initial_sp - 2, readByte(getCPU_Stack()));
    
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(initial_sp, readByte(getCPU_Stack()));
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
    TEST_ASSERT_EQUAL_HEX(0x8002, getPC());
}

void test_lda_zero_page_zero(void) {
    writeByte(0x81, 0x00);
    place_n_bytes(2, 0xA5, 0x81);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x00, readByte(getCPU_Accumulator()));
    verify_flags(0, 1, 0, 0, 0, 0);
    TEST_ASSERT_EQUAL_HEX(0x8002, getPC());
}

void test_lda_zero_page_negative(void) {
    writeByte(0x82, 0x80);
    place_n_bytes(2, 0xA5, 0x82);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x80, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 1);
    TEST_ASSERT_EQUAL_HEX(0x8002, getPC());
}

void test_ldx_zero_page(void) {
    writeByte(0x83, 0x55);
    place_n_bytes(2, 0xA6, 0x83);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x55, readByte(getCPU_XRegister()));
    verify_flags(0, 0, 0, 0, 0, 0);
    TEST_ASSERT_EQUAL_HEX(0x8002, getPC());
}

void test_ldy_zero_page(void) {
    writeByte(0x84, 0x33);
    place_n_bytes(2, 0xA4, 0x84);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x33, readByte(getCPU_YRegister()));
    verify_flags(0, 0, 0, 0, 0, 0);
    TEST_ASSERT_EQUAL_HEX(0x8002, getPC());
}

void test_sta_zero_page(void) {
    place_n_bytes(2, 0xA9, 0x77);
    execute_next_instruction();
    
    place_n_bytes(2, 0x85, 0x85);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x77, readByte(0x85));
    TEST_ASSERT_EQUAL_HEX(0x8004, getPC());
}

void test_stx_zero_page(void) {
    place_n_bytes(2, 0xA2, 0x88);
    execute_next_instruction();
    
    place_n_bytes(2, 0x86, 0x86);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x88, readByte(0x86));
    TEST_ASSERT_EQUAL_HEX(0x8004, getPC());
}

void test_sty_zero_page(void) {
    place_n_bytes(2, 0xA0, 0x99);
    execute_next_instruction();
    
    place_n_bytes(2, 0x84, 0x87);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x99, readByte(0x87));
    TEST_ASSERT_EQUAL_HEX(0x8004, getPC());
}

void test_inc_zero_page(void) {
    writeByte(0x88, 0x41);
    place_n_bytes(2, 0xE6, 0x88);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x42, readByte(0x88));
    verify_flags(0, 0, 0, 0, 0, 0);
    TEST_ASSERT_EQUAL_HEX(0x8002, getPC());
}

void test_inc_zero_page_overflow(void) {
    writeByte(0x89, 0xFF);
    place_n_bytes(2, 0xE6, 0x89);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x00, readByte(0x89));
    verify_flags(0, 1, 0, 0, 0, 0);
    TEST_ASSERT_EQUAL_HEX(0x8002, getPC());
}

void test_dec_zero_page(void) {
    writeByte(0x8A, 0x43);
    place_n_bytes(2, 0xC6, 0x8A);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x42, readByte(0x8A));
    verify_flags(0, 0, 0, 0, 0, 0);
    TEST_ASSERT_EQUAL_HEX(0x8002, getPC());
}

void test_dec_zero_page_zero(void) {
    writeByte(0x8B, 0x01);
    place_n_bytes(2, 0xC6, 0x8B);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x00, readByte(0x8B));
    verify_flags(0, 1, 0, 0, 0, 0);
    TEST_ASSERT_EQUAL_HEX(0x8002, getPC());
}

void test_asl_zero_page(void) {
    writeByte(0x8C, 0x42);
    place_n_bytes(2, 0x06, 0x8C);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x84, readByte(0x8C));
    verify_flags(0, 0, 0, 0, 0, 1);
    TEST_ASSERT_EQUAL_HEX(0x8002, getPC());
}

void test_asl_zero_page_carry(void) {
    writeByte(0x8D, 0x81);
    place_n_bytes(2, 0x06, 0x8D);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x02, readByte(0x8D));
    verify_flags(1, 0, 0, 0, 0, 0);
    TEST_ASSERT_EQUAL_HEX(0x8002, getPC());
}

void test_lsr_zero_page(void) {
    writeByte(0x8E, 0x84);
    place_n_bytes(2, 0x46, 0x8E);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x42, readByte(0x8E));
    verify_flags(0, 0, 0, 0, 0, 0);
    TEST_ASSERT_EQUAL_HEX(0x8002, getPC());
}

void test_lsr_zero_page_carry(void) {
    writeByte(0x8F, 0x85);
    place_n_bytes(2, 0x46, 0x8F);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x42, readByte(0x8F));
    verify_flags(1, 0, 0, 0, 0, 0);
    TEST_ASSERT_EQUAL_HEX(0x8002, getPC());
}

void test_rol_zero_page(void) {
    writeByte(0x90, 0x81);
    place_n_bytes(2, 0x26, 0x90);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x02, readByte(0x90));
    verify_flags(1, 0, 0, 0, 0, 0);
    TEST_ASSERT_EQUAL_HEX(0x8002, getPC());
}

void test_rol_zero_page_with_carry(void) {
    setCPUStatusFlag(CARRY, true);
    writeByte(0x91, 0x00);
    place_n_bytes(2, 0x26, 0x91);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x01, readByte(0x91));
    verify_flags(0, 0, 0, 0, 0, 0);
    TEST_ASSERT_EQUAL_HEX(0x8002, getPC());
}

void test_ror_zero_page(void) {
    writeByte(0x92, 0x02);
    place_n_bytes(2, 0x66, 0x92);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x01, readByte(0x92));
    verify_flags(0, 0, 0, 0, 0, 0);
    TEST_ASSERT_EQUAL_HEX(0x8002, getPC());
}

void test_ror_zero_page_with_carry(void) {
    setCPUStatusFlag(CARRY, true);
    writeByte(0x93, 0x00);
    place_n_bytes(2, 0x66, 0x93);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x80, readByte(0x93));
    verify_flags(0, 0, 0, 0, 0, 1);
    TEST_ASSERT_EQUAL_HEX(0x8002, getPC());
}

void test_adc_zero_page(void) {
    writeByte(0x94, 0x20);
    place_n_bytes(2, 0xA9, 0x10);
    execute_next_instruction();
    
    place_n_bytes(2, 0x65, 0x94);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x30, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 0);
    TEST_ASSERT_EQUAL_HEX(0x8004, getPC());
}

void test_sbc_zero_page(void) {
    writeByte(0x95, 0x20);
    place_n_bytes(2, 0xA9, 0x50);
    execute_next_instruction();
    
    place_n_bytes(2, 0xE5, 0x95);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x2F, readByte(getCPU_Accumulator()));
    verify_flags(1, 0, 0, 0, 0, 0);
    TEST_ASSERT_EQUAL_HEX(0x8004, getPC());
}

void test_ora_zero_page(void) {
    writeByte(0x96, 0xF0);
    place_n_bytes(2, 0xA9, 0x0F);
    execute_next_instruction();
    
    place_n_bytes(2, 0x05, 0x96);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0xFF, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 1);
    TEST_ASSERT_EQUAL_HEX(0x8004, getPC());
}

void test_and_zero_page(void) {
    writeByte(0x97, 0x0F);
    place_n_bytes(2, 0xA9, 0xFF);
    execute_next_instruction();
    
    place_n_bytes(2, 0x25, 0x97);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x0F, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 0);
    TEST_ASSERT_EQUAL_HEX(0x8004, getPC());
}

void test_eor_zero_page(void) {
    writeByte(0x98, 0x0F);
    place_n_bytes(2, 0xA9, 0xAA);
    execute_next_instruction();
    
    place_n_bytes(2, 0x45, 0x98);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0xA5, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 1);
    TEST_ASSERT_EQUAL_HEX(0x8004, getPC());
}

void test_bit_zero_page(void) {
    writeByte(0x99, 0xC0);
    place_n_bytes(2, 0xA9, 0x3F);
    execute_next_instruction();
    
    place_n_bytes(2, 0x24, 0x99);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x3F, readByte(getCPU_Accumulator()));
    verify_flags(0, 1, 0, 0, 1, 1);
    TEST_ASSERT_EQUAL_HEX(0x8004, getPC());
}

void test_cmp_zero_page(void) {
    writeByte(0x9A, 0x50);
    place_n_bytes(2, 0xA9, 0x50);
    execute_next_instruction();
    
    place_n_bytes(2, 0xC5, 0x9A);
    execute_next_instruction();
    
    verify_flags(1, 1, 0, 0, 0, 0);
    TEST_ASSERT_EQUAL_HEX(0x8004, getPC());
}

void test_cpx_zero_page(void) {
    writeByte(0x9B, 0x30);
    place_n_bytes(2, 0xA2, 0x30);
    execute_next_instruction();
    
    place_n_bytes(2, 0xE4, 0x9B);
    execute_next_instruction();
    
    verify_flags(1, 1, 0, 0, 0, 0);
    TEST_ASSERT_EQUAL_HEX(0x8004, getPC());
}

void test_cpy_zero_page(void) {
    writeByte(0x9C, 0x25);
    place_n_bytes(2, 0xA0, 0x25);
    execute_next_instruction();
    
    place_n_bytes(2, 0xC4, 0x9C);
    execute_next_instruction();
    
    verify_flags(1, 1, 0, 0, 0, 0);
    TEST_ASSERT_EQUAL_HEX(0x8004, getPC());
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
    TEST_ASSERT_EQUAL_HEX(0x8004, getPC());
}

void test_lda_zero_page_x_wrap(void) {
    writeByte(0x0F, 0x77);
    place_n_bytes(2, 0xA2, 0x10);
    execute_next_instruction();
    
    place_n_bytes(2, 0xB5, 0xFF);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x77, readByte(getCPU_Accumulator()));
    TEST_ASSERT_EQUAL_HEX(0x8004, getPC());
}

void test_ldy_zero_page_x(void) {
    writeByte(0x90, 0x33);
    place_n_bytes(2, 0xA2, 0x10);
    execute_next_instruction();
    
    place_n_bytes(2, 0xB4, 0x80);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x33, readByte(getCPU_YRegister()));
    verify_flags(0, 0, 0, 0, 0, 0);
    TEST_ASSERT_EQUAL_HEX(0x8004, getPC());
}

void test_ldx_zero_page_y(void) {
    writeByte(0xA0, 0x55);
    place_n_bytes(2, 0xA0, 0x20);
    execute_next_instruction();
    
    place_n_bytes(2, 0xB6, 0x80);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x55, readByte(getCPU_XRegister()));
    verify_flags(0, 0, 0, 0, 0, 0);
    TEST_ASSERT_EQUAL_HEX(0x8004, getPC());
}

void test_sta_zero_page_x(void) {
    place_n_bytes(2, 0xA9, 0x77);
    execute_next_instruction();
    
    place_n_bytes(2, 0xA2, 0x10);
    execute_next_instruction();
    
    place_n_bytes(2, 0x95, 0x80);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x77, readByte(0x90));
    TEST_ASSERT_EQUAL_HEX(0x8006, getPC());
}

void test_sty_zero_page_x(void) {
    place_n_bytes(2, 0xA0, 0x99);
    execute_next_instruction();
    
    place_n_bytes(2, 0xA2, 0x10);
    execute_next_instruction();
    
    place_n_bytes(2, 0x94, 0x80);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x99, readByte(0x90));
    TEST_ASSERT_EQUAL_HEX(0x8006, getPC());
}

void test_stx_zero_page_y(void) {
    place_n_bytes(2, 0xA2, 0x88);
    execute_next_instruction();
    
    place_n_bytes(2, 0xA0, 0x20);
    execute_next_instruction();
    
    place_n_bytes(2, 0x96, 0x80);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x88, readByte(0xA0));
    TEST_ASSERT_EQUAL_HEX(0x8006, getPC());
}

void test_inc_zero_page_x(void) {
    writeByte(0x90, 0x41);
    place_n_bytes(2, 0xA2, 0x10);
    execute_next_instruction();
    
    place_n_bytes(2, 0xF6, 0x80);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x42, readByte(0x90));
    verify_flags(0, 0, 0, 0, 0, 0);
    TEST_ASSERT_EQUAL_HEX(0x8004, getPC());
}

void test_dec_zero_page_x(void) {
    writeByte(0x90, 0x43);
    place_n_bytes(2, 0xA2, 0x10);
    execute_next_instruction();
    
    place_n_bytes(2, 0xD6, 0x80);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x42, readByte(0x90));
    verify_flags(0, 0, 0, 0, 0, 0);
    TEST_ASSERT_EQUAL_HEX(0x8004, getPC());
}

void test_asl_zero_page_x(void) {
    writeByte(0x90, 0x42);
    place_n_bytes(2, 0xA2, 0x10);
    execute_next_instruction();
    
    place_n_bytes(2, 0x16, 0x80);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x84, readByte(0x90));
    verify_flags(0, 0, 0, 0, 0, 1);
    TEST_ASSERT_EQUAL_HEX(0x8004, getPC());
}

void test_lsr_zero_page_x(void) {
    writeByte(0x90, 0x84);
    place_n_bytes(2, 0xA2, 0x10);
    execute_next_instruction();
    
    place_n_bytes(2, 0x56, 0x80);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x42, readByte(0x90));
    verify_flags(0, 0, 0, 0, 0, 0);
    TEST_ASSERT_EQUAL_HEX(0x8004, getPC());
}

void test_rol_zero_page_x(void) {
    writeByte(0x90, 0x81);
    place_n_bytes(2, 0xA2, 0x10);
    execute_next_instruction();
    
    place_n_bytes(2, 0x36, 0x80);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x02, readByte(0x90));
    verify_flags(1, 0, 0, 0, 0, 0);
    TEST_ASSERT_EQUAL_HEX(0x8004, getPC());
}

void test_ror_zero_page_x(void) {
    writeByte(0x90, 0x02);
    place_n_bytes(2, 0xA2, 0x10);
    execute_next_instruction();
    
    place_n_bytes(2, 0x76, 0x80);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x01, readByte(0x90));
    verify_flags(0, 0, 0, 0, 0, 0);
    TEST_ASSERT_EQUAL_HEX(0x8004, getPC());
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
    TEST_ASSERT_EQUAL_HEX(0x8006, getPC());
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
    TEST_ASSERT_EQUAL_HEX(0x8006, getPC());
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
    TEST_ASSERT_EQUAL_HEX(0x8006, getPC());
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
    TEST_ASSERT_EQUAL_HEX(0x8006, getPC());
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
    TEST_ASSERT_EQUAL_HEX(0x8006, getPC());
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
    TEST_ASSERT_EQUAL_HEX(0x8006, getPC());
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
    TEST_ASSERT_EQUAL_HEX(0x8003, getPC());
}

void test_lda_absolute_zero(void) {
    writeByte(0x1235, 0x00);
    place_n_bytes(3, 0xAD, 0x35, 0x12);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x00, readByte(getCPU_Accumulator()));
    verify_flags(0, 1, 0, 0, 0, 0);
    TEST_ASSERT_EQUAL_HEX(0x8003, getPC());
}

void test_lda_absolute_negative(void) {
    writeByte(0x1236, 0x80);
    place_n_bytes(3, 0xAD, 0x36, 0x12);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x80, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 1);
    TEST_ASSERT_EQUAL_HEX(0x8003, getPC());
}

void test_ldx_absolute(void) {
    writeByte(0x1237, 0x55);
    place_n_bytes(3, 0xAE, 0x37, 0x12);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x55, readByte(getCPU_XRegister()));
    verify_flags(0, 0, 0, 0, 0, 0);
    TEST_ASSERT_EQUAL_HEX(0x8003, getPC());
}

void test_ldy_absolute(void) {
    writeByte(0x1238, 0x33);
    place_n_bytes(3, 0xAC, 0x38, 0x12);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x33, readByte(getCPU_YRegister()));
    verify_flags(0, 0, 0, 0, 0, 0);
    TEST_ASSERT_EQUAL_HEX(0x8003, getPC());
}

void test_sta_absolute(void) {
    place_n_bytes(2, 0xA9, 0x77);
    execute_next_instruction();
    
    place_n_bytes(3, 0x8D, 0x39, 0x12);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x77, readByte(0x1239));
    TEST_ASSERT_EQUAL_HEX(0x8005, getPC());
}

void test_stx_absolute(void) {
    place_n_bytes(2, 0xA2, 0x88);
    execute_next_instruction();
    
    place_n_bytes(3, 0x8E, 0x3A, 0x12);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x88, readByte(0x123A));
    TEST_ASSERT_EQUAL_HEX(0x8005, getPC());
}

void test_sty_absolute(void) {
    place_n_bytes(2, 0xA0, 0x99);
    execute_next_instruction();
    
    place_n_bytes(3, 0x8C, 0x3B, 0x12);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x99, readByte(0x123B));
    TEST_ASSERT_EQUAL_HEX(0x8005, getPC());
}

void test_inc_absolute(void) {
    writeByte(0x123C, 0x41);
    place_n_bytes(3, 0xEE, 0x3C, 0x12);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x42, readByte(0x123C));
    verify_flags(0, 0, 0, 0, 0, 0);
    TEST_ASSERT_EQUAL_HEX(0x8003, getPC());
}

void test_inc_absolute_overflow(void) {
    writeByte(0x123D, 0xFF);
    place_n_bytes(3, 0xEE, 0x3D, 0x12);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x00, readByte(0x123D));
    verify_flags(0, 1, 0, 0, 0, 0);
    TEST_ASSERT_EQUAL_HEX(0x8003, getPC());
}

void test_dec_absolute(void) {
    writeByte(0x123E, 0x43);
    place_n_bytes(3, 0xCE, 0x3E, 0x12);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x42, readByte(0x123E));
    verify_flags(0, 0, 0, 0, 0, 0);
    TEST_ASSERT_EQUAL_HEX(0x8003, getPC());
}

void test_dec_absolute_zero(void) {
    writeByte(0x123F, 0x01);
    place_n_bytes(3, 0xCE, 0x3F, 0x12);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x00, readByte(0x123F));
    verify_flags(0, 1, 0, 0, 0, 0);
    TEST_ASSERT_EQUAL_HEX(0x8003, getPC());
}

void test_asl_absolute(void) {
    writeByte(0x1240, 0x42);
    place_n_bytes(3, 0x0E, 0x40, 0x12);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x84, readByte(0x1240));
    verify_flags(0, 0, 0, 0, 0, 1);
    TEST_ASSERT_EQUAL_HEX(0x8003, getPC());
}

void test_asl_absolute_carry(void) {
    writeByte(0x1241, 0x81);
    place_n_bytes(3, 0x0E, 0x41, 0x12);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x02, readByte(0x1241));
    verify_flags(1, 0, 0, 0, 0, 0);
    TEST_ASSERT_EQUAL_HEX(0x8003, getPC());
}

void test_lsr_absolute(void) {
    writeByte(0x1242, 0x84);
    place_n_bytes(3, 0x4E, 0x42, 0x12);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x42, readByte(0x1242));
    verify_flags(0, 0, 0, 0, 0, 0);
    TEST_ASSERT_EQUAL_HEX(0x8003, getPC());
}

void test_lsr_absolute_carry(void) {
    writeByte(0x1243, 0x85);
    place_n_bytes(3, 0x4E, 0x43, 0x12);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x42, readByte(0x1243));
    verify_flags(1, 0, 0, 0, 0, 0);
    TEST_ASSERT_EQUAL_HEX(0x8003, getPC());
}

void test_rol_absolute(void) {
    writeByte(0x1244, 0x81);
    place_n_bytes(3, 0x2E, 0x44, 0x12);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x02, readByte(0x1244));
    verify_flags(1, 0, 0, 0, 0, 0);
    TEST_ASSERT_EQUAL_HEX(0x8003, getPC());
}

void test_rol_absolute_with_carry(void) {
    setCPUStatusFlag(CARRY, true);
    writeByte(0x1245, 0x00);
    place_n_bytes(3, 0x2E, 0x45, 0x12);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x01, readByte(0x1245));
    verify_flags(0, 0, 0, 0, 0, 0);
    TEST_ASSERT_EQUAL_HEX(0x8003, getPC());
}

void test_ror_absolute(void) {
    writeByte(0x1246, 0x02);
    place_n_bytes(3, 0x6E, 0x46, 0x12);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x01, readByte(0x1246));
    verify_flags(0, 0, 0, 0, 0, 0);
    TEST_ASSERT_EQUAL_HEX(0x8003, getPC());
}

void test_ror_absolute_with_carry(void) {
    setCPUStatusFlag(CARRY, true);
    writeByte(0x1247, 0x00);
    place_n_bytes(3, 0x6E, 0x47, 0x12);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x80, readByte(0x1247));
    verify_flags(0, 0, 0, 0, 0, 1);
    TEST_ASSERT_EQUAL_HEX(0x8003, getPC());
}

void test_adc_absolute(void) {
    writeByte(0x1248, 0x20);
    place_n_bytes(2, 0xA9, 0x10);
    execute_next_instruction();
    
    place_n_bytes(3, 0x6D, 0x48, 0x12);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x30, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 0);
    TEST_ASSERT_EQUAL_HEX(0x8005, getPC());
}

void test_sbc_absolute(void) {
    writeByte(0x1249, 0x20);
    place_n_bytes(2, 0xA9, 0x50);
    execute_next_instruction();
    
    place_n_bytes(3, 0xED, 0x49, 0x12);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x2F, readByte(getCPU_Accumulator()));
    verify_flags(1, 0, 0, 0, 0, 0);
    TEST_ASSERT_EQUAL_HEX(0x8005, getPC());
}

void test_ora_absolute(void) {
    writeByte(0x124A, 0xF0);
    place_n_bytes(2, 0xA9, 0x0F);
    execute_next_instruction();
    
    place_n_bytes(3, 0x0D, 0x4A, 0x12);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0xFF, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 1);
    TEST_ASSERT_EQUAL_HEX(0x8005, getPC());
}

void test_and_absolute(void) {
    writeByte(0x124B, 0x0F);
    place_n_bytes(2, 0xA9, 0xFF);
    execute_next_instruction();
    
    place_n_bytes(3, 0x2D, 0x4B, 0x12);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x0F, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 0);
    TEST_ASSERT_EQUAL_HEX(0x8005, getPC());
}

void test_eor_absolute(void) {
    writeByte(0x124C, 0x0F);
    place_n_bytes(2, 0xA9, 0xAA);
    execute_next_instruction();
    
    place_n_bytes(3, 0x4D, 0x4C, 0x12);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0xA5, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 1);
    TEST_ASSERT_EQUAL_HEX(0x8005, getPC());
}

void test_cmp_absolute(void) {
    writeByte(0x124D, 0x50);
    place_n_bytes(2, 0xA9, 0x50);
    execute_next_instruction();
    
    place_n_bytes(3, 0xCD, 0x4D, 0x12);
    execute_next_instruction();
    
    verify_flags(1, 1, 0, 0, 0, 0);
    TEST_ASSERT_EQUAL_HEX(0x8005, getPC());
}

void test_bit_absolute(void) {
    writeByte(0x124E, 0xC0);
    place_n_bytes(2, 0xA9, 0x3F);
    execute_next_instruction();
    
    place_n_bytes(3, 0x2C, 0x4E, 0x12);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x3F, readByte(getCPU_Accumulator()));
    verify_flags(0, 1, 0, 0, 1, 1);
    TEST_ASSERT_EQUAL_HEX(0x8005, getPC());
}

void test_cpx_absolute(void) {
    writeByte(0x124F, 0x30);
    place_n_bytes(2, 0xA2, 0x30);
    execute_next_instruction();
    
    place_n_bytes(3, 0xEC, 0x4F, 0x12);
    execute_next_instruction();
    
    verify_flags(1, 1, 0, 0, 0, 0);
    TEST_ASSERT_EQUAL_HEX(0x8005, getPC());
}

void test_cpy_absolute(void) {
    writeByte(0x1250, 0x25);
    place_n_bytes(2, 0xA0, 0x25);
    execute_next_instruction();
    
    place_n_bytes(3, 0xCC, 0x50, 0x12);
    execute_next_instruction();
    
    verify_flags(1, 1, 0, 0, 0, 0);
    TEST_ASSERT_EQUAL_HEX(0x8005, getPC());
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
    TEST_ASSERT_EQUAL_HEX(0x8005, getPC());
}
void test_absolute_indexed_x_all_values(void) {
    // Use base near the end of a page to force page crossing
    unsigned short base = 0xEFF0;

    // Loop X = 0..0x0F to cover both non-crossing and crossing cases
    for (int x = 0; x <= 0xFF; x++) {
        // Prepare memory at effective address
        unsigned short effAddr = (base + x) & 0xFFFF;
        writeByte(effAddr, (unsigned char)x);

        // Load X register
        place_n_bytes(2, 0xA2, x); // LDX #$x
        execute_next_instruction();

        // LDA base,X
        unsigned char low = base & 0xFF;
        unsigned char high = (base >> 8) & 0xFF;
        place_n_bytes(3, 0xBD, low, high); // LDA abs,X
        execute_next_instruction();

        // Verify accumulator
        unsigned char acc = readByte(getCPU_Accumulator());
        TEST_ASSERT_EQUAL_HEX((unsigned char)x, acc);

        tearDown();
        setUp();
    }
}


void test_absolute_indexed_x_page_crossing(void) {
    unsigned short base = 0x10F8; // near end of page to trigger crossing
    unsigned char values[16];

    // Prepare memory values for X = 0..0x0F
    for (int x = 0; x <= 0x0F; x++) {
        values[x] = (unsigned char)(x * 3);
    }

    for (int x = 0; x <= 0x0F; x++) {
        
        // Write memory at effective address
        writeByte((base + x) & 0xFFFF, values[x]);
        
        // Load X register
        place_n_bytes(2, 0xA2, x); // LDX #$x
        execute_next_instruction();
        
        // LDA base,X
        place_n_bytes(3, 0xBD, base & 0xFF, (base >> 8) & 0xFF);
        execute_next_instruction();
        
        // Verify accumulator
        TEST_ASSERT_EQUAL_HEX(values[x], readByte(getCPU_Accumulator()));
        
        tearDown(); // cleanup for next iteration
        setUp(); // reset CPU/memory for each iteration
    }
}


void test_lda_absolute_y(void) {
    writeByte(0x1254, 0x77);
    place_n_bytes(2, 0xA0, 0x20);
    execute_next_instruction();
    
    place_n_bytes(3, 0xB9, 0x34, 0x12);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x77, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 0);
    TEST_ASSERT_EQUAL_HEX(0x8005, getPC());
}

void test_ldy_absolute_x(void) {
    writeByte(0x1244, 0x33);
    place_n_bytes(2, 0xA2, 0x10);
    execute_next_instruction();
    
    place_n_bytes(3, 0xBC, 0x34, 0x12);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x33, readByte(getCPU_YRegister()));
    verify_flags(0, 0, 0, 0, 0, 0);
    TEST_ASSERT_EQUAL_HEX(0x8005, getPC());
}

void test_ldx_absolute_y(void) {
    writeByte(0x1254, 0x55);
    place_n_bytes(2, 0xA0, 0x20);
    execute_next_instruction();
    
    place_n_bytes(3, 0xBE, 0x34, 0x12);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x55, readByte(getCPU_XRegister()));
    verify_flags(0, 0, 0, 0, 0, 0);
    TEST_ASSERT_EQUAL_HEX(0x8005, getPC());
}

void test_sta_absolute_x(void) {
    place_n_bytes(2, 0xA9, 0x77);
    execute_next_instruction();
    
    place_n_bytes(2, 0xA2, 0x10);
    execute_next_instruction();
    
    place_n_bytes(3, 0x9D, 0x34, 0x12);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x77, readByte(0x1244));
    TEST_ASSERT_EQUAL_HEX(0x8007, getPC());
}

void test_sta_absolute_y(void) {
    place_n_bytes(2, 0xA9, 0x88);
    execute_next_instruction();
    
    place_n_bytes(2, 0xA0, 0x20);
    execute_next_instruction();
    
    place_n_bytes(3, 0x99, 0x34, 0x12);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x88, readByte(0x1254));
    TEST_ASSERT_EQUAL_HEX(0x8007, getPC());
}

void test_inc_absolute_x(void) {
    writeByte(0x1244, 0x41);
    place_n_bytes(2, 0xA2, 0x10);
    execute_next_instruction();
    
    place_n_bytes(3, 0xFE, 0x34, 0x12);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x42, readByte(0x1244));
    verify_flags(0, 0, 0, 0, 0, 0);
    TEST_ASSERT_EQUAL_HEX(0x8005, getPC());
}

void test_dec_absolute_x(void) {
    writeByte(0x1244, 0x43);
    place_n_bytes(2, 0xA2, 0x10);
    execute_next_instruction();
    
    place_n_bytes(3, 0xDE, 0x34, 0x12);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x42, readByte(0x1244));
    verify_flags(0, 0, 0, 0, 0, 0);
    TEST_ASSERT_EQUAL_HEX(0x8005, getPC());
}

void test_asl_absolute_x(void) {
    writeByte(0x1244, 0x42);
    place_n_bytes(2, 0xA2, 0x10);
    execute_next_instruction();
    
    place_n_bytes(3, 0x1E, 0x34, 0x12);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x84, readByte(0x1244));
    verify_flags(0, 0, 0, 0, 0, 1);
    TEST_ASSERT_EQUAL_HEX(0x8005, getPC());
}

void test_lsr_absolute_x(void) {
    writeByte(0x1244, 0x84);
    place_n_bytes(2, 0xA2, 0x10);
    execute_next_instruction();
    
    place_n_bytes(3, 0x5E, 0x34, 0x12);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x42, readByte(0x1244));
    verify_flags(0, 0, 0, 0, 0, 0);
    TEST_ASSERT_EQUAL_HEX(0x8005, getPC());
}

void test_rol_absolute_x(void) {
    writeByte(0x1244, 0x81);
    place_n_bytes(2, 0xA2, 0x10);
    execute_next_instruction();
    
    place_n_bytes(3, 0x3E, 0x34, 0x12);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x02, readByte(0x1244));
    verify_flags(1, 0, 0, 0, 0, 0);
    TEST_ASSERT_EQUAL_HEX(0x8005, getPC());
}

void test_ror_absolute_x(void) {
    writeByte(0x1244, 0x02);
    place_n_bytes(2, 0xA2, 0x10);
    execute_next_instruction();
    
    place_n_bytes(3, 0x7E, 0x34, 0x12);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x01, readByte(0x1244));
    verify_flags(0, 0, 0, 0, 0, 0);
    TEST_ASSERT_EQUAL_HEX(0x8005, getPC());
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
    TEST_ASSERT_EQUAL_HEX(0x8007, getPC());
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
    TEST_ASSERT_EQUAL_HEX(0x8007, getPC());
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
    TEST_ASSERT_EQUAL_HEX(0x8007, getPC());
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
    TEST_ASSERT_EQUAL_HEX(0x8007, getPC());
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
    TEST_ASSERT_EQUAL_HEX(0x8007, getPC());
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
    TEST_ASSERT_EQUAL_HEX(0x8007, getPC());
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
    TEST_ASSERT_EQUAL_HEX(0x8007, getPC());
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
    TEST_ASSERT_EQUAL_HEX(0x8007, getPC());
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
    TEST_ASSERT_EQUAL_HEX(0x8007, getPC());
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
    TEST_ASSERT_EQUAL_HEX(0x8007, getPC());
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
    TEST_ASSERT_EQUAL_HEX(0x8007, getPC());
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
    TEST_ASSERT_EQUAL_HEX(0x8007, getPC());
}

void test_adc_exhaustive(void) {
    for(int a = 0; a <= 0xFF; a++) {
        for(int m = 0; m <= 0xFF; m++) {
            for(int carry_in = 0; carry_in <= 1; carry_in++) {
                // Set up test
                if(carry_in) {
                    setCPUStatusFlag(CARRY, true);
                } else {
                    setCPUStatusFlag(CARRY, false);
                }
                
                place_n_bytes(2, 0xA9, a);
                execute_next_instruction();
                
                place_n_bytes(2, 0x69, m);
                execute_next_instruction();
                
                // Calculate expected result
                int result = a + m + carry_in;
                unsigned char expected_a = result & 0xFF;
                int expected_carry = (result > 0xFF) ? 1 : 0;
                int expected_zero = (expected_a == 0) ? 1 : 0;
                int expected_negative = (expected_a & 0x80) ? 1 : 0;
                
                // Calculate overflow: V = (A^result) & (M^result) & 0x80
                // Overflow occurs when adding two positive numbers gives negative
                // or adding two negative numbers gives positive
                int expected_overflow = ((~(a ^ m) & (a ^ expected_a)) & 0x80) ? 1 : 0;
                
                // Verify
                unsigned char actual_a = readByte(getCPU_Accumulator());
                if(actual_a != expected_a || 
                   getCPUStatusFlag(CARRY) != expected_carry ||
                   getCPUStatusFlag(ZERO) != expected_zero ||
                   getCPUStatusFlag(NEGATIVE) != expected_negative ||
                   getCPUStatusFlag(CPU_OVERFLOW) != expected_overflow) {
                    
                    printf("ADC FAILED: A=0x%02X, M=0x%02X, C_in=%d\n", a, m, carry_in);
                    printf("Expected: A=0x%02X, C=%d, Z=%d, N=%d, V=%d\n", 
                           expected_a, expected_carry, expected_zero, expected_negative, expected_overflow);
                    printf("Got:      A=0x%02X, C=%d, Z=%d, N=%d, V=%d\n",
                           actual_a, getCPUStatusFlag(CARRY), getCPUStatusFlag(ZERO), 
                           getCPUStatusFlag(NEGATIVE), getCPUStatusFlag(CPU_OVERFLOW));
                    TEST_FAIL();
                    return;
                }
                
                // Reset for next iteration
                shutdownCPU();
                setUp();
            }
        }
    }
}

void test_sbc_exhaustive(void) {
    for(int a = 0; a <= 0xFF; a++) {
        for(int m = 0; m <= 0xFF; m++) {
            for(int carry_in = 0; carry_in <= 1; carry_in++) {
                // Set up test
                if(carry_in) {
                    setCPUStatusFlag(CARRY, true);
                } else {
                    setCPUStatusFlag(CARRY, false);
                }
                
                place_n_bytes(2, 0xA9, a);
                execute_next_instruction();
                
                place_n_bytes(2, 0xE9, m);
                execute_next_instruction();
                
                // Calculate expected result: A = A - M - (1 - C)
                int result = a - m - (1 - carry_in);
                unsigned char expected_a = result & 0xFF;
                int expected_carry = (result >= 0) ? 1 : 0;
                int expected_zero = (expected_a == 0) ? 1 : 0;
                int expected_negative = (expected_a & 0x80) ? 1 : 0;
                
                // Calculate overflow: V = ((A^M) & (A^result)) & 0x80
                // Overflow occurs when subtracting negative from positive gives negative
                // or subtracting positive from negative gives positive
                int expected_overflow = (((a ^ m) & (a ^ expected_a)) & 0x80) ? 1 : 0;
                
                // Verify
                unsigned char actual_a = readByte(getCPU_Accumulator());
                if(actual_a != expected_a || 
                   getCPUStatusFlag(CARRY) != expected_carry ||
                   getCPUStatusFlag(ZERO) != expected_zero ||
                   getCPUStatusFlag(NEGATIVE) != expected_negative ||
                   getCPUStatusFlag(CPU_OVERFLOW) != expected_overflow) {
                    
                    printf("SBC FAILED: A=0x%02X, M=0x%02X, C_in=%d\n", a, m, carry_in);
                    printf("Expected: A=0x%02X, C=%d, Z=%d, N=%d, V=%d\n", 
                           expected_a, expected_carry, expected_zero, expected_negative, expected_overflow);
                    printf("Got:      A=0x%02X, C=%d, Z=%d, N=%d, V=%d\n",
                           actual_a, getCPUStatusFlag(CARRY), getCPUStatusFlag(ZERO), 
                           getCPUStatusFlag(NEGATIVE), getCPUStatusFlag(CPU_OVERFLOW));
                    TEST_FAIL();
                    return;
                }
                
                // Reset for next iteration
                shutdownCPU();
                setUp();
            }
        }
    }
}

// ============================================================================
// INDIRECT,X ADDRESSING TESTS (Zero Page Indexed Indirect)
// ============================================================================

void test_lda_indirect_x(void) {
    // Setup: Store pointer at $10+$05 = $15 (low) and $16 (high)
    writeByte(0x15, 0x00); // Low byte of target address
    writeByte(0x16, 0x90); // High byte of target address
    writeByte(0x9000, 0x42); // Value to load
    
    place_n_bytes(2, 0xA2, 0x05); // LDX #$05
    execute_next_instruction();
    
    place_n_bytes(2, 0xA1, 0x10); // LDA ($10,X)
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x42, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 0);
    TEST_ASSERT_EQUAL_HEX(0x8004, getPC());
}

void test_lda_indirect_x_wrap(void) {
    // Test zero page wrap: $FF + $02 = $01 (wraps within zero page)
    writeByte(0x01, 0x34); // Low byte
    writeByte(0x02, 0x12); // High byte
    writeByte(0x1234, 0x77);
    
    place_n_bytes(2, 0xA2, 0x02); // LDX #$02
    execute_next_instruction();
    
    place_n_bytes(2, 0xA1, 0xFF); // LDA ($FF,X)
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x77, readByte(getCPU_Accumulator()));
    TEST_ASSERT_EQUAL_HEX(0x8004, getPC());
}

void test_sta_indirect_x(void) {
    place_n_bytes(2, 0xA9, 0x88); // LDA #$88
    execute_next_instruction();
    
    place_n_bytes(2, 0xA2, 0x04); // LDX #$04
    execute_next_instruction();
    
    writeByte(0x14, 0x00); // Low pointer at $10+$04
    writeByte(0x15, 0x95); // High pointer
    
    place_n_bytes(2, 0x81, 0x10); // STA ($10,X)
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x88, readByte(0x9500));
    TEST_ASSERT_EQUAL_HEX(0x8006, getPC());
}

void test_adc_indirect_x(void) {
    writeByte(0x20, 0x30); // Low pointer
    writeByte(0x21, 0x80); // High pointer
    writeByte(0x8030, 0x20); // Value to add
    
    place_n_bytes(2, 0xA9, 0x10); // LDA #$10
    execute_next_instruction();
    
    place_n_bytes(2, 0xA2, 0x05); // LDX #$05
    execute_next_instruction();
    
    place_n_bytes(2, 0x61, 0x1B); // ADC ($1B,X) -> ($20)
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x30, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 0);
    TEST_ASSERT_EQUAL_HEX(0x8006, getPC());
}

void test_sbc_indirect_x(void) {
    writeByte(0x30, 0x20); // Low pointer
    writeByte(0x31, 0x70); // High pointer
    writeByte(0x7020, 0x20); // Value to subtract
    
    place_n_bytes(2, 0xA9, 0x50); // LDA #$50
    execute_next_instruction();
    
    place_n_bytes(1, 0x38); // SEC
    execute_next_instruction();
    
    place_n_bytes(2, 0xA2, 0x06); // LDX #$06
    execute_next_instruction();
    
    place_n_bytes(2, 0xE1, 0x2A); // SBC ($2A,X) -> ($30)
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x30, readByte(getCPU_Accumulator()));
    verify_flags(1, 0, 0, 0, 0, 0);
    TEST_ASSERT_EQUAL_HEX(0x8007, getPC());
}

void test_and_indirect_x(void) {
    writeByte(0x40, 0x00); // Low pointer
    writeByte(0x41, 0x92); // High pointer
    writeByte(0x9200, 0x0F);
    
    place_n_bytes(2, 0xA9, 0xFF); // LDA #$FF
    execute_next_instruction();
    
    place_n_bytes(2, 0xA2, 0x03); // LDX #$03
    execute_next_instruction();
    
    place_n_bytes(2, 0x21, 0x3D); // AND ($3D,X) -> ($40)
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x0F, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 0);
    TEST_ASSERT_EQUAL_HEX(0x8006, getPC());
}

void test_ora_indirect_x(void) {
    writeByte(0x50, 0x10); // Low pointer
    writeByte(0x51, 0x93); // High pointer
    writeByte(0x9310, 0xF0);
    
    place_n_bytes(2, 0xA9, 0x0F); // LDA #$0F
    execute_next_instruction();
    
    place_n_bytes(2, 0xA2, 0x02); // LDX #$02
    execute_next_instruction();
    
    place_n_bytes(2, 0x01, 0x4E); // ORA ($4E,X) -> ($50)
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0xFF, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 1);
    TEST_ASSERT_EQUAL_HEX(0x8006, getPC());
}

void test_eor_indirect_x(void) {
    writeByte(0x60, 0x20); // Low pointer
    writeByte(0x61, 0x94); // High pointer
    writeByte(0x9420, 0x0F);
    
    place_n_bytes(2, 0xA9, 0xAA); // LDA #$AA
    execute_next_instruction();
    
    place_n_bytes(2, 0xA2, 0x01); // LDX #$01
    execute_next_instruction();
    
    place_n_bytes(2, 0x41, 0x5F); // EOR ($5F,X) -> ($60)
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0xA5, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 1);
    TEST_ASSERT_EQUAL_HEX(0x8006, getPC());
}

void test_cmp_indirect_x(void) {
    writeByte(0x70, 0x50); // Low pointer
    writeByte(0x71, 0x95); // High pointer
    writeByte(0x9550, 0x50); // Value to compare
    
    place_n_bytes(2, 0xA9, 0x50); // LDA #$50
    execute_next_instruction();
    
    place_n_bytes(2, 0xA2, 0x03); // LDX #$03
    execute_next_instruction();
    
    place_n_bytes(2, 0xC1, 0x6D); // CMP ($6D,X) -> ($70)
    execute_next_instruction();
    
    verify_flags(1, 1, 0, 0, 0, 0);
    TEST_ASSERT_EQUAL_HEX(0x8006, getPC());
}

// ============================================================================
// INDIRECT,Y ADDRESSING TESTS (Zero Page Indirect Indexed)
// ============================================================================

void test_lda_indirect_y(void) {
    writeByte(0x40, 0x00); // Low pointer
    writeByte(0x41, 0x97); // High pointer
    writeByte(0x9710, 0x55); // Value with Y offset
    
    place_n_bytes(2, 0xA0, 0x10); // LDY #$10
    execute_next_instruction();
    
    place_n_bytes(2, 0xB1, 0x40); // LDA ($40),Y
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x55, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 0);
    TEST_ASSERT_EQUAL_HEX(0x8004, getPC());
}

void test_lda_indirect_y_cross_page(void) {
    // Test page boundary crossing
    writeByte(0x42, 0xF0); // Low pointer
    writeByte(0x43, 0x97); // High pointer
    writeByte(0x9805, 0xAA); // Value after crossing
    
    place_n_bytes(2, 0xA0, 0x15); // LDY #$15
    execute_next_instruction();
    
    place_n_bytes(2, 0xB1, 0x42); // LDA ($42),Y crosses from 97F0+15 = 9805
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0xAA, readByte(getCPU_Accumulator()));
    TEST_ASSERT_EQUAL_HEX(0x8004, getPC());
}

void test_sta_indirect_y(void) {
    place_n_bytes(2, 0xA9, 0x77); // LDA #$77
    execute_next_instruction();
    
    place_n_bytes(2, 0xA0, 0x08); // LDY #$08
    execute_next_instruction();
    
    writeByte(0x44, 0x80); // Low pointer
    writeByte(0x45, 0x98); // High pointer
    
    place_n_bytes(2, 0x91, 0x44); // STA ($44),Y
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x77, readByte(0x9888));
    TEST_ASSERT_EQUAL_HEX(0x8006, getPC());
}

void test_adc_indirect_y(void) {
    writeByte(0x46, 0x00); // Low pointer
    writeByte(0x47, 0x99); // High pointer
    writeByte(0x9920, 0x15); // Value to add
    
    place_n_bytes(2, 0xA9, 0x10); // LDA #$10
    execute_next_instruction();
    
    place_n_bytes(2, 0xA0, 0x20); // LDY #$20
    execute_next_instruction();
    
    place_n_bytes(2, 0x71, 0x46); // ADC ($46),Y
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x25, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 0);
    TEST_ASSERT_EQUAL_HEX(0x8006, getPC());
}

void test_sbc_indirect_y(void) {
    writeByte(0x48, 0x30); // Low pointer
    writeByte(0x49, 0x99); // High pointer
    writeByte(0x9960, 0x10); // Value to subtract
    
    place_n_bytes(2, 0xA9, 0x50); // LDA #$50
    execute_next_instruction();
    
    place_n_bytes(1, 0x38); // SEC
    execute_next_instruction();
    
    place_n_bytes(2, 0xA0, 0x30); // LDY #$30
    execute_next_instruction();
    
    place_n_bytes(2, 0xF1, 0x48); // SBC ($48),Y
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x40, readByte(getCPU_Accumulator()));
    verify_flags(1, 0, 0, 0, 0, 0);
    TEST_ASSERT_EQUAL_HEX(0x8007, getPC());
}

void test_and_indirect_y(void) {
    writeByte(0x4A, 0x40); // Low pointer
    writeByte(0x4B, 0x99); // High pointer
    writeByte(0x9965, 0x0F); // Value
    
    place_n_bytes(2, 0xA9, 0xFF); // LDA #$FF
    execute_next_instruction();
    
    place_n_bytes(2, 0xA0, 0x25); // LDY #$25
    execute_next_instruction();
    
    place_n_bytes(2, 0x31, 0x4A); // AND ($4A),Y
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x0F, readByte(getCPU_Accumulator()));
    TEST_ASSERT_EQUAL_HEX(0x8006, getPC());
}

void test_ora_indirect_y(void) {
    writeByte(0x4C, 0x50); // Low pointer
    writeByte(0x4D, 0x99); // High pointer
    writeByte(0x99A0, 0xF0); // Value
    
    place_n_bytes(2, 0xA9, 0x0F); // LDA #$0F
    execute_next_instruction();
    
    place_n_bytes(2, 0xA0, 0x50); // LDY #$50
    execute_next_instruction();
    
    place_n_bytes(2, 0x11, 0x4C); // ORA ($4C),Y
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0xFF, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 1);
    TEST_ASSERT_EQUAL_HEX(0x8006, getPC());
}

void test_eor_indirect_y(void) {
    writeByte(0x4E, 0x60); // Low pointer
    writeByte(0x4F, 0x99); // High pointer
    writeByte(0x99C0, 0x0F); // Value
    
    place_n_bytes(2, 0xA9, 0xAA); // LDA #$AA
    execute_next_instruction();
    
    place_n_bytes(2, 0xA0, 0x60); // LDY #$60
    execute_next_instruction();
    
    place_n_bytes(2, 0x51, 0x4E); // EOR ($4E),Y
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0xA5, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 1);
    TEST_ASSERT_EQUAL_HEX(0x8006, getPC());
}

void test_cmp_indirect_y(void) {
    writeByte(0x50, 0x70); // Low pointer
    writeByte(0x51, 0x99); // High pointer
    writeByte(0x99D8, 0x50); // Value
    
    place_n_bytes(2, 0xA9, 0x50); // LDA #$50
    execute_next_instruction();
    
    place_n_bytes(2, 0xA0, 0x68); // LDY #$68
    execute_next_instruction();
    
    place_n_bytes(2, 0xD1, 0x50); // CMP ($50),Y
    execute_next_instruction();
    
    verify_flags(1, 1, 0, 0, 0, 0);
    TEST_ASSERT_EQUAL_HEX(0x8006, getPC());
}

// ============================================================================
// ABSOLUTE INDIRECT ADDRESSING TESTS (JMP ONLY)
// ============================================================================

void test_jmp_indirect(void) {
    // Setup pointer at $1234
    writeByte(0x1234, 0x78); // Low byte
    writeByte(0x1235, 0x56); // High byte
    
    place_n_bytes(3, 0x6C, 0x34, 0x12); // JMP ($1234)
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x5678, getPC());
}

void test_jmp_indirect_page_boundary_bug(void) {
    writeByte(0x00FF, 0x34); // Low byte read from $00FF
    writeByte(0x0000, 0x12); // High byte read from $0000 (BUG - should be $0100)
    
    writeByte(0x0100, 0xFF); // This should NOT be read
    
    // Execute JMP at page boundary
    place_n_bytes(3, 0x6C, 0xFF, 0x00); // JMP ($00FF)
    execute_next_instruction();
    
    // Should jump to $1234 (from $00FF/$0000), not $FF34 (from $00FF/$0100)
    TEST_ASSERT_EQUAL_HEX(0x1234, getPC());
}

// ============================================================================
// IMPLIED ADDRESSING TESTS - MISCELLANEOUS
// ============================================================================

void test_brk_behavior(void) {
    unsigned char initial_sp = readByte(getCPU_Stack());
    
    place_n_bytes(1, 0x00); // BRK
    place_n_bytes(1, 0xEA); // Padding byte
    execute_next_instruction();
    
    // Check stack for return address and status
    // Should push PC+2 (address of next instruction after padding)
    // and status with B flag set
    TEST_ASSERT_EQUAL_HEX(initial_sp - 3, readByte(getCPU_Stack()));
}


// ============================================================================
// PAGE CROSSING TESTS
// ============================================================================

void test_lda_absolute_x_page_cross(void) {
    writeByte(0x1300, 0x42);
    place_n_bytes(2, 0xA2, 0x01); // LDX #$01
    execute_next_instruction();
    
    // LDA $12FF,X should cross page to $1300 (extra cycle on real hardware)
    place_n_bytes(3, 0xBD, 0xFF, 0x12);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x42, readByte(getCPU_Accumulator()));
    TEST_ASSERT_EQUAL_HEX(0x8005, getPC());
}

void test_branch_relative_page_cross(void) {
    // Force PC to $80FE, branch forward to cross page boundary
    setPC(0x80FE);
    memPtr = 0x80FE - 0x6000;
    place_n_bytes(2, 0x90, 0x03); // BCC +3 (taken, crosses page)
    execute_next_instruction();
    
    // Should be 0x80FE + 2 + 3 = 0x8103 (page crossed from 80 to 81)
    TEST_ASSERT_EQUAL_HEX(0x8103, getPC());
}

// ============================================================================
// STACK EDGE CASE TESTS
// ============================================================================

void test_stack_wraparound_pha_pla(void) {
    // Set stack pointer to $00
    writeByte(getCPU_Stack(), 0x00);
    
    place_n_bytes(2, 0xA9, 0x42); // LDA #$42
    execute_next_instruction();
    
    place_n_bytes(1, 0x48); // PHA (should wrap to $0100 + $00)
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0xFF, readByte(getCPU_Stack())); // SP should wrap to $FF
    
    place_n_bytes(1, 0x68); // PLA (should unwrap from $FF)
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x00, readByte(getCPU_Stack())); // SP should be back to $00
    TEST_ASSERT_EQUAL_HEX(0x42, readByte(getCPU_Accumulator()));
    TEST_ASSERT_EQUAL_HEX(0x8004, getPC());
}

// ============================================================================
// FLAG BEHAVIOR TESTS
// ============================================================================

void test_tsx_does_affect_flags(void) {
    // Setup: Set stack pointer to known value ($80 = negative)
    writeByte(getCPU_Stack(), 0x80);
    
    place_n_bytes(1, 0xBA); // TSX
    execute_next_instruction();
    
    // Verify transfer and flag effects
    TEST_ASSERT_EQUAL_HEX(0x80, readByte(getCPU_XRegister()));
    verify_flags(0, 0, 0, 0, 0, 1); // N=1, Z=0
    TEST_ASSERT_EQUAL_HEX(0x8001, getPC());
}

// ============================================================================
// SBC BORROW BEHAVIOR TESTS
// ============================================================================

void test_sbc_borrow_with_carry_clear(void) {
    setCPUStatusFlag(CARRY, false);
    
    place_n_bytes(2, 0xA9, 0x50); // LDA #$50
    execute_next_instruction();
    
    place_n_bytes(2, 0xE9, 0x10); // SBC #$10 (carries borrow of 1)
    execute_next_instruction();
    
    // Result = $50 - $10 - 1 = $3F
    TEST_ASSERT_EQUAL_HEX(0x3F, readByte(getCPU_Accumulator()));
    verify_flags(1, 0, 0, 0, 0, 0); // Carry set (no borrow)
    TEST_ASSERT_EQUAL_HEX(0x8004, getPC());
}

void test_sbc_borrow_chain(void) {
    // Test multi-byte subtraction: $0100 - $0001
    setCPUStatusFlag(CARRY, true);
    
    place_n_bytes(2, 0xA9, 0x00); // LDA #$00 (low byte)
    execute_next_instruction();
    
    place_n_bytes(1, 0x38); // SEC (ensure carry for first byte)
    execute_next_instruction();
    
    place_n_bytes(2, 0xE9, 0x01); // SBC #$01 (low byte)
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0xFF, readByte(getCPU_Accumulator())); // Low result
    TEST_ASSERT_EQUAL(0, getCPUStatusFlag(CARRY)); // Borrow occurred
    TEST_ASSERT_EQUAL_HEX(0x8005, getPC());
    
    // High byte would be next: LDA #$01, SBC #$00
    // In real code you'd store low byte and repeat for high byte
}

void test_sbc_zero_minus_zero_carry_clear(void) {
    setCPUStatusFlag(CARRY, false);
    
    place_n_bytes(2, 0xA9, 0x00); // LDA #$00
    execute_next_instruction();
    
    place_n_bytes(2, 0xE9, 0x00); // SBC #$00
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0xFF, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 1); // No carry, negative result
    TEST_ASSERT_EQUAL_HEX(0x8004, getPC());
}

// ============================================================================
// BIT INSTRUCTION DETAIL TESTS
// ============================================================================

void test_bit_overflow_negative_flags(void) {
    writeByte(0x100, 0xC0); // Memory: bit 7 and 6 set
    
    place_n_bytes(2, 0xA9, 0x3F); // LDA #$3F
    execute_next_instruction();
    
    place_n_bytes(3, 0x2C, 0x00, 0x01); // BIT $0100
    execute_next_instruction();
    
    // Accumulator unchanged
    TEST_ASSERT_EQUAL_HEX(0x3F, readByte(getCPU_Accumulator()));
    // Zero flag set because A & M = 0
    TEST_ASSERT_EQUAL(1, getCPUStatusFlag(ZERO));
    // Overflow flag = bit 6 of memory
    TEST_ASSERT_EQUAL(1, getCPUStatusFlag(CPU_OVERFLOW));
    // Negative flag = bit 7 of memory
    TEST_ASSERT_EQUAL(1, getCPUStatusFlag(NEGATIVE));
    TEST_ASSERT_EQUAL_HEX(0x8005, getPC());
}

void test_bit_hardware_register_simulation(void) {
    // Simulate reading a hardware register that has side effects
    writeByte(0x200, 0x40); // Only bit 6 set
    
    place_n_bytes(2, 0xA9, 0x00); // LDA #$00
    execute_next_instruction();
    
    // BIT should trigger read (which in real hardware might do something)
    // but we just verify flags are set from memory, not accumulator
    place_n_bytes(3, 0x2C, 0x00, 0x02); // BIT $0200
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL(1, getCPUStatusFlag(CPU_OVERFLOW));
    TEST_ASSERT_EQUAL(0, getCPUStatusFlag(NEGATIVE));
    TEST_ASSERT_EQUAL(1, getCPUStatusFlag(ZERO));
    TEST_ASSERT_EQUAL_HEX(0x8005, getPC());
}

// ============================================================================
// BRK/RTI DETAIL TESTS
// ============================================================================

void test_brk_skips_padding_byte(void) {   
    place_n_bytes(1, 0x00); // BRK at 0x8000
    place_n_bytes(1, 0xFF); // Padding at 0x8001
    place_n_bytes(1, 0xEA); // NOP at 0x8002
    
    execute_next_instruction(); // Execute BRK
    
    unsigned char status = popFromStack();
    unsigned char pc_low = popFromStack();
    unsigned char pc_high = popFromStack();
    
    TEST_ASSERT_EQUAL_HEX(0x02, pc_low); // Low byte of 0x8002
    TEST_ASSERT_EQUAL_HEX(0x80, pc_high); // High byte of 0x8002
}

void test_rti_vs_rts_difference(void) {
    // Setup stack for RTI
    pushToStack(0x90); // PC high
    pushToStack(0x00); // PC low
    pushToStack(0x80); // Status (with some flags)
    
    place_n_bytes(1, 0x40); // RTI
    execute_next_instruction();
    
    // Should return to exactly $9000
    TEST_ASSERT_EQUAL_HEX(0x80, readByte(getCPU_StatusRegister()));
    TEST_ASSERT_EQUAL_HEX(0x9000, getPC());
}

// ============================================================================
// ZERO PAGE WRAPAROUND TESTS
// ============================================================================

void test_zero_page_indirect_x_wrap(void) {
    // ($FF + X) must wrap within zero page
    writeByte(0xFF, 0xFF); 
    writeByte(0x00, 0x12); // At wrap location
    writeByte(0x12FF, 0xAB);
    
    place_n_bytes(2, 0xA2, 0x01); // LDX #$01
    execute_next_instruction();
    
    place_n_bytes(2, 0xA1, 0xFE); // LDA ($FE,X) -> reads from $FF, $00 (wraps)
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0xAB, readByte(getCPU_Accumulator()));
    TEST_ASSERT_EQUAL_HEX(0x8004, getPC());
}

void test_zero_page_y_wrap_ldx(void) {
    writeByte(0x80, 0x55);
    
    place_n_bytes(2, 0xA0, 0x80); // LDY #$80
    execute_next_instruction();
    
    // LDX $7F,Y should wrap to $7F + $80 = $FF within zero page
    writeByte(0xFF, 0x42);
    place_n_bytes(2, 0xB6, 0x7F); // LDX $7F,Y
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x42, readByte(getCPU_XRegister()));
    TEST_ASSERT_EQUAL_HEX(0x8004, getPC());
}


void test_tsx_txs_stack_pointer_actual_value(void) {
    place_n_bytes(2, 0xA2, 0x55); // LDX #$55
    execute_next_instruction();
    
    place_n_bytes(1, 0x9A); // TXS
    execute_next_instruction();
    
    // Now do some stack operation
    place_n_bytes(2, 0xA9, 0x42); // LDA #$42
    execute_next_instruction();
    
    place_n_bytes(1, 0x48); // PHA
    execute_next_instruction();
    
    // Check that value was pushed to $0155 (not $0154 or $0156)
    TEST_ASSERT_EQUAL_HEX(0x42, readByte(0x0155));
    TEST_ASSERT_EQUAL_HEX(0x54, readByte(getCPU_Stack()));
    TEST_ASSERT_EQUAL_HEX(0x8006, getPC());
}

void test_php_always_sets_bits_4_and_5(void) {
    // Clear all flags
    place_n_bytes(1, 0x18); // CLC
    execute_next_instruction();
    place_n_bytes(1, 0xD8); // CLD
    execute_next_instruction();
    place_n_bytes(1, 0x58); // CLI
    execute_next_instruction();
    place_n_bytes(1, 0xB8); // CLV
    execute_next_instruction();
    
    place_n_bytes(1, 0x08); // PHP
    execute_next_instruction();
    
    unsigned char status = popFromStack();
    // Bits 4 and 5 should be set (0x30)
    TEST_ASSERT_EQUAL_HEX(0x30, status & 0x30);
    TEST_ASSERT_EQUAL_HEX(0x8005, getPC());
}

void test_plp_ignores_bits_4_and_5(void) {
    pushToStack(0xFF); // All bits set
    
    place_n_bytes(1, 0x28); // PLP
    execute_next_instruction();
    
    // Read status - bits 4 and 5 shouldn't be settable via PLP
    unsigned char status = readByte(getCPU_StatusRegister());
    // Bit 5 should always read as 1, bit 4 (B) is not a real flag
    TEST_ASSERT_EQUAL_HEX(0x20, status & 0x30); // Only bit 5 set
    TEST_ASSERT_EQUAL_HEX(0x8001, getPC());
}


// ============================================================================
// ADDRESSING MODE TESTS
// ============================================================================

void test_immediate_addressing(void) {
    place_n_bytes(1, 0x42); // Operand byte
    
    int address = IMM(getPC());
    
    TEST_ASSERT_EQUAL_HEX(getPC(), address);
}

void test_absolute_addressing(void) {
    place_n_bytes(2, 0x34, 0x12); // Low then high byte
    
    int address = ABS_A(getPC());
    
    TEST_ASSERT_EQUAL_HEX(0x1234, address);
}

void test_absolute_indexed_x(void) {
    place_n_bytes(2, 0x00, 0x10); // Base address $1000
    
    writeByte(getCPU_XRegister(), 0x50); // X = $50
    
    int address = ABS_INDEX_X(getPC());
    
    TEST_ASSERT_EQUAL_HEX(0x1050, address);
}

void test_absolute_indexed_y(void) {
    place_n_bytes(2, 0x00, 0x20); // Base address $2000
    
    writeByte(getCPU_YRegister(), 0x30); // Y = $30
    
    int address = ABS_INDEX_Y(getPC());
    
    TEST_ASSERT_EQUAL_HEX(0x2030, address);
}

void test_accumulator_addressing(void) {
    writeByte(getCPU_Accumulator(), 0x42); // Set accumulator value
    
    int address = ACC(getPC());
    
    TEST_ASSERT_EQUAL_HEX(getCPU_Accumulator(), address);
}

void test_implied_addressing(void) {
    int address = IMP(getPC());
    
    TEST_ASSERT_EQUAL_HEX(-1, address); // IMP should return -1
}

void test_zero_page_addressing(void) {
    place_n_bytes(1, 0x80); // Zero page address
    
    int address = ZP(getPC());
    
    TEST_ASSERT_EQUAL_HEX(0x80, address);
}

void test_zero_page_indexed_x(void) {
    place_n_bytes(1, 0x40); // Base zero page address
    
    writeByte(getCPU_XRegister(), 0x10); // X = $10
    
    int address = ZP_INDX_X(getPC());
    
    TEST_ASSERT_EQUAL_HEX(0x50, address); // $40 + $10 = $50
}

void test_zero_page_indexed_x_wrap(void) {
    place_n_bytes(1, 0xF0); // Base zero page address
    
    writeByte(getCPU_XRegister(), 0x20); // X = $20
    
    int address = ZP_INDX_X(getPC());
    
    TEST_ASSERT_EQUAL_HEX(0x10, address); // $F0 + $20 = $110, wraps to $10
}

void test_zero_page_indexed_y(void) {
    place_n_bytes(1, 0x60); // Base zero page address
    
    writeByte(getCPU_YRegister(), 0x05); // Y = $05
    
    int address = ZP_INDX_Y(getPC());
    
    TEST_ASSERT_EQUAL_HEX(0x65, address); // $60 + $05 = $65
}

void test_zero_page_indirect(void) {
    place_n_bytes(1, 0x30); // Zero page pointer address
    
    // Set up indirect pointer at $30
    writeByte(0x30, 0x40); // Low byte
    writeByte(0x31, 0x12); // High byte
    
    int address = ZP_IND(getPC());
    
    TEST_ASSERT_EQUAL_HEX(0x1240, address);
}

void test_zero_page_indexed_indirect(void) {
    place_n_bytes(1, 0x20); // Base zero page address
    
    writeByte(getCPU_XRegister(), 0x05); // X = $05
    
    // Set up indirect pointer at $25 ($20 + $05)
    writeByte(0x25, 0x60); // Low byte
    writeByte(0x26, 0x34); // High byte
    
    int address = ZP_INDX_IND(getPC());
    
    TEST_ASSERT_EQUAL_HEX(0x3460, address);
}

void test_zero_page_indexed_indirect_wrap(void) {
    place_n_bytes(1, 0xFE); // Base zero page address
    
    writeByte(getCPU_XRegister(), 0x03); // X = $03
    
    // Set up indirect pointer with wrap: $FE + $03 = $01 (wraps)
    writeByte(0x01, 0x80); // Low byte
    writeByte(0x02, 0x56); // High byte
    
    int address = ZP_INDX_IND(getPC());
    
    TEST_ASSERT_EQUAL_HEX(0x5680, address);
}

void test_zero_page_indirect_indexed_y(void) {
    place_n_bytes(1, 0x40); // Zero page pointer address
    
    writeByte(getCPU_YRegister(), 0x10); // Y = $10
    
    // Set up indirect pointer at $40
    writeByte(0x40, 0x30); // Low byte
    writeByte(0x41, 0x12); // High byte
    
    int address = ZP_IND_INDX_Y(getPC());
    
    TEST_ASSERT_EQUAL_HEX(0x1240, address); // $1230 + $10 = $1240
}

void test_zero_page_indirect_indexed_y_cross_page(void) {
    place_n_bytes(1, 0x50); // Zero page pointer address
    
    writeByte(getCPU_YRegister(), 0xF0); // Y = $F0
    
    // Set up indirect pointer at $50
    writeByte(0x50, 0x20); // Low byte
    writeByte(0x51, 0x12); // High byte
    
    int address = ZP_IND_INDX_Y(getPC());
    
    TEST_ASSERT_EQUAL_HEX(0x1310, address); // $1220 + $F0 = $1310 (crosses page)
}

void test_implied_returns_invalid(void) {
    int address = IMP(getPC());
    
    TEST_ASSERT_EQUAL_HEX(-1, address); // IMP should always return -1
}

void test_accumulator_returns_register_address(void) {
    // Set accumulator to a known value
    writeByte(getCPU_Accumulator(), 0x77);
    
    int address = ACC(getPC());
    
    // Should return the address of the accumulator register
    TEST_ASSERT_EQUAL_HEX(getCPU_Accumulator(), address);
}

void test_zero_page_indirect_wrap_behavior(void) {
    place_n_bytes(1, 0xFF); // Zero page pointer at boundary
    
    // Set up indirect pointer with wrap: $FF (low) and $00 (high)
    writeByte(0xFF, 0xCD); // Low byte
    writeByte(0x00, 0xAB); // High byte (wraps from $FF to $00)
    
    int address = ZP_IND(getPC());
    
    TEST_ASSERT_EQUAL_HEX(0xABCD, address);
}

void test_indexed_indirect_no_wrap_calculation(void) {
    place_n_bytes(1, 0x10); // Base address
    
    writeByte(getCPU_XRegister(), 0x05); // X = $05
    
    // Pointer at $15 ($10 + $05)
    writeByte(0x15, 0x34); // Low byte
    writeByte(0x16, 0x12); // High byte
    
    int address = ZP_INDX_IND(getPC());
    
    TEST_ASSERT_EQUAL_HEX(0x1234, address);
}

void test_indirect_indexed_no_offset(void) {
    place_n_bytes(1, 0x20); // Pointer address
    
    writeByte(getCPU_YRegister(), 0x00); // Y = $00 (no offset)
    
    // Pointer at $20
    writeByte(0x20, 0x56); // Low byte
    writeByte(0x21, 0x34); // High byte
    
    int address = ZP_IND_INDX_Y(getPC());
    
    TEST_ASSERT_EQUAL_HEX(0x3456, address); // Base address with no Y offset
}

void test_indirect_indexed_max_offset(void) {
    place_n_bytes(1, 0x30); // Pointer address
    
    writeByte(getCPU_YRegister(), 0xFF); // Y = $FF (max offset)
    
    // Pointer at $30
    writeByte(0x30, 0x01); // Low byte
    writeByte(0x31, 0x12); // High byte
    
    int address = ZP_IND_INDX_Y(getPC());
    
    TEST_ASSERT_EQUAL_HEX(0x1300, address); // $1201 + $FF = $1300
}

void test_absolute_indexed_cross_page_detection(void) {
    place_n_bytes(2, 0xFF, 0x12); // Base address $12FF
    
    writeByte(getCPU_XRegister(), 0x01); // X = $01
    
    int address = ABS_INDEX_X(getPC());
    
    // Should correctly calculate $12FF + $01 = $1300 (crosses page)
    TEST_ASSERT_EQUAL_HEX(0x1300, address);
}

void test_zero_page_boundary_conditions(void) {
    // Test all boundary conditions for zero page addressing
    
    // Minimum zero page address
    memPtr = 0x8000 - 0x6000; // Reset to default
    place_n_bytes(1, 0x00);
    TEST_ASSERT_EQUAL_HEX(0x00, ZP(getPC()));
    
    // Maximum zero page address  
    memPtr = 0x8000 - 0x6000; // Reset to default
    place_n_bytes(1, 0xFF);
    TEST_ASSERT_EQUAL_HEX(0xFF, ZP(getPC()));
    
    // Middle zero page address
    memPtr = 0x8000 - 0x6000; // Reset to default
    place_n_bytes(1, 0x80);
    TEST_ASSERT_EQUAL_HEX(0x80, ZP(getPC()));
}

void test_immediate_ignores_memory(void) {
    // IMM should return PC regardless of what's in memory
    int address = IMM(getPC());
    
    TEST_ASSERT_EQUAL_HEX(getPC(), address);
}


// ============================================================================
// ADDITIONAL EDGE CASE TESTS
// ============================================================================

void test_absolute_indexed_wrap_around(void) {
    // Test indexing that wraps around 64K boundary
    place_n_bytes(2, 0xFF, 0xFF); // Base address $FFFF
    
    writeByte(getCPU_XRegister(), 0x01); // X = $01
    
    int address = ABS_INDEX_X(getPC());
    
    // Should wrap from $FFFF + 1 = $0000
    TEST_ASSERT_EQUAL_HEX(0x0000, address);
}

void test_zero_page_indirect_invalid_pointer(void) {
    // Test with pointer that points to invalid memory (edge of memory map)
    place_n_bytes(1, 0xFD); // Zero page pointer
    
    // Set up pointer to $FFFE (last two bytes of 64K space)
    writeByte(0xFD, 0xFE); // Low byte
    writeByte(0xFE, 0xFF); // High byte
    
    int address = ZP_IND(getPC());
    
    TEST_ASSERT_EQUAL_HEX(0xFFFE, address);
}

void test_indexed_indirect_zero_x_register(void) {
    // Test with X = 0 (no effective indexing)
    place_n_bytes(1, 0x40); // Base address
    
    writeByte(getCPU_XRegister(), 0x00); // X = $00
    
    // Pointer at $40 ($40 + $00)
    writeByte(0x40, 0x34); // Low byte
    writeByte(0x41, 0x12); // High byte
    
    int address = ZP_INDX_IND(getPC());
    
    TEST_ASSERT_EQUAL_HEX(0x1234, address);
}

void test_indirect_indexed_zero_y_register(void) {
    // Test with Y = 0 (no effective indexing)
    place_n_bytes(1, 0x50); // Pointer address
    
    writeByte(getCPU_YRegister(), 0x00); // Y = $00
    
    // Pointer at $50
    writeByte(0x50, 0x67); // Low byte
    writeByte(0x51, 0x89); // High byte
    
    int address = ZP_IND_INDX_Y(getPC());
    
    TEST_ASSERT_EQUAL_HEX(0x8967, address);
}

void test_stack_addressing_edge_cases(void) {
    // Test stack with minimum value
    writeByte(getCPU_Stack(), 0x00); // Stack pointer = $00
    
    int address = STK(getPC());
    
    TEST_ASSERT_EQUAL_HEX(0x0100, address);
    
    // Test stack with maximum value
    writeByte(getCPU_Stack(), 0xFF); // Stack pointer = $FF
    
    address = STK(getPC());
    
    TEST_ASSERT_EQUAL_HEX(0x01FF, address);
}


void test_accumulator_register_isolation(void) {
    // Verify accumulator addressing doesn't depend on PC
    writeByte(getCPU_Accumulator(), 0x99);
    
    // Should return same address regardless of PC value
    TEST_ASSERT_EQUAL_HEX(getCPU_Accumulator(), ACC(0x0000));
    TEST_ASSERT_EQUAL_HEX(getCPU_Accumulator(), ACC(0x1234));
    TEST_ASSERT_EQUAL_HEX(getCPU_Accumulator(), ACC(0xFFFF));
}

void test_implied_consistency(void) {
    // IMP should always return -1 regardless of input
    TEST_ASSERT_EQUAL_HEX(-1, IMP(0x0000));
    TEST_ASSERT_EQUAL_HEX(-1, IMP(0x1234));
    TEST_ASSERT_EQUAL_HEX(-1, IMP(0xFFFF));
    TEST_ASSERT_EQUAL_HEX(-1, IMP(-1));
}



void test_brk_pushes_status_with_bits_4_and_5_set(void) {
    unsigned short pc_before;
    unsigned char sp_before;

    /*
        ============================
        Case 1: P = $00 → expect $30
        ============================
    */

    place_n_bytes(2, 0xA9, 0x00); // LDA #$00
    place_n_bytes(1, 0x48);       // PHA
    place_n_bytes(1, 0x28);       // PLP
    place_n_bytes(1, 0x00);       // BRK
    place_n_bytes(1, 0xEA);       // NOP (padding)

    execute_next_instruction(); // LDA
    execute_next_instruction(); // PHA
    execute_next_instruction(); // PLP

    verify_all_flags_clear();

    pc_before = getPC();
    sp_before = readByte(getCPU_Stack());

    execute_next_instruction(); // BRK
    execute_next_instruction(); // NOP

    // Status pushed by BRK must be visible in A via IRQ handler
    TEST_ASSERT_EQUAL_HEX(0x30, readByte(getCPU_Accumulator()));

    // BRK must advance PC by 2
    TEST_ASSERT_EQUAL_HEX(pc_before + 2, getPC());

    // Interrupt flag must be set during BRK handling
    TEST_ASSERT_EQUAL(1, getCPUStatusFlag(INTERRUPT));

    // Stack must be balanced after RTI
    TEST_ASSERT_EQUAL_HEX(sp_before, readByte(getCPU_Stack()));

    /*
        ============================
        Case 2: P = $FF → expect $FF
        ============================
    */

    place_n_bytes(2, 0xA9, 0xFF); // LDA #$FF
    place_n_bytes(1, 0x48);       // PHA
    place_n_bytes(1, 0x28);       // PLP
    place_n_bytes(1, 0x00);       // BRK
    place_n_bytes(1, 0xEA);       // NOP

    execute_next_instruction(); // LDA
    execute_next_instruction(); // PHA
    execute_next_instruction(); // PLP

    TEST_ASSERT_EQUAL_HEX(0xFF, getCPU_StatusRegister());

    pc_before = getPC();
    sp_before = readByte(getCPU_Stack());

    execute_next_instruction(); // BRK
    execute_next_instruction(); // NOP

    // Full status must be preserved
    TEST_ASSERT_EQUAL_HEX(0xFF, readByte(getCPU_Accumulator()));

    // PC behavior must still be correct
    TEST_ASSERT_EQUAL_HEX(pc_before + 2, getPC());

    // Stack pointer must again be restored
    TEST_ASSERT_EQUAL_HEX(sp_before, readByte(getCPU_Stack()));
}



// ============================================================================
// MAIN TEST RUNNER
// ============================================================================

void test_abs_x_page_cross(void)
{
    unsigned short base = 0x10F0;
    unsigned char X = 0x20;
    unsigned char value = 0x42;

    writeByte(base + X, value);

    place_n_bytes(2, 0xA2, X);                 // LDX #X
    execute_next_instruction();

    place_n_bytes(3, 0xBD, base & 0xFF, base >> 8); // LDA base,X
    execute_next_instruction();

    TEST_ASSERT_EQUAL_HEX(value, readByte(getCPU_Accumulator()));
}

/* ------------------------------------------------------------
   ABS,Y — page crossing
------------------------------------------------------------ */
void test_abs_y_page_cross(void)
{
    unsigned short base = 0x10F0;
    unsigned char Y = 0x15;
    unsigned char value = 0x99;

    writeByte(base + Y, value);

    place_n_bytes(2, 0xA0, Y);                 // LDY #Y
    execute_next_instruction();

    place_n_bytes(3, 0xB9, base & 0xFF, base >> 8); // LDA base,Y
    execute_next_instruction();

    TEST_ASSERT_EQUAL_HEX(value, readByte(getCPU_Accumulator()));
}

/* ------------------------------------------------------------
   ABS,X — no page crossing
------------------------------------------------------------ */
void test_abs_x_no_page_cross(void)
{
    unsigned short base = 0x1000;
    unsigned char X = 0x0F;
    unsigned char value = 0xAA;

    writeByte(base + X, value);

    place_n_bytes(2, 0xA2, X);                 // LDX #X
    execute_next_instruction();

    place_n_bytes(3, 0xBD, base & 0xFF, base >> 8); // LDA base,X
    execute_next_instruction();

    TEST_ASSERT_EQUAL_HEX(value, readByte(getCPU_Accumulator()));
}

/* ------------------------------------------------------------
   ABS,X — exhaustive low-byte carry check
------------------------------------------------------------ */
void test_abs_x_low_byte_carry_exhaustive(void)
{
    unsigned short base = 0x10F0;

    for (int x = 0x00; x <= 0x1F; x++) {
        unsigned char value = (unsigned char)(x ^ 0xA5);

        writeByte(base + x, value);

        place_n_bytes(2, 0xA2, x);             // LDX #x
        execute_next_instruction();

        place_n_bytes(3, 0xBD, base & 0xFF, base >> 8); // LDA base,X
        execute_next_instruction();

        TEST_ASSERT_EQUAL_HEX(value, readByte(getCPU_Accumulator()));

        tearDown();
        setUp();
    }
}

/* ------------------------------------------------------------
   ABS,Y — exhaustive low-byte carry check
------------------------------------------------------------ */
void test_abs_y_low_byte_carry_exhaustive(void)
{
    unsigned short base = 0x30F0;

    for (int y = 0x00; y <= 0x1F; y++) {
        unsigned char value = (unsigned char)(y + 0x33);

        writeByte(base + y, value);

        place_n_bytes(2, 0xA0, y);             // LDY #y
        execute_next_instruction();

        place_n_bytes(3, 0xB9, base & 0xFF, base >> 8); // LDA base,Y
        execute_next_instruction();

        TEST_ASSERT_EQUAL_HEX(value, readByte(getCPU_Accumulator()));

        tearDown();
        setUp();
    }
}


int main(void) {
    UNITY_BEGIN();
    
    // Immediate addressing tests
    RUN_TEST(test_abs_x_page_cross);
    RUN_TEST(test_abs_y_page_cross);
    RUN_TEST(test_abs_x_no_page_cross);
    RUN_TEST(test_abs_x_low_byte_carry_exhaustive);
    RUN_TEST(test_abs_y_low_byte_carry_exhaustive);
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
    RUN_TEST(test_multiple_push_pop);

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
    RUN_TEST(test_branch_max_backward);
    RUN_TEST(test_branch_max_forward);

    // Jump and subroutine tests
    RUN_TEST(test_jmp_absolute);
    RUN_TEST(test_jsr_rts);
    RUN_TEST(test_jsr_stack_behavior);
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
    RUN_TEST(test_absolute_indexed_x_all_values);
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

    // ADC and SBC exhaustive: Disabled for speed
    // RUN_TEST(test_adc_exhaustive);
    // RUN_TEST(test_sbc_exhaustive);

    // Indirect,X addressing tests
    RUN_TEST(test_lda_indirect_x);
    RUN_TEST(test_lda_indirect_x_wrap);
    RUN_TEST(test_sta_indirect_x);
    RUN_TEST(test_adc_indirect_x);
    RUN_TEST(test_sbc_indirect_x);
    RUN_TEST(test_and_indirect_x);
    RUN_TEST(test_ora_indirect_x);
    RUN_TEST(test_eor_indirect_x);
    RUN_TEST(test_cmp_indirect_x);
    
    // Indirect,Y addressing tests
    RUN_TEST(test_lda_indirect_y);
    RUN_TEST(test_lda_indirect_y_cross_page);
    RUN_TEST(test_sta_indirect_y);
    RUN_TEST(test_adc_indirect_y);
    RUN_TEST(test_sbc_indirect_y);
    RUN_TEST(test_and_indirect_y);
    RUN_TEST(test_ora_indirect_y);
    RUN_TEST(test_eor_indirect_y);
    RUN_TEST(test_cmp_indirect_y);
    
    // Absolute indirect tests
    RUN_TEST(test_jmp_indirect);
    RUN_TEST(test_jmp_indirect_page_boundary_bug);
    
    // Implied misc tests
    RUN_TEST(test_brk_behavior);

    // Page crossing tests
    RUN_TEST(test_lda_absolute_x_page_cross);
    RUN_TEST(test_branch_relative_page_cross);

    // Stack edge case tests
    RUN_TEST(test_stack_wraparound_pha_pla);

    // Flag behavior tests
    RUN_TEST(test_tsx_does_affect_flags);

    // SBC borrow tests
    RUN_TEST(test_sbc_borrow_with_carry_clear);
    RUN_TEST(test_sbc_borrow_chain);
    RUN_TEST(test_sbc_zero_minus_zero_carry_clear);

    // BIT instruction tests
    RUN_TEST(test_bit_overflow_negative_flags);
    RUN_TEST(test_bit_hardware_register_simulation);

    // BRK/RTI detail tests
    RUN_TEST(test_brk_skips_padding_byte);
    RUN_TEST(test_rti_vs_rts_difference);

    // Zero page wraparound tests
    RUN_TEST(test_zero_page_indirect_x_wrap);
    RUN_TEST(test_zero_page_y_wrap_ldx);
    RUN_TEST(test_tsx_txs_stack_pointer_actual_value);
    RUN_TEST(test_plp_ignores_bits_4_and_5);

        
    // Basic addressing mode tests
    RUN_TEST(test_immediate_addressing);
    RUN_TEST(test_absolute_addressing);
    RUN_TEST(test_absolute_indexed_x);
    RUN_TEST(test_absolute_indexed_y);
    RUN_TEST(test_accumulator_addressing);
    RUN_TEST(test_implied_addressing);
    RUN_TEST(test_zero_page_addressing);
    RUN_TEST(test_zero_page_indexed_x);
    RUN_TEST(test_zero_page_indexed_x_wrap);
    RUN_TEST(test_zero_page_indexed_y);
    RUN_TEST(test_zero_page_indirect);
    RUN_TEST(test_zero_page_indexed_indirect);
    RUN_TEST(test_zero_page_indexed_indirect_wrap);
    RUN_TEST(test_zero_page_indirect_indexed_y);
    RUN_TEST(test_zero_page_indirect_indexed_y_cross_page);
    
    // Edge case and boundary tests
    RUN_TEST(test_implied_returns_invalid);
    RUN_TEST(test_accumulator_returns_register_address);
    RUN_TEST(test_zero_page_indirect_wrap_behavior);
    RUN_TEST(test_indexed_indirect_no_wrap_calculation);
    RUN_TEST(test_indirect_indexed_no_offset);
    RUN_TEST(test_indirect_indexed_max_offset);
    RUN_TEST(test_absolute_indexed_cross_page_detection);
    RUN_TEST(test_zero_page_boundary_conditions);
    RUN_TEST(test_immediate_ignores_memory);
    
    // Additional edge case tests
    RUN_TEST(test_absolute_indexed_wrap_around);
    RUN_TEST(test_zero_page_indirect_invalid_pointer);
    RUN_TEST(test_indexed_indirect_zero_x_register);
    RUN_TEST(test_indirect_indexed_zero_y_register);
    RUN_TEST(test_stack_addressing_edge_cases);
    RUN_TEST(test_accumulator_register_isolation);
    RUN_TEST(test_implied_consistency);
    RUN_TEST(test_brk_pushes_status_with_bits_4_and_5_set);
    RUN_TEST(test_absolute_indexed_x_page_crossing);

    return UNITY_END();
}

// ============================================================================
// MAIN TEST RUNNER FOR ADDRESSING MODES
// ============================================================================