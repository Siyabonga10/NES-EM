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
    bootCPU();
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

void test_lda_immediate(void) {
    place_n_bytes(2, 0xA9, 0x42);
    
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x42, readByte(getCPU_Accumulator()));
    TEST_ASSERT_EQUAL(0, getCPUStatusFlag(ZERO));
    TEST_ASSERT_EQUAL(0, getCPUStatusFlag(NEGATIVE));
}

void test_lda_immediate_zero(void) {
    place_n_bytes(2, 0xA9, 0x00);
    
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x00, readByte(getCPU_Accumulator()));
    TEST_ASSERT_EQUAL(0, getCPUStatusFlag(NEGATIVE));
    TEST_ASSERT_EQUAL(1, getCPUStatusFlag(ZERO)); 
}

void test_lda_immediate_negative(void) {
    place_n_bytes(2, 0xA9, 0x80);
    
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x80, readByte(getCPU_Accumulator()));
    TEST_ASSERT_EQUAL(0, getCPUStatusFlag(ZERO));
    TEST_ASSERT_EQUAL(1, getCPUStatusFlag(NEGATIVE));  
}

void test_ldx_immediate(void) {
    place_n_bytes(2, 0xA2, 0x55);
    
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x55, readByte(getCPU_XRegister()));
    TEST_ASSERT_EQUAL(0, getCPUStatusFlag(ZERO));
    TEST_ASSERT_EQUAL(0, getCPUStatusFlag(NEGATIVE));
}
 
void test_ldy_immediate(void) {
    place_n_bytes(2, 0xA0, 0x33);
    
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x33, readByte(getCPU_YRegister()));
    TEST_ASSERT_EQUAL(0, getCPUStatusFlag(ZERO));
    TEST_ASSERT_EQUAL(0, getCPUStatusFlag(NEGATIVE));
}

void test_adc_immediate(void) {
    place_n_bytes(2, 0xA9, 0x10);
    
    execute_next_instruction();
    
    place_n_bytes(2, 0x69, 0x20);
    
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x30, readByte(getCPU_Accumulator()));  // 0x10 + 0x20 = 0x30
    TEST_ASSERT_EQUAL(0, getCPUStatusFlag(CARRY));
    TEST_ASSERT_EQUAL(0, getCPUStatusFlag(ZERO));
    TEST_ASSERT_EQUAL(0, getCPUStatusFlag(CPU_OVERFLOW));
    TEST_ASSERT_EQUAL(0, getCPUStatusFlag(NEGATIVE));
}

void test_adc_immediate_with_carry(void) {
    setCPUStatusFlag(CARRY, true);
    place_n_bytes(2, 0xA9, 0x10);
    
    execute_next_instruction();
    
    place_n_bytes(2, 0x69, 0x20);
    
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x31, readByte(getCPU_Accumulator()));  
}

void test_sbc_immediate(void) {
    place_n_bytes(2, 0xA9, 0x50);
    
    execute_next_instruction();
    
    place_n_bytes(2, 0xE9, 0x20);

    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x2F, readByte(getCPU_Accumulator())); 
    TEST_ASSERT_EQUAL(1, getCPUStatusFlag(CARRY));  
}
void test_ora_immediate(void) {
    place_n_bytes(2, 0xA9, 0x0F); // LDA #$0F
    execute_next_instruction();
    
    place_n_bytes(2, 0x09, 0xF0); // ORA #$F0
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0xFF, readByte(getCPU_Accumulator()));  // 0x0F | 0xF0 = 0xFF
    TEST_ASSERT_EQUAL(1, getCPUStatusFlag(NEGATIVE));
}

void test_and_immediate(void) {
    place_n_bytes(2, 0xA9, 0xFF); // LDA #$FF
    execute_next_instruction();
    
    place_n_bytes(2, 0x29, 0x0F); // AND #$0F
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x0F, readByte(getCPU_Accumulator()));  // 0xFF & 0x0F = 0x0F
    TEST_ASSERT_EQUAL(0, getCPUStatusFlag(ZERO));
}

void test_eor_immediate(void) {
    place_n_bytes(2, 0xA9, 0xAA); // LDA #$AA
    execute_next_instruction();
    
    place_n_bytes(2, 0x49, 0x0F); // EOR #$0F
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0xA5, readByte(getCPU_Accumulator()));  // 0xAA ^ 0x0F = 0xA5
}

void test_cmp_immediate(void) {
    place_n_bytes(2, 0xA9, 0x50); // LDA #$50
    execute_next_instruction();
    
    place_n_bytes(2, 0xC9, 0x50); // CMP #$50
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL(1, getCPUStatusFlag(ZERO));  // Equal values
    TEST_ASSERT_EQUAL(1, getCPUStatusFlag(CARRY)); // A >= memory
}

void test_cmp_immediate_greater(void) {
    place_n_bytes(2, 0xA9, 0x60); // LDA #$60
    execute_next_instruction();
    
    place_n_bytes(2, 0xC9, 0x50); // CMP #$50
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL(0, getCPUStatusFlag(ZERO));  // Not equal
    TEST_ASSERT_EQUAL(1, getCPUStatusFlag(CARRY)); // A > memory
}

void test_cmp_immediate_less(void) {
    place_n_bytes(2, 0xA9, 0x40); // LDA #$40
    execute_next_instruction();
    
    place_n_bytes(2, 0xC9, 0x50); // CMP #$50
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL(0, getCPUStatusFlag(ZERO));  // Not equal
    TEST_ASSERT_EQUAL(0, getCPUStatusFlag(CARRY)); // A < memory
}

void test_cpx_immediate(void) {
    place_n_bytes(2, 0xA2, 0x30); // LDX #$30
    execute_next_instruction();
    
    place_n_bytes(2, 0xE0, 0x30); // CPX #$30
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL(1, getCPUStatusFlag(ZERO));  // Equal values
    TEST_ASSERT_EQUAL(1, getCPUStatusFlag(CARRY)); // X >= memory
}

void test_cpy_immediate(void) {
    place_n_bytes(2, 0xA0, 0x25); // LDY #$25
    execute_next_instruction();
    
    place_n_bytes(2, 0xC0, 0x25); // CPY #$25
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL(1, getCPUStatusFlag(ZERO));  // Equal values
    TEST_ASSERT_EQUAL(1, getCPUStatusFlag(CARRY)); // Y >= memory
}

// Accumulator operations
void test_rol_accumulator(void) {
    place_n_bytes(2, 0xA9, 0x81); // LDA #$81 - set accumulator to test value
    execute_next_instruction();
    
    place_n_bytes(1, 0x2A); // ROL A
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x02, readByte(getCPU_Accumulator())); // 0x81 << 1 = 0x02 with carry
    TEST_ASSERT_EQUAL(1, getCPUStatusFlag(CARRY));
}

void test_asl_accumulator(void) {
    place_n_bytes(2, 0xA9, 0x42); // LDA #$42
    execute_next_instruction();
    
    place_n_bytes(1, 0x0A); // ASL A
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x84, readByte(getCPU_Accumulator()));
}

void test_ror_accumulator(void) {
    place_n_bytes(2, 0xA9, 0x02); // LDA #$02  
    execute_next_instruction();
    
    place_n_bytes(1, 0x6A); // ROR A
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x01, readByte(getCPU_Accumulator()));
}

void test_lsr_accumulator(void) {
    place_n_bytes(2, 0xA9, 0x84); // LDA #$84
    execute_next_instruction();
    
    place_n_bytes(1, 0x4A); // LSR A
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x42, readByte(getCPU_Accumulator()));
}

// Transfer operations
void test_txa(void) {
    place_n_bytes(2, 0xA2, 0x55); // LDX #$55
    execute_next_instruction();
    
    place_n_bytes(1, 0x8A); // TXA
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x55, readByte(getCPU_Accumulator()));
}

void test_tya(void) {
    place_n_bytes(2, 0xA0, 0x33); // LDY #$33
    execute_next_instruction();
    
    place_n_bytes(1, 0x98); // TYA
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x33, readByte(getCPU_Accumulator()));
}

void test_tax(void) {
    place_n_bytes(2, 0xA9, 0x77); // LDA #$77
    execute_next_instruction();
    
    place_n_bytes(1, 0xAA); // TAX
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x77, readByte(getCPU_XRegister()));
}

void test_tay(void) {
    place_n_bytes(2, 0xA9, 0x99); // LDA #$99
    execute_next_instruction();
    
    place_n_bytes(1, 0xA8); // TAY
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x99, readByte(getCPU_YRegister()));
}

// Increment/Decrement operations
void test_inx(void) {
    place_n_bytes(2, 0xA2, 0x10); // LDX #$10
    execute_next_instruction();
    
    place_n_bytes(1, 0xE8); // INX
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x11, readByte(getCPU_XRegister()));
}

void test_iny(void) {
    place_n_bytes(2, 0xA0, 0x20); // LDY #$20
    execute_next_instruction();
    
    place_n_bytes(1, 0xC8); // INY
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x21, readByte(getCPU_YRegister()));
}

void test_dex(void) {
    place_n_bytes(2, 0xA2, 0x01); // LDX #$01
    execute_next_instruction();
    
    place_n_bytes(1, 0xCA); // DEX
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x00, readByte(getCPU_XRegister()));
    TEST_ASSERT_EQUAL(1, getCPUStatusFlag(ZERO));
}

void test_dey(void) {
    place_n_bytes(2, 0xA0, 0x01); // LDY #$01
    execute_next_instruction();
    
    place_n_bytes(1, 0x88); // DEY
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL_HEX(0x00, readByte(getCPU_YRegister()));
    TEST_ASSERT_EQUAL(1, getCPUStatusFlag(ZERO));
}

// Flag operations
void test_sec(void) {
    place_n_bytes(1, 0x38); // SEC
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL(1, getCPUStatusFlag(CARRY));
}

void test_clc(void) {
    setCPUStatusFlag(CARRY, true); // Set carry first
    place_n_bytes(1, 0x18); // CLC
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL(0, getCPUStatusFlag(CARRY));
}

void test_sed(void) {
    place_n_bytes(1, 0xF8); // SED
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL(1, getCPUStatusFlag(DECIMAL));
}

void test_cld(void) {
    setCPUStatusFlag(DECIMAL, true); // Set decimal first
    place_n_bytes(1, 0xD8); // CLD
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL(0, getCPUStatusFlag(DECIMAL));
}

void test_sei(void) {
    place_n_bytes(1, 0x78); // SEI
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL(1, getCPUStatusFlag(INTERRUPT));
}

void test_cli(void) {
    setCPUStatusFlag(INTERRUPT, true); // Set interrupt first
    place_n_bytes(1, 0x58); // CLI
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL(0, getCPUStatusFlag(INTERRUPT));
}

void test_clv(void) {
    setCPUStatusFlag(CPU_OVERFLOW, true); // Set overflow first
    place_n_bytes(1, 0xB8); // CLV
    execute_next_instruction();
    
    TEST_ASSERT_EQUAL(0, getCPUStatusFlag(CPU_OVERFLOW));
}

void test_nop(void) {
    unsigned char initial_a = readByte(getCPU_Accumulator());
    unsigned char initial_x = readByte(getCPU_XRegister());
    unsigned char initial_y = readByte(getCPU_YRegister());
    
    place_n_bytes(1, 0xEA); // NOP
    execute_next_instruction();
    
    // NOP should not change any registers
    TEST_ASSERT_EQUAL_HEX(initial_a, readByte(getCPU_Accumulator()));
    TEST_ASSERT_EQUAL_HEX(initial_x, readByte(getCPU_XRegister()));
    TEST_ASSERT_EQUAL_HEX(initial_y, readByte(getCPU_YRegister()));
}

int main(void) {
    UNITY_BEGIN();
    
    // Immediate addressing tests
    RUN_TEST(test_lda_immediate);
    RUN_TEST(test_lda_immediate_zero);
    RUN_TEST(test_lda_immediate_negative);
    RUN_TEST(test_ldx_immediate);
    RUN_TEST(test_ldy_immediate);
    RUN_TEST(test_adc_immediate);
    RUN_TEST(test_adc_immediate_with_carry);
    RUN_TEST(test_sbc_immediate);
    RUN_TEST(test_ora_immediate);
    RUN_TEST(test_and_immediate);
    RUN_TEST(test_eor_immediate);
    RUN_TEST(test_cmp_immediate);
    RUN_TEST(test_cmp_immediate_greater);
    RUN_TEST(test_cmp_immediate_less);
    RUN_TEST(test_cpx_immediate);
    RUN_TEST(test_cpy_immediate);
    
    // Implied addressing tests - Accumulator operations
    RUN_TEST(test_rol_accumulator);
    RUN_TEST(test_asl_accumulator);
    RUN_TEST(test_ror_accumulator);
    RUN_TEST(test_lsr_accumulator);
    
    // Implied addressing tests - Transfer operations
    RUN_TEST(test_txa);
    RUN_TEST(test_tya);
    RUN_TEST(test_tax);
    RUN_TEST(test_tay);
    
    // Implied addressing tests - Increment/Decrement operations
    RUN_TEST(test_inx);
    RUN_TEST(test_iny);
    RUN_TEST(test_dex);
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