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

// Zero Page Addressing Tests

void test_lda_zero_page(void) {
    // Set up zero page memory
    writeByte(0x80, 0x42);
    place_n_bytes(2, 0xA5, 0x80); // LDA $80
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x42, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 0);
}

void test_lda_zero_page_zero(void) {
    writeByte(0x81, 0x00);
    place_n_bytes(2, 0xA5, 0x81); // LDA $81
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x00, readByte(getCPU_Accumulator()));
    verify_flags(0, 1, 0, 0, 0, 0);
}

void test_lda_zero_page_negative(void) {
    writeByte(0x82, 0x80);
    place_n_bytes(2, 0xA5, 0x82); // LDA $82
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x80, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 1);
}

void test_ldx_zero_page(void) {
    writeByte(0x83, 0x55);
    place_n_bytes(2, 0xA6, 0x83); // LDX $83
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x55, readByte(getCPU_XRegister()));
    verify_flags(0, 0, 0, 0, 0, 0);
}

void test_ldy_zero_page(void) {
    writeByte(0x84, 0x33);
    place_n_bytes(2, 0xA4, 0x84); // LDY $84
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x33, readByte(getCPU_YRegister()));
    verify_flags(0, 0, 0, 0, 0, 0);
}

void test_sta_zero_page(void) {
    place_n_bytes(2, 0xA9, 0x77); // LDA #$77
    execute_next_instruction();
    
    place_n_bytes(2, 0x85, 0x85); // STA $85
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x77, readByte(0x85));
}

void test_stx_zero_page(void) {
    place_n_bytes(2, 0xA2, 0x88); // LDX #$88
    execute_next_instruction();
    
    place_n_bytes(2, 0x86, 0x86); // STX $86
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x88, readByte(0x86));
}

void test_sty_zero_page(void) {
    place_n_bytes(2, 0xA0, 0x99); // LDY #$99
    execute_next_instruction();
    
    place_n_bytes(2, 0x84, 0x87); // STY $87
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x99, readByte(0x87));
}

void test_inc_zero_page(void) {
    writeByte(0x88, 0x41);
    place_n_bytes(2, 0xE6, 0x88); // INC $88
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x42, readByte(0x88));
    verify_flags(0, 0, 0, 0, 0, 0);
}

void test_inc_zero_page_overflow(void) {
    writeByte(0x89, 0xFF);
    place_n_bytes(2, 0xE6, 0x89); // INC $89
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x00, readByte(0x89));
    verify_flags(0, 1, 0, 0, 0, 0);
}

void test_dec_zero_page(void) {
    writeByte(0x8A, 0x43);
    place_n_bytes(2, 0xC6, 0x8A); // DEC $8A
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x42, readByte(0x8A));
    verify_flags(0, 0, 0, 0, 0, 0);
}

void test_dec_zero_page_zero(void) {
    writeByte(0x8B, 0x01);
    place_n_bytes(2, 0xC6, 0x8B); // DEC $8B
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x00, readByte(0x8B));
    verify_flags(0, 1, 0, 0, 0, 0);
}

void test_asl_zero_page(void) {
    writeByte(0x8C, 0x42);
    place_n_bytes(2, 0x06, 0x8C); // ASL $8C
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x84, readByte(0x8C));
    verify_flags(0, 0, 0, 0, 0, 1);
}

void test_asl_zero_page_carry(void) {
    writeByte(0x8D, 0x81);
    place_n_bytes(2, 0x06, 0x8D); // ASL $8D
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x02, readByte(0x8D));
    verify_flags(1, 0, 0, 0, 0, 0);
}

void test_lsr_zero_page(void) {
    writeByte(0x8E, 0x84);
    place_n_bytes(2, 0x46, 0x8E); // LSR $8E
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x42, readByte(0x8E));
    verify_flags(0, 0, 0, 0, 0, 0);
}

void test_lsr_zero_page_carry(void) {
    writeByte(0x8F, 0x85);
    place_n_bytes(2, 0x46, 0x8F); // LSR $8F
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x42, readByte(0x8F));
    verify_flags(1, 0, 0, 0, 0, 0);
}

void test_rol_zero_page(void) {
    writeByte(0x90, 0x81);
    place_n_bytes(2, 0x26, 0x90); // ROL $90
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x02, readByte(0x90));
    verify_flags(1, 0, 0, 0, 0, 0);
}

void test_rol_zero_page_with_carry(void) {
    setCPUStatusFlag(CARRY, true);
    writeByte(0x91, 0x00);
    place_n_bytes(2, 0x26, 0x91); // ROL $91
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x01, readByte(0x91));
    verify_flags(0, 0, 0, 0, 0, 0);
}

void test_ror_zero_page(void) {
    writeByte(0x92, 0x02);
    place_n_bytes(2, 0x66, 0x92); // ROR $92
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x01, readByte(0x92));
    verify_flags(0, 0, 0, 0, 0, 0);
}

void test_ror_zero_page_with_carry(void) {
    setCPUStatusFlag(CARRY, true);
    writeByte(0x93, 0x00);
    place_n_bytes(2, 0x66, 0x93); // ROR $93
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x80, readByte(0x93));
    verify_flags(0, 0, 0, 0, 0, 1);
}

void test_adc_zero_page(void) {
    writeByte(0x94, 0x20);
    place_n_bytes(2, 0xA9, 0x10); // LDA #$10
    execute_next_instruction();
    
    place_n_bytes(2, 0x65, 0x94); // ADC $94
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x30, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 0);
}

void test_sbc_zero_page(void) {
    writeByte(0x95, 0x20);
    place_n_bytes(2, 0xA9, 0x50); // LDA #$50
    execute_next_instruction();
    
    place_n_bytes(2, 0xE5, 0x95); // SBC $95
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x2F, readByte(getCPU_Accumulator()));
    verify_flags(1, 0, 0, 0, 0, 0);
}

void test_ora_zero_page(void) {
    writeByte(0x96, 0xF0);
    place_n_bytes(2, 0xA9, 0x0F); // LDA #$0F
    execute_next_instruction();
    
    place_n_bytes(2, 0x05, 0x96); // ORA $96
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0xFF, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 1);
}

void test_and_zero_page(void) {
    writeByte(0x97, 0x0F);
    place_n_bytes(2, 0xA9, 0xFF); // LDA #$FF
    execute_next_instruction();
    
    place_n_bytes(2, 0x25, 0x97); // AND $97
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x0F, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 0);
}

void test_eor_zero_page(void) {
    writeByte(0x98, 0x0F);
    place_n_bytes(2, 0xA9, 0xAA); // LDA #$AA
    execute_next_instruction();
    
    place_n_bytes(2, 0x45, 0x98); // EOR $98
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0xA5, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 1);
}

void test_bit_zero_page(void) {
    writeByte(0x99, 0xC0); // Bits 7 and 6 set
    place_n_bytes(2, 0xA9, 0x3F); // LDA #$3F
    execute_next_instruction();
    
    place_n_bytes(2, 0x24, 0x99); // BIT $99
    execute_next_instruction();
    
    // BIT sets Zero flag if (A & mem) == 0
    // 0x3F & 0xC0 = 0x00 -> Zero flag set
    // Also copies bits 7 and 6 of memory to Overflow and Negative flags
    verify_flags(0, 1, 0, 0, 1, 1);
}

void test_cmp_zero_page(void) {
    writeByte(0x9A, 0x50);
    place_n_bytes(2, 0xA9, 0x50); // LDA #$50
    execute_next_instruction();
    
    place_n_bytes(2, 0xC5, 0x9A); // CMP $9A
    execute_next_instruction();
    
    verify_flags(1, 1, 0, 0, 0, 0);
}

void test_cpx_zero_page(void) {
    writeByte(0x9B, 0x30);
    place_n_bytes(2, 0xA2, 0x30); // LDX #$30
    execute_next_instruction();
    
    place_n_bytes(2, 0xE4, 0x9B); // CPX $9B
    execute_next_instruction();
    
    verify_flags(1, 1, 0, 0, 0, 0);
}

void test_cpy_zero_page(void) {
    writeByte(0x9C, 0x25);
    place_n_bytes(2, 0xA0, 0x25); // LDY #$25
    execute_next_instruction();
    
    place_n_bytes(2, 0xC4, 0x9C); // CPY $9C
    execute_next_instruction();
    
    verify_flags(1, 1, 0, 0, 0, 0);
}

// Zero Page,X and Zero Page,Y Addressing Tests

void test_lda_zero_page_x(void) {
    writeByte(0x90, 0x42); // Target address: 0x80 + 0x10
    place_n_bytes(2, 0xA2, 0x10); // LDX #$10
    execute_next_instruction();
    
    place_n_bytes(2, 0xB5, 0x80); // LDA $80,X
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x42, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 0);
}

void test_lda_zero_page_x_wrap(void) {
    writeByte(0x0F, 0x77); // Target address: 0xFF + 0x10 = 0x0F (wraps)
    place_n_bytes(2, 0xA2, 0x10); // LDX #$10
    execute_next_instruction();
    
    place_n_bytes(2, 0xB5, 0xFF); // LDA $FF,X
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x77, readByte(getCPU_Accumulator()));
}

void test_ldy_zero_page_x(void) {
    writeByte(0x90, 0x33); // Target address: 0x80 + 0x10
    place_n_bytes(2, 0xA2, 0x10); // LDX #$10
    execute_next_instruction();
    
    place_n_bytes(2, 0xB4, 0x80); // LDY $80,X
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x33, readByte(getCPU_YRegister()));
    verify_flags(0, 0, 0, 0, 0, 0);
}

void test_ldx_zero_page_y(void) {
    writeByte(0xA0, 0x55); // Target address: 0x80 + 0x20
    place_n_bytes(2, 0xA0, 0x20); // LDY #$20
    execute_next_instruction();
    
    place_n_bytes(2, 0xB6, 0x80); // LDX $80,Y
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x55, readByte(getCPU_XRegister()));
    verify_flags(0, 0, 0, 0, 0, 0);
}

void test_sta_zero_page_x(void) {
    place_n_bytes(2, 0xA9, 0x77); // LDA #$77
    execute_next_instruction();
    
    place_n_bytes(2, 0xA2, 0x10); // LDX #$10
    execute_next_instruction();
    
    place_n_bytes(2, 0x95, 0x80); // STA $80,X
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x77, readByte(0x90));
}

void test_sty_zero_page_x(void) {
    place_n_bytes(2, 0xA0, 0x99); // LDY #$99
    execute_next_instruction();
    
    place_n_bytes(2, 0xA2, 0x10); // LDX #$10
    execute_next_instruction();
    
    place_n_bytes(2, 0x94, 0x80); // STY $80,X
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x99, readByte(0x90));
}

void test_stx_zero_page_y(void) {
    place_n_bytes(2, 0xA2, 0x88); // LDX #$88
    execute_next_instruction();
    
    place_n_bytes(2, 0xA0, 0x20); // LDY #$20
    execute_next_instruction();
    
    place_n_bytes(2, 0x96, 0x80); // STX $80,Y
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x88, readByte(0xA0));
}

void test_inc_zero_page_x(void) {
    writeByte(0x90, 0x41); // Target address: 0x80 + 0x10
    place_n_bytes(2, 0xA2, 0x10); // LDX #$10
    execute_next_instruction();
    
    place_n_bytes(2, 0xF6, 0x80); // INC $80,X
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x42, readByte(0x90));
    verify_flags(0, 0, 0, 0, 0, 0);
}

void test_dec_zero_page_x(void) {
    writeByte(0x90, 0x43); // Target address: 0x80 + 0x10
    place_n_bytes(2, 0xA2, 0x10); // LDX #$10
    execute_next_instruction();
    
    place_n_bytes(2, 0xD6, 0x80); // DEC $80,X
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x42, readByte(0x90));
    verify_flags(0, 0, 0, 0, 0, 0);
}

void test_asl_zero_page_x(void) {
    writeByte(0x90, 0x42); // Target address: 0x80 + 0x10
    place_n_bytes(2, 0xA2, 0x10); // LDX #$10
    execute_next_instruction();
    
    place_n_bytes(2, 0x16, 0x80); // ASL $80,X
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x84, readByte(0x90));
    verify_flags(0, 0, 0, 0, 0, 1);
}

void test_lsr_zero_page_x(void) {
    writeByte(0x90, 0x84); // Target address: 0x80 + 0x10
    place_n_bytes(2, 0xA2, 0x10); // LDX #$10
    execute_next_instruction();
    
    place_n_bytes(2, 0x56, 0x80); // LSR $80,X
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x42, readByte(0x90));
    verify_flags(0, 0, 0, 0, 0, 0);
}

void test_rol_zero_page_x(void) {
    writeByte(0x90, 0x81); // Target address: 0x80 + 0x10
    place_n_bytes(2, 0xA2, 0x10); // LDX #$10
    execute_next_instruction();
    
    place_n_bytes(2, 0x36, 0x80); // ROL $80,X
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x02, readByte(0x90));
    verify_flags(1, 0, 0, 0, 0, 0);
}

void test_ror_zero_page_x(void) {
    writeByte(0x90, 0x02); // Target address: 0x80 + 0x10
    place_n_bytes(2, 0xA2, 0x10); // LDX #$10
    execute_next_instruction();
    
    place_n_bytes(2, 0x76, 0x80); // ROR $80,X
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x01, readByte(0x90));
    verify_flags(0, 0, 0, 0, 0, 0);
}

void test_adc_zero_page_x(void) {
    writeByte(0x90, 0x20); // Target address: 0x80 + 0x10
    place_n_bytes(2, 0xA9, 0x10); // LDA #$10
    execute_next_instruction();
    
    place_n_bytes(2, 0xA2, 0x10); // LDX #$10
    execute_next_instruction();
    
    place_n_bytes(2, 0x75, 0x80); // ADC $80,X
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x30, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 0);
}

void test_sbc_zero_page_x(void) {
    writeByte(0x90, 0x20); // Target address: 0x80 + 0x10
    place_n_bytes(2, 0xA9, 0x50); // LDA #$50
    execute_next_instruction();
    
    place_n_bytes(2, 0xA2, 0x10); // LDX #$10
    execute_next_instruction();
    
    place_n_bytes(2, 0xF5, 0x80); // SBC $80,X
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x2F, readByte(getCPU_Accumulator()));
    verify_flags(1, 0, 0, 0, 0, 0);
}

void test_ora_zero_page_x(void) {
    writeByte(0x90, 0xF0); // Target address: 0x80 + 0x10
    place_n_bytes(2, 0xA9, 0x0F); // LDA #$0F
    execute_next_instruction();
    
    place_n_bytes(2, 0xA2, 0x10); // LDX #$10
    execute_next_instruction();
    
    place_n_bytes(2, 0x15, 0x80); // ORA $80,X
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0xFF, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 1);
}

void test_and_zero_page_x(void) {
    writeByte(0x90, 0x0F); // Target address: 0x80 + 0x10
    place_n_bytes(2, 0xA9, 0xFF); // LDA #$FF
    execute_next_instruction();
    
    place_n_bytes(2, 0xA2, 0x10); // LDX #$10
    execute_next_instruction();
    
    place_n_bytes(2, 0x35, 0x80); // AND $80,X
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x0F, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 0);
}

void test_eor_zero_page_x(void) {
    writeByte(0x90, 0x0F); // Target address: 0x80 + 0x10
    place_n_bytes(2, 0xA9, 0xAA); // LDA #$AA
    execute_next_instruction();
    
    place_n_bytes(2, 0xA2, 0x10); // LDX #$10
    execute_next_instruction();
    
    place_n_bytes(2, 0x55, 0x80); // EOR $80,X
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0xA5, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 1);
}

void test_cmp_zero_page_x(void) {
    writeByte(0x90, 0x50); // Target address: 0x80 + 0x10
    place_n_bytes(2, 0xA9, 0x50); // LDA #$50
    execute_next_instruction();
    
    place_n_bytes(2, 0xA2, 0x10); // LDX #$10
    execute_next_instruction();
    
    place_n_bytes(2, 0xD5, 0x80); // CMP $80,X
    execute_next_instruction();
    
    verify_flags(1, 1, 0, 0, 0, 0);
}

// Absolute Addressing Tests

void test_lda_absolute(void) {
    writeByte(0x1234, 0x42);
    place_n_bytes(3, 0xAD, 0x34, 0x12); // LDA $1234
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x42, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 0);
}

void test_lda_absolute_zero(void) {
    writeByte(0x1235, 0x00);
    place_n_bytes(3, 0xAD, 0x35, 0x12); // LDA $1235
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x00, readByte(getCPU_Accumulator()));
    verify_flags(0, 1, 0, 0, 0, 0);
}

void test_lda_absolute_negative(void) {
    writeByte(0x1236, 0x80);
    place_n_bytes(3, 0xAD, 0x36, 0x12); // LDA $1236
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x80, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 1);
}

void test_ldx_absolute(void) {
    writeByte(0x1237, 0x55);
    place_n_bytes(3, 0xAE, 0x37, 0x12); // LDX $1237
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x55, readByte(getCPU_XRegister()));
    verify_flags(0, 0, 0, 0, 0, 0);
}

void test_ldy_absolute(void) {
    writeByte(0x1238, 0x33);
    place_n_bytes(3, 0xAC, 0x38, 0x12); // LDY $1238
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x33, readByte(getCPU_YRegister()));
    verify_flags(0, 0, 0, 0, 0, 0);
}

void test_sta_absolute(void) {
    place_n_bytes(2, 0xA9, 0x77); // LDA #$77
    execute_next_instruction();
    
    place_n_bytes(3, 0x8D, 0x39, 0x12); // STA $1239
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x77, readByte(0x1239));
}

void test_stx_absolute(void) {
    place_n_bytes(2, 0xA2, 0x88); // LDX #$88
    execute_next_instruction();
    
    place_n_bytes(3, 0x8E, 0x3A, 0x12); // STX $123A
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x88, readByte(0x123A));
}

void test_sty_absolute(void) {
    place_n_bytes(2, 0xA0, 0x99); // LDY #$99
    execute_next_instruction();
    
    place_n_bytes(3, 0x8C, 0x3B, 0x12); // STY $123B
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x99, readByte(0x123B));
}

void test_inc_absolute(void) {
    writeByte(0x123C, 0x41);
    place_n_bytes(3, 0xEE, 0x3C, 0x12); // INC $123C
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x42, readByte(0x123C));
    verify_flags(0, 0, 0, 0, 0, 0);
}

void test_inc_absolute_overflow(void) {
    writeByte(0x123D, 0xFF);
    place_n_bytes(3, 0xEE, 0x3D, 0x12); // INC $123D
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x00, readByte(0x123D));
    verify_flags(0, 1, 0, 0, 0, 0);
}

void test_dec_absolute(void) {
    writeByte(0x123E, 0x43);
    place_n_bytes(3, 0xCE, 0x3E, 0x12); // DEC $123E
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x42, readByte(0x123E));
    verify_flags(0, 0, 0, 0, 0, 0);
}

void test_dec_absolute_zero(void) {
    writeByte(0x123F, 0x01);
    place_n_bytes(3, 0xCE, 0x3F, 0x12); // DEC $123F
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x00, readByte(0x123F));
    verify_flags(0, 1, 0, 0, 0, 0);
}

void test_asl_absolute(void) {
    writeByte(0x1240, 0x42);
    place_n_bytes(3, 0x0E, 0x40, 0x12); // ASL $1240
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x84, readByte(0x1240));
    verify_flags(0, 0, 0, 0, 0, 1);
}

void test_asl_absolute_carry(void) {
    writeByte(0x1241, 0x81);
    place_n_bytes(3, 0x0E, 0x41, 0x12); // ASL $1241
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x02, readByte(0x1241));
    verify_flags(1, 0, 0, 0, 0, 0);
}

void test_lsr_absolute(void) {
    writeByte(0x1242, 0x84);
    place_n_bytes(3, 0x4E, 0x42, 0x12); // LSR $1242
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x42, readByte(0x1242));
    verify_flags(0, 0, 0, 0, 0, 0);
}

void test_lsr_absolute_carry(void) {
    writeByte(0x1243, 0x85);
    place_n_bytes(3, 0x4E, 0x43, 0x12); // LSR $1243
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x42, readByte(0x1243));
    verify_flags(1, 0, 0, 0, 0, 0);
}

void test_rol_absolute(void) {
    writeByte(0x1244, 0x81);
    place_n_bytes(3, 0x2E, 0x44, 0x12); // ROL $1244
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x02, readByte(0x1244));
    verify_flags(1, 0, 0, 0, 0, 0);
}

void test_rol_absolute_with_carry(void) {
    setCPUStatusFlag(CARRY, true);
    writeByte(0x1245, 0x00);
    place_n_bytes(3, 0x2E, 0x45, 0x12); // ROL $1245
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x01, readByte(0x1245));
    verify_flags(0, 0, 0, 0, 0, 0);
}

void test_ror_absolute(void) {
    writeByte(0x1246, 0x02);
    place_n_bytes(3, 0x6E, 0x46, 0x12); // ROR $1246
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x01, readByte(0x1246));
    verify_flags(0, 0, 0, 0, 0, 0);
}

void test_ror_absolute_with_carry(void) {
    setCPUStatusFlag(CARRY, true);
    writeByte(0x1247, 0x00);
    place_n_bytes(3, 0x6E, 0x47, 0x12); // ROR $1247
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x80, readByte(0x1247));
    verify_flags(0, 0, 0, 0, 0, 1);
}

void test_adc_absolute(void) {
    writeByte(0x1248, 0x20);
    place_n_bytes(2, 0xA9, 0x10); // LDA #$10
    execute_next_instruction();
    
    place_n_bytes(3, 0x6D, 0x48, 0x12); // ADC $1248
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x30, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 0);
}

void test_sbc_absolute(void) {
    writeByte(0x1249, 0x20);
    place_n_bytes(2, 0xA9, 0x50); // LDA #$50
    execute_next_instruction();
    
    place_n_bytes(3, 0xED, 0x49, 0x12); // SBC $1249
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x2F, readByte(getCPU_Accumulator()));
    verify_flags(1, 0, 0, 0, 0, 0);
}

void test_ora_absolute(void) {
    writeByte(0x124A, 0xF0);
    place_n_bytes(2, 0xA9, 0x0F); // LDA #$0F
    execute_next_instruction();
    
    place_n_bytes(3, 0x0D, 0x4A, 0x12); // ORA $124A
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0xFF, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 1);
}

void test_and_absolute(void) {
    writeByte(0x124B, 0x0F);
    place_n_bytes(2, 0xA9, 0xFF); // LDA #$FF
    execute_next_instruction();
    
    place_n_bytes(3, 0x2D, 0x4B, 0x12); // AND $124B
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x0F, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 0);
}

void test_eor_absolute(void) {
    writeByte(0x124C, 0x0F);
    place_n_bytes(2, 0xA9, 0xAA); // LDA #$AA
    execute_next_instruction();
    
    place_n_bytes(3, 0x4D, 0x4C, 0x12); // EOR $124C
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0xA5, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 1);
}

void test_cmp_absolute(void) {
    writeByte(0x124D, 0x50);
    place_n_bytes(2, 0xA9, 0x50); // LDA #$50
    execute_next_instruction();
    
    place_n_bytes(3, 0xCD, 0x4D, 0x12); // CMP $124D
    execute_next_instruction();
    
    verify_flags(1, 1, 0, 0, 0, 0);
}

void test_bit_absolute(void) {
    writeByte(0x124E, 0xC0); // Bits 7 and 6 set
    place_n_bytes(2, 0xA9, 0x3F); // LDA #$3F
    execute_next_instruction();
    
    place_n_bytes(3, 0x2C, 0x4E, 0x12); // BIT $124E
    execute_next_instruction();
    
    verify_flags(0, 1, 0, 0, 1, 1);
}

void test_cpx_absolute(void) {
    writeByte(0x124F, 0x30);
    place_n_bytes(2, 0xA2, 0x30); // LDX #$30
    execute_next_instruction();
    
    place_n_bytes(3, 0xEC, 0x4F, 0x12); // CPX $124F
    execute_next_instruction();
    
    verify_flags(1, 1, 0, 0, 0, 0);
}

void test_cpy_absolute(void) {
    writeByte(0x1250, 0x25);
    place_n_bytes(2, 0xA0, 0x25); // LDY #$25
    execute_next_instruction();
    
    place_n_bytes(3, 0xCC, 0x50, 0x12); // CPY $1250
    execute_next_instruction();
    
    verify_flags(1, 1, 0, 0, 0, 0);
}

// Absolute,X and Absolute,Y Addressing Tests

void test_lda_absolute_x(void) {
    writeByte(0x1244, 0x42); // Target address: 0x1234 + 0x10
    place_n_bytes(2, 0xA2, 0x10); // LDX #$10
    execute_next_instruction();
    
    place_n_bytes(3, 0xBD, 0x34, 0x12); // LDA $1234,X
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x42, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 0);
}

void test_lda_absolute_y(void) {
    writeByte(0x1254, 0x77); // Target address: 0x1234 + 0x20
    place_n_bytes(2, 0xA0, 0x20); // LDY #$20
    execute_next_instruction();
    
    place_n_bytes(3, 0xB9, 0x34, 0x12); // LDA $1234,Y
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x77, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 0);
}

void test_ldy_absolute_x(void) {
    writeByte(0x1244, 0x33); // Target address: 0x1234 + 0x10
    place_n_bytes(2, 0xA2, 0x10); // LDX #$10
    execute_next_instruction();
    
    place_n_bytes(3, 0xBC, 0x34, 0x12); // LDY $1234,X
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x33, readByte(getCPU_YRegister()));
    verify_flags(0, 0, 0, 0, 0, 0);
}

void test_ldx_absolute_y(void) {
    writeByte(0x1254, 0x55); // Target address: 0x1234 + 0x20
    place_n_bytes(2, 0xA0, 0x20); // LDY #$20
    execute_next_instruction();
    
    place_n_bytes(3, 0xBE, 0x34, 0x12); // LDX $1234,Y
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x55, readByte(getCPU_XRegister()));
    verify_flags(0, 0, 0, 0, 0, 0);
}

void test_sta_absolute_x(void) {
    place_n_bytes(2, 0xA9, 0x77); // LDA #$77
    execute_next_instruction();
    
    place_n_bytes(2, 0xA2, 0x10); // LDX #$10
    execute_next_instruction();
    
    place_n_bytes(3, 0x9D, 0x34, 0x12); // STA $1234,X
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x77, readByte(0x1244));
}

void test_sta_absolute_y(void) {
    place_n_bytes(2, 0xA9, 0x88); // LDA #$88
    execute_next_instruction();
    
    place_n_bytes(2, 0xA0, 0x20); // LDY #$20
    execute_next_instruction();
    
    place_n_bytes(3, 0x99, 0x34, 0x12); // STA $1234,Y
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x88, readByte(0x1254));
}

void test_inc_absolute_x(void) {
    writeByte(0x1244, 0x41); // Target address: 0x1234 + 0x10
    place_n_bytes(2, 0xA2, 0x10); // LDX #$10
    execute_next_instruction();
    
    place_n_bytes(3, 0xFE, 0x34, 0x12); // INC $1234,X
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x42, readByte(0x1244));
    verify_flags(0, 0, 0, 0, 0, 0);
}

void test_dec_absolute_x(void) {
    writeByte(0x1244, 0x43); // Target address: 0x1234 + 0x10
    place_n_bytes(2, 0xA2, 0x10); // LDX #$10
    execute_next_instruction();
    
    place_n_bytes(3, 0xDE, 0x34, 0x12); // DEC $1234,X
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x42, readByte(0x1244));
    verify_flags(0, 0, 0, 0, 0, 0);
}

void test_asl_absolute_x(void) {
    writeByte(0x1244, 0x42); // Target address: 0x1234 + 0x10
    place_n_bytes(2, 0xA2, 0x10); // LDX #$10
    execute_next_instruction();
    
    place_n_bytes(3, 0x1E, 0x34, 0x12); // ASL $1234,X
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x84, readByte(0x1244));
    verify_flags(0, 0, 0, 0, 0, 1);
}

void test_lsr_absolute_x(void) {
    writeByte(0x1244, 0x84); // Target address: 0x1234 + 0x10
    place_n_bytes(2, 0xA2, 0x10); // LDX #$10
    execute_next_instruction();
    
    place_n_bytes(3, 0x5E, 0x34, 0x12); // LSR $1234,X
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x42, readByte(0x1244));
    verify_flags(0, 0, 0, 0, 0, 0);
}

void test_rol_absolute_x(void) {
    writeByte(0x1244, 0x81); // Target address: 0x1234 + 0x10
    place_n_bytes(2, 0xA2, 0x10); // LDX #$10
    execute_next_instruction();
    
    place_n_bytes(3, 0x3E, 0x34, 0x12); // ROL $1234,X
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x02, readByte(0x1244));
    verify_flags(1, 0, 0, 0, 0, 0);
}

void test_ror_absolute_x(void) {
    writeByte(0x1244, 0x02); // Target address: 0x1234 + 0x10
    place_n_bytes(2, 0xA2, 0x10); // LDX #$10
    execute_next_instruction();
    
    place_n_bytes(3, 0x7E, 0x34, 0x12); // ROR $1234,X
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x01, readByte(0x1244));
    verify_flags(0, 0, 0, 0, 0, 0);
}

void test_adc_absolute_x(void) {
    writeByte(0x1244, 0x20); // Target address: 0x1234 + 0x10
    place_n_bytes(2, 0xA9, 0x10); // LDA #$10
    execute_next_instruction();
    
    place_n_bytes(2, 0xA2, 0x10); // LDX #$10
    execute_next_instruction();
    
    place_n_bytes(3, 0x7D, 0x34, 0x12); // ADC $1234,X
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x30, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 0);
}

void test_adc_absolute_y(void) {
    writeByte(0x1254, 0x20); // Target address: 0x1234 + 0x20
    place_n_bytes(2, 0xA9, 0x10); // LDA #$10
    execute_next_instruction();
    
    place_n_bytes(2, 0xA0, 0x20); // LDY #$20
    execute_next_instruction();
    
    place_n_bytes(3, 0x79, 0x34, 0x12); // ADC $1234,Y
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x30, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 0);
}

void test_sbc_absolute_x(void) {
    writeByte(0x1244, 0x20); // Target address: 0x1234 + 0x10
    place_n_bytes(2, 0xA9, 0x50); // LDA #$50
    execute_next_instruction();
    
    place_n_bytes(2, 0xA2, 0x10); // LDX #$10
    execute_next_instruction();
    
    place_n_bytes(3, 0xFD, 0x34, 0x12); // SBC $1234,X
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x2F, readByte(getCPU_Accumulator()));
    verify_flags(1, 0, 0, 0, 0, 0);
}

void test_sbc_absolute_y(void) {
    writeByte(0x1254, 0x20); // Target address: 0x1234 + 0x20
    place_n_bytes(2, 0xA9, 0x50); // LDA #$50
    execute_next_instruction();
    
    place_n_bytes(2, 0xA0, 0x20); // LDY #$20
    execute_next_instruction();
    
    place_n_bytes(3, 0xF9, 0x34, 0x12); // SBC $1234,Y
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x2F, readByte(getCPU_Accumulator()));
    verify_flags(1, 0, 0, 0, 0, 0);
}

void test_ora_absolute_x(void) {
    writeByte(0x1244, 0xF0); // Target address: 0x1234 + 0x10
    place_n_bytes(2, 0xA9, 0x0F); // LDA #$0F
    execute_next_instruction();
    
    place_n_bytes(2, 0xA2, 0x10); // LDX #$10
    execute_next_instruction();
    
    place_n_bytes(3, 0x1D, 0x34, 0x12); // ORA $1234,X
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0xFF, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 1);
}

void test_ora_absolute_y(void) {
    writeByte(0x1254, 0xF0); // Target address: 0x1234 + 0x20
    place_n_bytes(2, 0xA9, 0x0F); // LDA #$0F
    execute_next_instruction();
    
    place_n_bytes(2, 0xA0, 0x20); // LDY #$20
    execute_next_instruction();
    
    place_n_bytes(3, 0x19, 0x34, 0x12); // ORA $1234,Y
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0xFF, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 1);
}

void test_and_absolute_x(void) {
    writeByte(0x1244, 0x0F); // Target address: 0x1234 + 0x10
    place_n_bytes(2, 0xA9, 0xFF); // LDA #$FF
    execute_next_instruction();
    
    place_n_bytes(2, 0xA2, 0x10); // LDX #$10
    execute_next_instruction();
    
    place_n_bytes(3, 0x3D, 0x34, 0x12); // AND $1234,X
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x0F, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 0);
}

void test_and_absolute_y(void) {
    writeByte(0x1254, 0x0F); // Target address: 0x1234 + 0x20
    place_n_bytes(2, 0xA9, 0xFF); // LDA #$FF
    execute_next_instruction();
    
    place_n_bytes(2, 0xA0, 0x20); // LDY #$20
    execute_next_instruction();
    
    place_n_bytes(3, 0x39, 0x34, 0x12); // AND $1234,Y
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x0F, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 0);
}

void test_eor_absolute_x(void) {
    writeByte(0x1244, 0x0F); // Target address: 0x1234 + 0x10
    place_n_bytes(2, 0xA9, 0xAA); // LDA #$AA
    execute_next_instruction();
    
    place_n_bytes(2, 0xA2, 0x10); // LDX #$10
    execute_next_instruction();
    
    place_n_bytes(3, 0x5D, 0x34, 0x12); // EOR $1234,X
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0xA5, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 1);
}

void test_eor_absolute_y(void) {
    writeByte(0x1254, 0x0F); // Target address: 0x1234 + 0x20
    place_n_bytes(2, 0xA9, 0xAA); // LDA #$AA
    execute_next_instruction();
    
    place_n_bytes(2, 0xA0, 0x20); // LDY #$20
    execute_next_instruction();
    
    place_n_bytes(3, 0x59, 0x34, 0x12); // EOR $1234,Y
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0xA5, readByte(getCPU_Accumulator()));
    verify_flags(0, 0, 0, 0, 0, 1);
}

void test_cmp_absolute_x(void) {
    writeByte(0x1244, 0x50); // Target address: 0x1234 + 0x10
    place_n_bytes(2, 0xA9, 0x50); // LDA #$50
    execute_next_instruction();
    
    place_n_bytes(2, 0xA2, 0x10); // LDX #$10
    execute_next_instruction();
    
    place_n_bytes(3, 0xDD, 0x34, 0x12); // CMP $1234,X
    execute_next_instruction();
    
    verify_flags(1, 1, 0, 0, 0, 0);
}

void test_cmp_absolute_y(void) {
    writeByte(0x1254, 0x50); // Target address: 0x1234 + 0x20
    place_n_bytes(2, 0xA9, 0x50); // LDA #$50
    execute_next_instruction();
    
    place_n_bytes(2, 0xA0, 0x20); // LDY #$20
    execute_next_instruction();
    
    place_n_bytes(3, 0xD9, 0x34, 0x12); // CMP $1234,Y
    execute_next_instruction();
    
    verify_flags(1, 1, 0, 0, 0, 0);
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

