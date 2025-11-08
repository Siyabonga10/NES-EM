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

void place_two_bytes(unsigned char opcode, unsigned char operand) {
    test_rom_data[memPtr] = opcode; 
    memPtr += 1;
    test_rom_data[memPtr] = operand;
    memPtr += 1;
}

void test_lda_immediate(void) {
    place_two_bytes(0xA9, 0x42);
    
    ExecutionInfo info = getNextInstruction();
    executeInstruction(info);
    
    TEST_ASSERT_EQUAL_HEX(0x42, readByte(getCPU_Accumulator()));
    TEST_ASSERT_EQUAL(0, getCPUStatusFlag(ZERO));
    TEST_ASSERT_EQUAL(0, getCPUStatusFlag(NEGATIVE));
}

void test_lda_immediate_zero(void) {
    place_two_bytes(0xA9, 0x00);
    
    ExecutionInfo info = getNextInstruction();
    executeInstruction(info);
    
    TEST_ASSERT_EQUAL_HEX(0x00, readByte(getCPU_Accumulator()));
    TEST_ASSERT_EQUAL(0, getCPUStatusFlag(NEGATIVE));
    TEST_ASSERT_EQUAL(1, getCPUStatusFlag(ZERO)); 
}

void test_lda_immediate_negative(void) {
    place_two_bytes(0xA9, 0x80);
    
    ExecutionInfo info = getNextInstruction();
    executeInstruction(info);
    
    TEST_ASSERT_EQUAL_HEX(0x80, readByte(getCPU_Accumulator()));
    TEST_ASSERT_EQUAL(0, getCPUStatusFlag(ZERO));
    TEST_ASSERT_EQUAL(1, getCPUStatusFlag(NEGATIVE));  
}

void test_ldx_immediate(void) {
    place_two_bytes(0xA2, 0x55);
    
    ExecutionInfo info = getNextInstruction();
    executeInstruction(info);
    
    TEST_ASSERT_EQUAL_HEX(0x55, readByte(getCPU_XRegister()));
    TEST_ASSERT_EQUAL(0, getCPUStatusFlag(ZERO));
    TEST_ASSERT_EQUAL(0, getCPUStatusFlag(NEGATIVE));
}
 
void test_ldy_immediate(void) {
    place_two_bytes(0xA0, 0x33);
    
    ExecutionInfo info = getNextInstruction();
    executeInstruction(info);
    
    TEST_ASSERT_EQUAL_HEX(0x33, readByte(getCPU_YRegister()));
    TEST_ASSERT_EQUAL(0, getCPUStatusFlag(ZERO));
    TEST_ASSERT_EQUAL(0, getCPUStatusFlag(NEGATIVE));
}

void test_adc_immediate(void) {
    place_two_bytes(0xA9, 0x10);
    
    ExecutionInfo info = getNextInstruction();
    executeInstruction(info);
    
    place_two_bytes(0x69, 0x20);
    
    info = getNextInstruction();
    executeInstruction(info);
    
    TEST_ASSERT_EQUAL_HEX(0x30, readByte(getCPU_Accumulator()));  // 0x10 + 0x20 = 0x30
    TEST_ASSERT_EQUAL(0, getCPUStatusFlag(CARRY));
    TEST_ASSERT_EQUAL(0, getCPUStatusFlag(ZERO));
    TEST_ASSERT_EQUAL(0, getCPUStatusFlag(CPU_OVERFLOW));
    TEST_ASSERT_EQUAL(0, getCPUStatusFlag(NEGATIVE));
}

void test_adc_immediate_with_carry(void) {
    setCPUStatusFlag(CARRY, true);
    place_two_bytes(0xA9, 0x10);
    
    ExecutionInfo info = getNextInstruction();
    executeInstruction(info);
    
    place_two_bytes(0x69, 0x20);
    
    info = getNextInstruction();
    executeInstruction(info);
    
    TEST_ASSERT_EQUAL_HEX(0x31, readByte(getCPU_Accumulator()));  
}

void test_sbc_immediate(void) {
    place_two_bytes(0xA9, 0x50);
    
    ExecutionInfo info = getNextInstruction();
    executeInstruction(info);
    
    place_two_bytes(0xE9, 0x20);

    info = getNextInstruction();
    executeInstruction(info);
    
    TEST_ASSERT_EQUAL_HEX(0x2F, readByte(getCPU_Accumulator())); 
    TEST_ASSERT_EQUAL(1, getCPUStatusFlag(CARRY));  
}
void test_ora_immediate(void) {
    place_two_bytes(0xA9, 0x0F); // LDA #$0F
    ExecutionInfo info = getNextInstruction();
    executeInstruction(info);
    
    place_two_bytes(0x09, 0xF0); // ORA #$F0
    info = getNextInstruction();
    executeInstruction(info);
    
    TEST_ASSERT_EQUAL_HEX(0xFF, readByte(getCPU_Accumulator()));  // 0x0F | 0xF0 = 0xFF
    TEST_ASSERT_EQUAL(1, getCPUStatusFlag(NEGATIVE));
}

void test_and_immediate(void) {
    place_two_bytes(0xA9, 0xFF); // LDA #$FF
    ExecutionInfo info = getNextInstruction();
    executeInstruction(info);
    
    place_two_bytes(0x29, 0x0F); // AND #$0F
    info = getNextInstruction();
    executeInstruction(info);
    
    TEST_ASSERT_EQUAL_HEX(0x0F, readByte(getCPU_Accumulator()));  // 0xFF & 0x0F = 0x0F
    TEST_ASSERT_EQUAL(0, getCPUStatusFlag(ZERO));
}

void test_eor_immediate(void) {
    place_two_bytes(0xA9, 0xAA); // LDA #$AA
    ExecutionInfo info = getNextInstruction();
    executeInstruction(info);
    
    place_two_bytes(0x49, 0x0F); // EOR #$0F
    info = getNextInstruction();
    executeInstruction(info);
    
    TEST_ASSERT_EQUAL_HEX(0xA5, readByte(getCPU_Accumulator()));  // 0xAA ^ 0x0F = 0xA5
}

void test_cmp_immediate(void) {
    place_two_bytes(0xA9, 0x50); // LDA #$50
    ExecutionInfo info = getNextInstruction();
    executeInstruction(info);
    
    place_two_bytes(0xC9, 0x50); // CMP #$50
    info = getNextInstruction();
    executeInstruction(info);
    
    TEST_ASSERT_EQUAL(1, getCPUStatusFlag(ZERO));  // Equal values
    TEST_ASSERT_EQUAL(1, getCPUStatusFlag(CARRY)); // A >= memory
}

void test_cmp_immediate_greater(void) {
    place_two_bytes(0xA9, 0x60); // LDA #$60
    ExecutionInfo info = getNextInstruction();
    executeInstruction(info);
    
    place_two_bytes(0xC9, 0x50); // CMP #$50
    info = getNextInstruction();
    executeInstruction(info);
    
    TEST_ASSERT_EQUAL(0, getCPUStatusFlag(ZERO));  // Not equal
    TEST_ASSERT_EQUAL(1, getCPUStatusFlag(CARRY)); // A > memory
}

void test_cmp_immediate_less(void) {
    place_two_bytes(0xA9, 0x40); // LDA #$40
    ExecutionInfo info = getNextInstruction();
    executeInstruction(info);
    
    place_two_bytes(0xC9, 0x50); // CMP #$50
    info = getNextInstruction();
    executeInstruction(info);
    
    TEST_ASSERT_EQUAL(0, getCPUStatusFlag(ZERO));  // Not equal
    TEST_ASSERT_EQUAL(0, getCPUStatusFlag(CARRY)); // A < memory
}

void test_cpx_immediate(void) {
    place_two_bytes(0xA2, 0x30); // LDX #$30
    ExecutionInfo info = getNextInstruction();
    executeInstruction(info);
    
    place_two_bytes(0xE0, 0x30); // CPX #$30
    info = getNextInstruction();
    executeInstruction(info);
    
    TEST_ASSERT_EQUAL(1, getCPUStatusFlag(ZERO));  // Equal values
    TEST_ASSERT_EQUAL(1, getCPUStatusFlag(CARRY)); // X >= memory
}

void test_cpy_immediate(void) {
    place_two_bytes(0xA0, 0x25); // LDY #$25
    ExecutionInfo info = getNextInstruction();
    executeInstruction(info);
    
    place_two_bytes(0xC0, 0x25); // CPY #$25
    info = getNextInstruction();
    executeInstruction(info);
    
    TEST_ASSERT_EQUAL(1, getCPUStatusFlag(ZERO));  // Equal values
    TEST_ASSERT_EQUAL(1, getCPUStatusFlag(CARRY)); // Y >= memory
}

int main(void) {
    UNITY_BEGIN();
    
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
    
    return UNITY_END();
}