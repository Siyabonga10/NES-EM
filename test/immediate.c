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
    TEST_ASSERT_EQUAL(overflow, getCPUStatusFlag(CPU_OVERFLOW));
    TEST_ASSERT_EQUAL(negative, getCPUStatusFlag(NEGATIVE));
    TEST_ASSERT_EQUAL(carry, getCPUStatusFlag(CARRY));
}

// Helper to check all flags are clear (initial state)
void verify_all_flags_clear(void) {
    verify_flags(0, 0, 0, 0, 0, 0);
}

// Enhanced LDA tests
void test_lda_immediate(void) {
    place_n_bytes(2, 0xA9, 0x42);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x42, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 0); // No flags should be set
}

void test_lda_immediate_zero(void) {
    place_n_bytes(2, 0xA9, 0x00);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x00, readByte(getCPU_Accumulator()));
    verify_flags(0, 1, 0, 0, 0, 0); // Zero flag should be set
}

void test_lda_immediate_negative(void) {
    place_n_bytes(2, 0xA9, 0x80);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x80, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 1); // Negative flag should be set
}

// Additional LDA edge case
void test_lda_immediate_max_positive(void) {
    place_n_bytes(2, 0xA9, 0x7F); // Maximum positive value
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x7F, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 0); // No flags should be set
}

// Enhanced LDX tests
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
 
// Enhanced LDY tests
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

// Enhanced ADC tests
void test_adc_immediate(void) {
    place_n_bytes(2, 0xA9, 0x10);
    execute_next_instruction();
    
    place_n_bytes(2, 0x69, 0x20);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x30, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 0); // No carry, no overflow, not zero, not negative
}

void test_adc_immediate_with_carry(void) {
    setCPUStatusFlag(CARRY, true);
    place_n_bytes(2, 0xA9, 0x10);
    execute_next_instruction();
    
    place_n_bytes(2, 0x69, 0x20);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x31, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 0); // Carry consumed, no flags set
}

void test_adc_immediate_carry_generated(void) {
    place_n_bytes(2, 0xA9, 0xFF);
    execute_next_instruction();
    
    place_n_bytes(2, 0x69, 0x01);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x00, readByte(getCPU_Accumulator()));
    verify_flags(1, 1, 0, 0, 0, 0); // Carry and zero flags set
}

void test_adc_immediate_overflow(void) {
    place_n_bytes(2, 0xA9, 0x7F); // 127 (max positive)
    execute_next_instruction();
    
    place_n_bytes(2, 0x69, 0x01); // Add 1
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x80, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 1, 1); // Overflow and negative flags set
}

// Enhanced SBC tests
void test_sbc_immediate(void) {
    place_n_bytes(2, 0xA9, 0x50);
    execute_next_instruction();
    
    place_n_bytes(2, 0xE9, 0x20);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x2F, readByte(getCPU_Accumulator()));
    verify_flags(1, 0, 0, 0, 0, 0); // Carry set (no borrow)
}

void test_sbc_immediate_borrow(void) {
    place_n_bytes(2, 0xA9, 0x01);
    execute_next_instruction();
    
    place_n_bytes(2, 0xE9, 0x02);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0xFE, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 1); // Carry clear (borrow occurred), negative set
}

void test_sbc_immediate_zero(void) {
    place_n_bytes(2, 0xA9, 0x50);
    execute_next_instruction();
    
    place_n_bytes(2, 0xE9, 0x50);
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0xFF, readByte(getCPU_Accumulator())); // Note: SBC behavior with initial carry
    // This depends on initial carry state - might need adjustment based on your implementation
}

// Enhanced logical operation tests
void test_ora_immediate(void) {
    place_n_bytes(2, 0xA9, 0x0F); // LDA #$0F
    execute_next_instruction();
    
    place_n_bytes(2, 0x09, 0xF0); // ORA #$F0
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0xFF, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 1); // Negative flag set
}

void test_ora_immediate_zero(void) {
    place_n_bytes(2, 0xA9, 0x00); // LDA #$00
    execute_next_instruction();
    
    place_n_bytes(2, 0x09, 0x00); // ORA #$00
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x00, readByte(getCPU_Accumulator()));
    verify_flags(0, 1, 0, 0, 0, 0); // Zero flag set
}

void test_and_immediate(void) {
    place_n_bytes(2, 0xA9, 0xFF); // LDA #$FF
    execute_next_instruction();
    
    place_n_bytes(2, 0x29, 0x0F); // AND #$0F
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x0F, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 0); // No flags set
}

void test_and_immediate_zero(void) {
    place_n_bytes(2, 0xA9, 0xF0); // LDA #$F0
    execute_next_instruction();
    
    place_n_bytes(2, 0x29, 0x0F); // AND #$0F
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x00, readByte(getCPU_Accumulator()));
    verify_flags(0, 1, 0, 0, 0, 0); // Zero flag set
}

void test_eor_immediate(void) {
    place_n_bytes(2, 0xA9, 0xAA); // LDA #$AA
    execute_next_instruction();
    
    place_n_bytes(2, 0x49, 0x0F); // EOR #$0F
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0xA5, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 1); // Negative flag set
}

void test_eor_immediate_zero(void) {
    place_n_bytes(2, 0xA9, 0xFF); // LDA #$FF
    execute_next_instruction();
    
    place_n_bytes(2, 0x49, 0xFF); // EOR #$FF
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x00, readByte(getCPU_Accumulator()));
    verify_flags(0, 1, 0, 0, 0, 0); // Zero flag set
}

// Enhanced comparison tests
void test_cmp_immediate(void) {
    place_n_bytes(2, 0xA9, 0x50); // LDA #$50
    execute_next_instruction();
    
    place_n_bytes(2, 0xC9, 0x50); // CMP #$50
    execute_next_instruction();
    
    verify_flags(1, 1, 0, 0, 0, 0); // Carry and zero set (equal)
}

void test_cmp_immediate_greater(void) {
    place_n_bytes(2, 0xA9, 0x60); // LDA #$60
    execute_next_instruction();
    
    place_n_bytes(2, 0xC9, 0x50); // CMP #$50
    execute_next_instruction();
    
    verify_flags(1, 0, 0, 0, 0, 0); // Carry set (greater)
}

void test_cmp_immediate_less(void) {
    place_n_bytes(2, 0xA9, 0x40); // LDA #$40
    execute_next_instruction();
    
    place_n_bytes(2, 0xC9, 0x50); // CMP #$50
    execute_next_instruction();
    
    verify_flags(0, 0, 0, 0, 0, 1); // Carry clear (less), negative set
}

void test_cmp_immediate_negative_result(void) {
    place_n_bytes(2, 0xA9, 0x80); // LDA #$80 (negative)
    execute_next_instruction();
    
    place_n_bytes(2, 0xC9, 0x7F); // CMP #$7F (positive)
    execute_next_instruction();
    
    // 0x80 - 0x7F = 0x01, but signed: -128 - 127 = -255 (wraps around)
    verify_flags(1, 0, 0, 0, 0, 0); // Implementation specific
}

void test_cpx_immediate(void) {
    place_n_bytes(2, 0xA2, 0x30); // LDX #$30
    execute_next_instruction();
    
    place_n_bytes(2, 0xE0, 0x30); // CPX #$30
    execute_next_instruction();
    
    verify_flags(1, 1, 0, 0, 0, 0); // Equal values
}

void test_cpx_immediate_greater(void) {
    place_n_bytes(2, 0xA2, 0x40); // LDX #$40
    execute_next_instruction();
    
    place_n_bytes(2, 0xE0, 0x30); // CPX #$30
    execute_next_instruction();
    
    verify_flags(1, 0, 0, 0, 0, 0); // X > memory
}

void test_cpy_immediate(void) {
    place_n_bytes(2, 0xA0, 0x25); // LDY #$25
    execute_next_instruction();
    
    place_n_bytes(2, 0xC0, 0x25); // CPY #$25
    execute_next_instruction();
    
    verify_flags(1, 1, 0, 0, 0, 0); // Equal values
}

void test_cpy_immediate_greater(void) {
    place_n_bytes(2, 0xA0, 0x35); // LDY #$35
    execute_next_instruction();
    
    place_n_bytes(2, 0xC0, 0x25); // CPY #$25
    execute_next_instruction();
    
    verify_flags(1, 0, 0, 0, 0, 0); // Y > memory
}

// Enhanced accumulator operations
void test_rol_accumulator(void) {
    place_n_bytes(2, 0xA9, 0x81); // LDA #$81
    execute_next_instruction();
    
    place_n_bytes(1, 0x2A); // ROL A
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x02, readByte(getCPU_Accumulator()));
    verify_flags(1, 0, 0, 0, 0, 0); // Carry set, not zero, not negative
}

void test_rol_accumulator_zero(void) {
    place_n_bytes(2, 0xA9, 0x80); // LDA #$80
    execute_next_instruction();
    
    place_n_bytes(1, 0x2A); // ROL A
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x00, readByte(getCPU_Accumulator()));
    verify_flags(1, 1, 0, 0, 0, 0); // Carry and zero set
}

void test_asl_accumulator(void) {
    place_n_bytes(2, 0xA9, 0x42); // LDA #$42
    execute_next_instruction();
    
    place_n_bytes(1, 0x0A); // ASL A
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x84, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 1); // Negative set
}

void test_ror_accumulator(void) {
    place_n_bytes(2, 0xA9, 0x02); // LDA #$02  
    execute_next_instruction();
    
    place_n_bytes(1, 0x6A); // ROR A
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x01, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 0); // No flags set
}

void test_ror_accumulator_with_carry(void) {
    setCPUStatusFlag(CARRY, true);
    place_n_bytes(2, 0xA9, 0x00); // LDA #$00
    execute_next_instruction();
    
    place_n_bytes(1, 0x6A); // ROR A
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x80, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 1); // Negative set
}

void test_lsr_accumulator(void) {
    place_n_bytes(2, 0xA9, 0x84); // LDA #$84
    execute_next_instruction();
    
    place_n_bytes(1, 0x4A); // LSR A
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x42, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 0); // No flags set
}

void test_lsr_accumulator_carry(void) {
    place_n_bytes(2, 0xA9, 0x85); // LDA #$85 (odd number)
    execute_next_instruction();
    
    place_n_bytes(1, 0x4A); // LSR A
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x42, readByte(getCPU_Accumulator()));
    verify_flags(1, 0, 0, 0, 0, 0); // Carry set
}

// Enhanced transfer operations
void test_txa(void) {
    place_n_bytes(2, 0xA2, 0x55); // LDX #$55
    execute_next_instruction();
    
    place_n_bytes(1, 0x8A); // TXA
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x55, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 0);
}

void test_txa_zero(void) {
    place_n_bytes(2, 0xA2, 0x00); // LDX #$00
    execute_next_instruction();
    
    place_n_bytes(1, 0x8A); // TXA
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x00, readByte(getCPU_Accumulator()));
    verify_flags(0, 1, 0, 0, 0, 0); // Zero flag set
}

void test_tya(void) {
    place_n_bytes(2, 0xA0, 0x33); // LDY #$33
    execute_next_instruction();
    
    place_n_bytes(1, 0x98); // TYA
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x33, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 0);
}

void test_tax(void) {
    place_n_bytes(2, 0xA9, 0x77); // LDA #$77
    execute_next_instruction();
    
    place_n_bytes(1, 0xAA); // TAX
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x77, readByte(getCPU_XRegister()));
    verify_flags(0, 0, 0, 0, 0, 0);
}

void test_tay(void) {
    place_n_bytes(2, 0xA9, 0x99); // LDA #$99
    execute_next_instruction();
    
    place_n_bytes(1, 0xA8); // TAY
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x99, readByte(getCPU_YRegister()));
    verify_flags(0, 0, 0, 0, 0, 1);
}

void test_tay_positive(void) {
    place_n_bytes(2, 0xA9, 0x45); // LDA #$45 (positive value)
    execute_next_instruction();
    
    place_n_bytes(1, 0xA8); // TAY
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x45, readByte(getCPU_YRegister()));
    verify_flags(0, 0, 0, 0, 0, 0); 
}
// Enhanced increment/decrement operations
void test_inx(void) {
    place_n_bytes(2, 0xA2, 0x10); // LDX #$10
    execute_next_instruction();
    
    place_n_bytes(1, 0xE8); // INX
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x11, readByte(getCPU_XRegister()));
    verify_flags(0, 0, 0, 0, 0, 0);
}

void test_inx_overflow(void) {
    place_n_bytes(2, 0xA2, 0xFF); // LDX #$FF
    execute_next_instruction();
    
    place_n_bytes(1, 0xE8); // INX
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x00, readByte(getCPU_XRegister()));
    verify_flags(0, 1, 0, 0, 0, 0); // Zero flag set
}

void test_iny(void) {
    place_n_bytes(2, 0xA0, 0x20); // LDY #$20
    execute_next_instruction();
    
    place_n_bytes(1, 0xC8); // INY
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x21, readByte(getCPU_YRegister()));
    verify_flags(0, 0, 0, 0, 0, 0);
}

void test_dex(void) {
    place_n_bytes(2, 0xA2, 0x01); // LDX #$01
    execute_next_instruction();
    
    place_n_bytes(1, 0xCA); // DEX
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x00, readByte(getCPU_XRegister()));
    verify_flags(0, 1, 0, 0, 0, 0); // Zero flag set
}

void test_dex_underflow(void) {
    place_n_bytes(2, 0xA2, 0x00); // LDX #$00
    execute_next_instruction();
    
    place_n_bytes(1, 0xCA); // DEX
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0xFF, readByte(getCPU_XRegister()));
    verify_flags(0, 0, 0, 0, 0, 1); // Negative flag set
}

void test_dey(void) {
    place_n_bytes(2, 0xA0, 0x01); // LDY #$01
    execute_next_instruction();
    
    place_n_bytes(1, 0x88); // DEY
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x00, readByte(getCPU_YRegister()));
    verify_flags(0, 1, 0, 0, 0, 0); // Zero flag set
}

// Enhanced flag operations
void test_sec(void) {
    place_n_bytes(1, 0x38); // SEC
    execute_next_instruction();
    
    verify_flags(1, 0, 0, 0, 0, 0); // Only carry set
}

void test_clc(void) {
    setCPUStatusFlag(CARRY, true);
    place_n_bytes(1, 0x18); // CLC
    execute_next_instruction();
    
    verify_flags(0, 0, 0, 0, 0, 0); // All flags clear
}

void test_sed(void) {
    place_n_bytes(1, 0xF8); // SED
    execute_next_instruction();
    
    verify_flags(0, 0, 0, 1, 0, 0); // Only decimal set
}

void test_cld(void) {
    setCPUStatusFlag(DECIMAL, true);
    place_n_bytes(1, 0xD8); // CLD
    execute_next_instruction();
    
    verify_flags(0, 0, 0, 0, 0, 0); // All flags clear
}

void test_sei(void) {
    place_n_bytes(1, 0x78); // SEI
    execute_next_instruction();
    
    verify_flags(0, 0, 1, 0, 0, 0); // Only interrupt set
}

void test_cli(void) {
    setCPUStatusFlag(INTERRUPT, true);
    place_n_bytes(1, 0x58); // CLI
    execute_next_instruction();
    
    verify_flags(0, 0, 0, 0, 0, 0); // All flags clear
}

void test_clv(void) {
    setCPUStatusFlag(CPU_OVERFLOW, true);
    place_n_bytes(1, 0xB8); // CLV
    execute_next_instruction();
    
    verify_flags(0, 0, 0, 0, 0, 0); // All flags clear
}

void test_nop(void) {
    unsigned char initial_a = readByte(getCPU_Accumulator());
    unsigned char initial_x = readByte(getCPU_XRegister());
    unsigned char initial_y = readByte(getCPU_YRegister());
    unsigned char initial_status = readByte(getCPU_StatusRegister());
    
    place_n_bytes(1, 0xEA); // NOP
    execute_next_instruction();
    
    // NOP should not change any registers or flags
    TEST_ASSERT_EQUAL_HEX(initial_a, readByte(getCPU_Accumulator()));
    TEST_ASSERT_EQUAL_HEX(initial_x, readByte(getCPU_XRegister()));
    TEST_ASSERT_EQUAL_HEX(initial_y, readByte(getCPU_YRegister()));
    TEST_ASSERT_EQUAL_HEX(initial_status, readByte(getCPU_StatusRegister()));
}

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
    RUN_TEST(test_sbc_immediate);
    RUN_TEST(test_sbc_immediate_borrow);
    RUN_TEST(test_sbc_immediate_zero);
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
    
    return UNITY_END();
}