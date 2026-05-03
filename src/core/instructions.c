#include "instructions.h"
#include "bus.h"
#include "addressing_modes.h"
#include "statusFlag.h"
#include <stdlib.h>
#include <stdio.h>

static ExecutionInfo last_instruction = (ExecutionInfo){.addressing_mode = NULL, .executor = NULL, .instruction_size = 0, .clock_cycles = 0};
static int pending_i_flag = -1; // -1 no change, 0 clear I flag, 1 set I flag (delayed by one instruction)
static int i_flag_delay = 0;    // number of instructions remaining before applying pending_i_flag

// Helper to get indirect pointer for ZP_IND_INDX_Y before adding Y
static unsigned short get_indirect_pointer(unsigned char zp)
{
    return read_byte(zp) | (read_byte((zp + 1) & 0xFF) << 8);
}

// Helper to determine if page crossing adds an extra cycle
// Only addressing modes that can cross a page boundary with an extra cycle are:
// ABS_INDEX_X, ABS_INDEX_Y, ZP_IND_INDX_Y (indirect indexed Y)
static int extra_page_cycle(int pc, int (*addressing_mode)(int), int operandAddr)
{
    if (addressing_mode == ABS_INDEX_X || addressing_mode == ABS_INDEX_Y)
    {
        unsigned short base = ABS_A(pc);
        if ((base & 0xFF00) != (operandAddr & 0xFF00))
        {
            return 1; // page crossed
        }
    }
    if (addressing_mode == ZP_IND_INDX_Y)
    {
        unsigned char zp = read_byte(pc);
        unsigned short pointer = get_indirect_pointer(zp);
        if ((pointer & 0xFF00) != (operandAddr & 0xFF00))
        {
            return 1; // page crossed
        }
    }
    return 0;
}

// Define all instructions
// General format is to take in the address to the operand, compute the result, potentially having side effects, return the result just in case
unsigned char ADC(ExecutionInfo *exInfo)
{
    int operandAddr = exInfo->addressing_mode(get_pc() + 1);
    if (extra_page_cycle(get_pc() + 1, exInfo->addressing_mode, operandAddr))
    {
        exInfo->clock_cycles += 1; // extra cycle for page crossing
    }
    unsigned char acc = read_byte(get_cpu_accumulator());
    unsigned char mem = read_byte(operandAddr);
    int tmp = acc + mem + (get_cpu_status_flag(CARRY) ? 1 : 0);

    set_cpu_status_flag(CARRY, tmp > 0xFF);
    set_cpu_status_flag(ZERO, (unsigned char)tmp == 0);
    set_cpu_status_flag(CPU_OVERFLOW, (tmp ^ acc) & (tmp ^ mem) & 0x80);
    set_cpu_status_flag(NEGATIVE, tmp & (1 << NEGATIVE));
    write_byte(get_cpu_accumulator(), (unsigned char)tmp);
    return tmp;
}

unsigned char AND(ExecutionInfo *exInfo)
{
    int operandAddr = exInfo->addressing_mode(get_pc() + 1);
    if (extra_page_cycle(get_pc() + 1, exInfo->addressing_mode, operandAddr))
    {
        exInfo->clock_cycles += 1; // extra cycle for page crossing
    }
    unsigned char result = read_byte(operandAddr) & read_byte(get_cpu_accumulator());
    write_byte(get_cpu_accumulator(), result);
    set_cpu_status_flag(ZERO, result == 0);
    set_cpu_status_flag(NEGATIVE, result & (1 << NEGATIVE));
    return result;
}
unsigned char ASL(ExecutionInfo *exInfo)
{
    int operandAddr = exInfo->addressing_mode(get_pc() + 1);
    unsigned char operand = read_byte(operandAddr);
    set_cpu_status_flag(CARRY, operand & (1 << 7));
    operand <<= 1;
    set_cpu_status_flag(ZERO, operand == 0);
    set_cpu_status_flag(NEGATIVE, operand & (1 << NEGATIVE));
    write_byte(operandAddr, operand);
    return operand;
}

unsigned char BCC(ExecutionInfo *exInfo)
{
    int operandAddr = exInfo->addressing_mode(get_pc() + 1);
    if (!get_cpu_status_flag(CARRY))
    {
        int pc = get_pc() + 1;
        char offset = (char)read_byte(pc);
        int newPC = pc + offset;
        if (((pc + 1) & 0xFF00) != ((newPC + 1) & 0xFF00))
        {
            exInfo->clock_cycles += 1;
        }
        set_pc(newPC - 1);
        exInfo->clock_cycles += 1;
    }
    return 0;
}

unsigned char BCS(ExecutionInfo *exInfo)
{
    int operandAddr = exInfo->addressing_mode(get_pc() + 1);
    if (get_cpu_status_flag(CARRY))
    {
        int pc = get_pc() + 1;
        char offset = (char)read_byte(pc);
        int newPC = pc + offset;
        if (((pc + 1) & 0xFF00) != ((newPC + 1) & 0xFF00))
        {
            exInfo->clock_cycles += 1;
        }
        set_pc(newPC - 1);
        exInfo->clock_cycles += 1;
    }
    else
    {
    }
    return 0;
}

unsigned char BEQ(ExecutionInfo *exInfo)
{
    int operandAddr = exInfo->addressing_mode(get_pc() + 1);
    if (get_cpu_status_flag(ZERO))
    {
        int pc = get_pc() + 1;
        char offset = (char)read_byte(pc);
        int newPC = pc + offset;
        if (((pc + 1) & 0xFF00) != ((newPC + 1) & 0xFF00))
        {
            exInfo->clock_cycles += 1;
        }
        set_pc(newPC - 1);
        exInfo->clock_cycles += 1;
    }
    return 0;
}

unsigned char BIT(ExecutionInfo *exInfo)
{
    int operandAddr = exInfo->addressing_mode(get_pc() + 1);
    unsigned char memory = read_byte(operandAddr);
    unsigned char result = read_byte(get_cpu_accumulator()) & memory;
    set_cpu_status_flag(ZERO, result == 0);
    set_cpu_status_flag(CPU_OVERFLOW, memory & (1 << CPU_OVERFLOW));
    set_cpu_status_flag(NEGATIVE, memory & (1 << NEGATIVE));
    return result;
}
unsigned char BMI(ExecutionInfo *exInfo)
{
    int operandAddr = exInfo->addressing_mode(get_pc() + 1);
    if (get_cpu_status_flag(NEGATIVE))
    {
        int pc = get_pc() + 1;
        char offset = (char)read_byte(pc);
        int newPC = pc + offset;
        if (((pc + 1) & 0xFF00) != ((newPC + 1) & 0xFF00))
        {
            exInfo->clock_cycles += 1;
        }
        set_pc(newPC - 1);
        exInfo->clock_cycles += 1;
    }
    else
    {
    }
    return 0;
}

unsigned char BNE(ExecutionInfo *exInfo)
{
    int operandAddr = exInfo->addressing_mode(get_pc() + 1);
    if (!get_cpu_status_flag(ZERO))
    {
        int pc = get_pc() + 1;
        char offset = (char)read_byte(pc);
        int newPC = pc + offset;
        if (((pc + 1) & 0xFF00) != ((newPC + 1) & 0xFF00))
        {
            exInfo->clock_cycles += 1;
        }
        set_pc(newPC - 1);
        exInfo->clock_cycles += 1;
    }
    else
    {
    }
    return 0;
}

unsigned char BPL(ExecutionInfo *exInfo)
{
    int operandAddr = exInfo->addressing_mode(get_pc() + 1);
    if (!get_cpu_status_flag(NEGATIVE))
    {
        int pc = get_pc() + 1;
        char offset = (char)read_byte(pc);
        int newPC = pc + offset;
        if (((pc + 1) & 0xFF00) != ((newPC + 1) & 0xFF00))
        {
            exInfo->clock_cycles += 1;
        }
        set_pc(newPC - 1);
        exInfo->clock_cycles += 1;
    }
    else
    {
    }
    return 0;
}

unsigned char BVC(ExecutionInfo *exInfo)
{
    int operandAddr = exInfo->addressing_mode(get_pc() + 1);
    if (!get_cpu_status_flag(CPU_OVERFLOW))
    {
        int pc = get_pc() + 1;
        char offset = (char)read_byte(pc);
        int newPC = pc + offset;
        if (((pc + 1) & 0xFF00) != ((newPC + 1) & 0xFF00))
        {
            exInfo->clock_cycles += 1;
        }
        set_pc(newPC - 1);
        exInfo->clock_cycles += 1;
    }
    return 0;
}

unsigned char BVS(ExecutionInfo *exInfo)
{
    int operandAddr = exInfo->addressing_mode(get_pc() + 1);
    if (get_cpu_status_flag(CPU_OVERFLOW))
    {
        int pc = get_pc() + 1;
        char offset = (char)read_byte(pc);
        int newPC = pc + offset;
        if (((pc + 1) & 0xFF00) != ((newPC + 1) & 0xFF00))
        {
            exInfo->clock_cycles += 1;
        }
        set_pc(newPC - 1);
        exInfo->clock_cycles += 1;
    }
    return 0;
}

unsigned char CLC(ExecutionInfo *exInfo)
{
    int operandAddr = exInfo->addressing_mode(get_pc() + 1);
    set_cpu_status_flag(CARRY, false);
    return 0;
}
unsigned char CLD(ExecutionInfo *exInfo)
{
    int operandAddr = exInfo->addressing_mode(get_pc() + 1);
    set_cpu_status_flag(DECIMAL, false);
    return 0;
}
unsigned char CLI(ExecutionInfo *exInfo)
{
    int operandAddr = exInfo->addressing_mode(get_pc() + 1);
    pending_i_flag = 0; // clear interrupt flag after next instruction
    i_flag_delay = 1;   // apply after one more instruction completes
    return 0;
}
unsigned char CLV(ExecutionInfo *exInfo)
{
    int operandAddr = exInfo->addressing_mode(get_pc() + 1);
    set_cpu_status_flag(CPU_OVERFLOW, false);
    return 0;
}
unsigned char CMP(ExecutionInfo *exInfo)
{
    int operandAddr = exInfo->addressing_mode(get_pc() + 1);
    if (extra_page_cycle(get_pc() + 1, exInfo->addressing_mode, operandAddr))
    {
        exInfo->clock_cycles += 1; // extra cycle for page crossing
    }
    unsigned char A = read_byte(get_cpu_accumulator());
    unsigned char memory = read_byte(operandAddr);
    int result = A - memory;
    set_cpu_status_flag(CARRY, A >= memory);
    set_cpu_status_flag(ZERO, memory == A);
    set_cpu_status_flag(NEGATIVE, result & (1 << NEGATIVE));
    return result;
}
unsigned char CPX(ExecutionInfo *exInfo)
{
    int operandAddr = exInfo->addressing_mode(get_pc() + 1);
    unsigned char X = read_byte(get_cpu_x_register());
    unsigned char memory = read_byte(operandAddr);
    int result = X - memory;
    set_cpu_status_flag(CARRY, X >= memory);
    set_cpu_status_flag(ZERO, X == memory);
    set_cpu_status_flag(NEGATIVE, result & (1 << NEGATIVE));
    return result;
}
unsigned char CPY(ExecutionInfo *exInfo)
{
    int operandAddr = exInfo->addressing_mode(get_pc() + 1);
    unsigned char Y = read_byte(get_cpu_y_register());
    unsigned char memory = read_byte(operandAddr);
    int result = Y - memory;
    set_cpu_status_flag(CARRY, Y >= memory);
    set_cpu_status_flag(ZERO, Y == memory);
    set_cpu_status_flag(NEGATIVE, result & (1 << NEGATIVE));
    return result;
}

unsigned char DEC(ExecutionInfo *exInfo)
{
    int operandAddr = exInfo->addressing_mode(get_pc() + 1);
    unsigned char memory = read_byte(operandAddr);
    memory -= 1;
    write_byte(operandAddr, memory);
    set_cpu_status_flag(ZERO, memory == 0);
    set_cpu_status_flag(NEGATIVE, memory & (1 << NEGATIVE));
    return memory;
}
unsigned char DEX(ExecutionInfo *exInfo)
{
    int operandAddr = exInfo->addressing_mode(get_pc() + 1);
    unsigned char memory = read_byte(get_cpu_x_register());
    memory -= 1;
    write_byte(get_cpu_x_register(), memory);
    set_cpu_status_flag(ZERO, memory == 0);
    set_cpu_status_flag(NEGATIVE, memory & (1 << NEGATIVE));
    return memory;
}
unsigned char DEY(ExecutionInfo *exInfo)
{
    int operandAddr = exInfo->addressing_mode(get_pc() + 1);
    unsigned char memory = read_byte(get_cpu_y_register());
    memory -= 1;
    write_byte(get_cpu_y_register(), memory);
    set_cpu_status_flag(ZERO, memory == 0);
    set_cpu_status_flag(NEGATIVE, memory & (1 << NEGATIVE));

    return memory;
}

unsigned char EOR(ExecutionInfo *exInfo)
{
    int operandAddr = exInfo->addressing_mode(get_pc() + 1);
    if (extra_page_cycle(get_pc() + 1, exInfo->addressing_mode, operandAddr))
    {
        exInfo->clock_cycles += 1; // extra cycle for page crossing
    }
    unsigned char result = read_byte(get_cpu_accumulator()) ^ read_byte(operandAddr);
    write_byte(get_cpu_accumulator(), result);
    set_cpu_status_flag(ZERO, result == 0);
    set_cpu_status_flag(NEGATIVE, result & (1 << NEGATIVE));
    return result;
}

unsigned char INC(ExecutionInfo *exInfo)
{
    int operandAddr = exInfo->addressing_mode(get_pc() + 1);
    unsigned char result = read_byte(operandAddr);
    result += 1;
    write_byte(operandAddr, result);
    set_cpu_status_flag(ZERO, result == 0);
    set_cpu_status_flag(NEGATIVE, result & (1 << NEGATIVE));
    return result;
}
unsigned char INX(ExecutionInfo *exInfo)
{
    int operandAddr = exInfo->addressing_mode(get_pc() + 1);
    unsigned char result = read_byte(get_cpu_x_register());
    result += 1;
    write_byte(get_cpu_x_register(), result);
    set_cpu_status_flag(ZERO, result == 0);
    set_cpu_status_flag(NEGATIVE, result & (1 << NEGATIVE));
    return result;
}
unsigned char INY(ExecutionInfo *exInfo)
{
    int operandAddr = exInfo->addressing_mode(get_pc() + 1);
    unsigned char result = read_byte(get_cpu_y_register());
    result += 1;
    write_byte(get_cpu_y_register(), result);
    set_cpu_status_flag(ZERO, result == 0);
    set_cpu_status_flag(NEGATIVE, result & (1 << NEGATIVE));
    return result;
}

unsigned char LDA(ExecutionInfo *exInfo)
{
    int operandAddr = exInfo->addressing_mode(get_pc() + 1);
    if (extra_page_cycle(get_pc() + 1, exInfo->addressing_mode, operandAddr))
    {
        exInfo->clock_cycles += 1; // extra cycle for page crossing
    }
    unsigned char memory = read_byte(operandAddr);
    write_byte(get_cpu_accumulator(), memory);
    set_cpu_status_flag(ZERO, memory == 0);
    set_cpu_status_flag(NEGATIVE, memory & (1 << NEGATIVE));
    return memory;
}
unsigned char LDX(ExecutionInfo *exInfo)
{
    int operandAddr = exInfo->addressing_mode(get_pc() + 1);
    if (extra_page_cycle(get_pc() + 1, exInfo->addressing_mode, operandAddr))
    {
        exInfo->clock_cycles += 1; // extra cycle for page crossing
    }
    unsigned char memory = read_byte(operandAddr);
    write_byte(get_cpu_x_register(), memory);
    set_cpu_status_flag(ZERO, memory == 0);
    set_cpu_status_flag(NEGATIVE, memory & (1 << NEGATIVE));
    return memory;
}
unsigned char LDY(ExecutionInfo *exInfo)
{
    int operandAddr = exInfo->addressing_mode(get_pc() + 1);
    if (extra_page_cycle(get_pc() + 1, exInfo->addressing_mode, operandAddr))
    {
        exInfo->clock_cycles += 1; // extra cycle for page crossing
    }
    unsigned char memory = read_byte(operandAddr);
    write_byte(get_cpu_y_register(), memory);
    set_cpu_status_flag(ZERO, memory == 0);
    set_cpu_status_flag(NEGATIVE, memory & (1 << NEGATIVE));
    return memory;
}
unsigned char LSR(ExecutionInfo *exInfo)
{
    int operandAddr = exInfo->addressing_mode(get_pc() + 1);
    unsigned char operand = read_byte(operandAddr);
    set_cpu_status_flag(CARRY, operand & 1);
    operand >>= 1;
    set_cpu_status_flag(ZERO, operand == 0);
    set_cpu_status_flag(NEGATIVE, 0);
    write_byte(operandAddr, operand);
    return operand;
}

unsigned char NOP(ExecutionInfo *exInfo)
{
    int operandAddr = exInfo->addressing_mode(get_pc() + 1);
    return 0;
}

unsigned char ORA(ExecutionInfo *exInfo)
{
    int operandAddr = exInfo->addressing_mode(get_pc() + 1);
    if (extra_page_cycle(get_pc() + 1, exInfo->addressing_mode, operandAddr))
    {
        exInfo->clock_cycles += 1; // extra cycle for page crossing
    }
    unsigned char result = read_byte(get_cpu_accumulator()) | read_byte(operandAddr);
    write_byte(get_cpu_accumulator(), result);
    set_cpu_status_flag(ZERO, result == 0);
    set_cpu_status_flag(NEGATIVE, result & (1 << NEGATIVE));
    return result;
}

unsigned char PHA(ExecutionInfo *exInfo)
{
    int operandAddr = exInfo->addressing_mode(get_pc() + 1);
    unsigned char A = read_byte(get_cpu_accumulator());
    push_to_stack(A);
    return A;
}
unsigned char PHP(ExecutionInfo *exInfo)
{
    int operandAddr = exInfo->addressing_mode(get_pc() + 1);
    unsigned char SR = read_byte(get_cpu_status_register());
    // Set bits 4 and 5 before pushing
    SR |= 0x30; // Set both bit 5 (0x20) and bit 4 (0x10)
    push_to_stack(SR);
    return SR;
}
unsigned char PLA(ExecutionInfo *exInfo)
{
    int operandAddr = exInfo->addressing_mode(get_pc() + 1);
    unsigned char A = pop_from_stack();
    write_byte(get_cpu_accumulator(), A);
    set_cpu_status_flag(ZERO, A == 0);
    set_cpu_status_flag(NEGATIVE, A & (1 << NEGATIVE));
    return A;
}
unsigned char PLP(ExecutionInfo *exInfo)
{
    int operandAddr = exInfo->addressing_mode(get_pc() + 1);
    unsigned char SR = pop_from_stack();
    // Bit 5 is always 1, bit 4 is ignored (not a real flag)
    SR |= 0x20;  // Ensure bit 5 is set
    SR &= ~0x10; // Clear bit 4 (it's not a real flag)
    write_byte(get_cpu_status_register(), SR);
    return SR;
}

unsigned char ROL(ExecutionInfo *exInfo)
{
    int operandAddr = exInfo->addressing_mode(get_pc() + 1);
    unsigned char value = read_byte(operandAddr);
    unsigned char initialValue = value;
    unsigned char carryBit = get_cpu_status_flag(CARRY) ? 1 : 0;
    value = (value << 1) + carryBit;
    write_byte(operandAddr, value);
    set_cpu_status_flag(CARRY, initialValue & (1 << 7));
    set_cpu_status_flag(ZERO, value == 0);
    set_cpu_status_flag(NEGATIVE, value & (1 << NEGATIVE));
    return value;
}
unsigned char ROR(ExecutionInfo *exInfo)
{
    int operandAddr = exInfo->addressing_mode(get_pc() + 1);
    unsigned char value = read_byte(operandAddr);
    unsigned char initialValue = value;
    unsigned char carryBit = get_cpu_status_flag(CARRY) ? 1 : 0;
    value = (value >> 1) + (carryBit << 7);
    write_byte(operandAddr, value);
    set_cpu_status_flag(CARRY, initialValue & 1);
    set_cpu_status_flag(ZERO, value == 0);
    set_cpu_status_flag(NEGATIVE, value & (1 << NEGATIVE));
    return value;
}
unsigned char SBC(ExecutionInfo *exInfo)
{
    int operandAddr = exInfo->addressing_mode(get_pc() + 1);
    if (extra_page_cycle(get_pc() + 1, exInfo->addressing_mode, operandAddr))
    {
        exInfo->clock_cycles += 1; // extra cycle for page crossing
    }
    unsigned char memory = read_byte(operandAddr);
    unsigned char A = read_byte(get_cpu_accumulator());
    unsigned char C = get_cpu_status_flag(CARRY) ? 1 : 0;

    int tmp = A - memory - (1 - C);
    unsigned char result = (unsigned char)tmp;

    write_byte(get_cpu_accumulator(), result);

    set_cpu_status_flag(CARRY, tmp >= 0);
    set_cpu_status_flag(ZERO, result == 0);
    set_cpu_status_flag(CPU_OVERFLOW, (result ^ A) & (result ^ ~memory) & 0x80);
    set_cpu_status_flag(NEGATIVE, result >> NEGATIVE);
    return result;
}
unsigned char SEC(ExecutionInfo *exInfo)
{
    int operandAddr = exInfo->addressing_mode(get_pc() + 1);
    set_cpu_status_flag(CARRY, true);
    return 0;
}
unsigned char SED(ExecutionInfo *exInfo)
{
    int operandAddr = exInfo->addressing_mode(get_pc() + 1);
    set_cpu_status_flag(DECIMAL, true);
    return 0;
}
unsigned char SEI(ExecutionInfo *exInfo)
{
    int operandAddr = exInfo->addressing_mode(get_pc() + 1);
    pending_i_flag = 1; // set interrupt flag after next instruction
    i_flag_delay = 1;   // apply after one more instruction completes
    return 0;
}
unsigned char STA(ExecutionInfo *exInfo)
{
    int operandAddr = exInfo->addressing_mode(get_pc() + 1);
    unsigned char value = read_byte(get_cpu_accumulator());
    write_byte(operandAddr, value);
    return read_byte(get_cpu_accumulator());
}
unsigned char STX(ExecutionInfo *exInfo)
{
    int operandAddr = exInfo->addressing_mode(get_pc() + 1);
    write_byte(operandAddr, read_byte(get_cpu_x_register()));
    return read_byte(get_cpu_x_register());
}
unsigned char STY(ExecutionInfo *exInfo)
{
    int operandAddr = exInfo->addressing_mode(get_pc() + 1);
    write_byte(operandAddr, read_byte(get_cpu_y_register()));
    return read_byte(get_cpu_y_register());
}

unsigned char TAX(ExecutionInfo *exInfo)
{
    int operandAddr = exInfo->addressing_mode(get_pc() + 1);
    unsigned char A = read_byte(get_cpu_accumulator());
    write_byte(get_cpu_x_register(), A);
    set_cpu_status_flag(ZERO, A == 0);
    set_cpu_status_flag(NEGATIVE, A >> NEGATIVE);
    return A;
}
unsigned char TAY(ExecutionInfo *exInfo)
{
    int operandAddr = exInfo->addressing_mode(get_pc() + 1);
    unsigned char A = read_byte(get_cpu_accumulator());
    write_byte(get_cpu_y_register(), A);
    set_cpu_status_flag(ZERO, A == 0);
    set_cpu_status_flag(NEGATIVE, A >> NEGATIVE);
    return A;
}
unsigned char TSX(ExecutionInfo *exInfo)
{
    int operandAddr = exInfo->addressing_mode(get_pc() + 1);
    unsigned char SP = read_byte(get_cpu_stack());
    write_byte(get_cpu_x_register(), SP);
    set_cpu_status_flag(ZERO, SP == 0);
    set_cpu_status_flag(NEGATIVE, SP >> NEGATIVE);
    return SP;
}
unsigned char TXA(ExecutionInfo *exInfo)
{
    int operandAddr = exInfo->addressing_mode(get_pc() + 1);
    unsigned char X = read_byte(get_cpu_x_register());
    write_byte(get_cpu_accumulator(), X);
    set_cpu_status_flag(ZERO, X == 0);
    set_cpu_status_flag(NEGATIVE, X >> NEGATIVE);
    return X;
}
unsigned char TXS(ExecutionInfo *exInfo)
{
    int operandAddr = exInfo->addressing_mode(get_pc() + 1);
    unsigned char X = read_byte(get_cpu_x_register());
    write_byte(get_cpu_stack(), X);
    return X;
}
unsigned char TYA(ExecutionInfo *exInfo)
{
    int operandAddr = exInfo->addressing_mode(get_pc() + 1);
    unsigned char Y = read_byte(get_cpu_y_register());
    write_byte(get_cpu_accumulator(), Y);
    set_cpu_status_flag(ZERO, Y == 0);
    set_cpu_status_flag(NEGATIVE, Y >> NEGATIVE);
    return Y;
}

static bool pending_nmi = false;
static bool nmi_delayed = false;
static bool pending_irq = false;
void NMI()
{
    pending_nmi = true;
    nmi_delayed = false;
}

void trigger_delayed_nmi()
{
    pending_nmi = true;
    nmi_delayed = true;
}

void execute_nmi()
{
    static unsigned int last_ts;
    // printf("CPU DEBUG: Beginning execution of NMI, number of clock cycles since last NMI trigger: %ld\n", get_elapsed_clock_cycles() - last_ts);
    last_ts = get_elapsed_clock_cycles();
    int pc = get_pc();
    push_to_stack(pc >> 8);
    push_to_stack(pc & 0xFF);
    unsigned char p_copy = read_byte(get_cpu_status_register());
    unsigned char mask = 1;
    mask <<= 4;
    mask = ~mask;
    push_to_stack(p_copy & mask);
    int low = read_byte(0xFFFA);
    int high = ((int)read_byte(0xFFFB) << 8);
    set_pc(low + high);
    set_cpu_status_flag(INTERRUPT, true);
    pending_nmi = false;
}

bool pending_nmi_func()
{
    return pending_nmi && !nmi_delayed;
}

void trigger_irq()
{
    pending_irq = true;
}

bool pending_irq_func()
{
    return pending_irq && !get_cpu_status_flag(INTERRUPT);
}

void clear_pending_irq()
{
    pending_irq = false;
}

void execute_irq()
{
    int pc = get_pc();
    push_to_stack(pc >> 8);
    push_to_stack(pc & 0xFF);
    unsigned char p_copy = read_byte(get_cpu_status_register());
    unsigned char mask = 1;
    mask <<= 4;
    mask = ~mask;
    push_to_stack(p_copy & mask);
    int low = read_byte(0xFFFE);
    int high = ((int)read_byte(0xFFFF) << 8);
    set_pc(low + high);
    set_cpu_status_flag(INTERRUPT, true);
    pending_irq = false;
}

void cpu_instruction_completed()
{
    nmi_delayed = false;
    // Apply delayed interrupt flag change after one instruction delay
    if (i_flag_delay > 0)
    {
        i_flag_delay--;
        if (i_flag_delay == 0 && pending_i_flag != -1)
        {
            set_cpu_status_flag(INTERRUPT, pending_i_flag == 1);
            pending_i_flag = -1;
        }
    }
}

unsigned char JMP(ExecutionInfo *exInfo)
{
    int operandAddr = exInfo->addressing_mode(get_pc() + 1);
    int newPC = read_byte(get_pc() + 1) + ((int)read_byte(get_pc() + 2) << 8);
    if (last_instruction.addressing_mode == ABS_IND)
    {
        int secondAddr = newPC;
        if ((secondAddr & 0xFF) == 0xFF)
            secondAddr -= 0xFF;
        else
            secondAddr += 1;
        newPC = read_byte(newPC) + ((int)read_byte(secondAddr) << 8);
    }
    set_pc(newPC);
    return 0;
}
unsigned char JSR(ExecutionInfo *exInfo)
{
    int operandAddr = exInfo->addressing_mode(get_pc() + 1);
    int newPC = read_byte(get_pc() + 1) + ((int)read_byte(get_pc() + 2) << 8);
    int pc = get_pc() + 2;
    push_to_stack(pc >> 8);
    push_to_stack(pc & 0xFF);
    set_pc(newPC);
    return 0;
}
unsigned char RTI(ExecutionInfo *exInfo)
{
    int operandAddr = exInfo->addressing_mode(get_pc() + 1);
    unsigned char status = pop_from_stack();
    unsigned char pcLow = pop_from_stack();
    unsigned char pcHigh = pop_from_stack();
    write_byte(get_cpu_status_register(), status);
    set_pc(pcLow + ((int)pcHigh << 8));
    return 0;
}
unsigned char RTS(ExecutionInfo *exInfo)
{
    int operandAddr = exInfo->addressing_mode(get_pc() + 1);
    unsigned char pcLow = pop_from_stack();
    unsigned char pcHigh = pop_from_stack();
    set_pc(pcLow + ((int)pcHigh << 8) + 1);
    return 0;
}

unsigned char BRK(ExecutionInfo *exInfo)
{
    int operandAddr = exInfo->addressing_mode(get_pc() + 1);
    int pc = get_pc() + 2;
    push_to_stack(pc >> 8);
    push_to_stack(pc & 0xFF);
    set_cpu_status_flag(4, true);
    set_cpu_status_flag(2, true);
    set_cpu_status_flag(5, true);
    push_to_stack(read_byte(get_cpu_status_register()));
    set_pc(read_byte(0xFFFE) + ((int)read_byte(0xFFFE + 1) << 8));
    return 0;
}

// Shout out claude, I am not writing this table by hand
static ExecutionInfo lookup_table[16][16] = {
    // Row 0: 0x00 - 0x0F
    {
        {IMP, BRK, 0, 7},         // 0x00 BRK
        {ZP_INDX_IND, ORA, 2, 6}, // 0x01 ORA (zp,x)
        {IMP, NOP, 2, 2},         // 0x02 *KIL (treat as 1-byte NOP that halts - changed cycles)
        {IMP, NOP, 2, 8},         // 0x03 *SLO (zp,x) - unofficial
        {IMP, NOP, 2, 3},         // 0x04 *NOP zp - unofficial
        {ZP, ORA, 2, 3},          // 0x05 ORA zp
        {ZP, ASL, 2, 5},          // 0x06 ASL zp
        {IMP, NOP, 2, 5},         // 0x07 *SLO zp - unofficial
        {IMP, PHP, 1, 3},         // 0x08 PHP
        {IMM, ORA, 2, 2},         // 0x09 ORA #
        {ACC, ASL, 1, 2},         // 0x0A ASL A
        {IMP, NOP, 2, 2},         // 0x0B *ANC # - unofficial
        {IMP, NOP, 3, 4},         // 0x0C *NOP a - unofficial
        {ABS_A, ORA, 3, 4},       // 0x0D ORA a
        {ABS_A, ASL, 3, 6},       // 0x0E ASL a
        {IMP, NOP, 3, 6},         // 0x0F *SLO a - unofficial
    },
    // Row 1: 0x10 - 0x1F
    {
        {PCR, BPL, 2, 2},           // 0x10 BPL
        {ZP_IND_INDX_Y, ORA, 2, 5}, // 0x11 ORA (zp),y
        {ZP_IND, ORA, 2, 5},        // 0x12 ORA (zp)
        {IMP, NOP, 2, 8},           // 0x13 *SLO (zp),y - unofficial
        {IMP, NOP, 2, 4},           // 0x14 *NOP zp,x - unofficial
        {ZP_INDX_X, ORA, 2, 4},     // 0x15 ORA zp,x
        {ZP_INDX_X, ASL, 2, 6},     // 0x16 ASL zp,x
        {IMP, NOP, 2, 6},           // 0x17 *SLO zp,x - unofficial
        {IMP, CLC, 1, 2},           // 0x18 CLC
        {ABS_INDEX_Y, ORA, 3, 4},   // 0x19 ORA a,y
        {ACC, INC, 1, 2},           // 0x1A INC A
        {IMP, NOP, 3, 7},           // 0x1B *SLO a,y - unofficial
        {IMP, NOP, 3, 4},           // 0x1C *NOP a,x - unofficial
        {ABS_INDEX_X, ORA, 3, 4},   // 0x1D ORA a,x
        {ABS_INDEX_X, ASL, 3, 7},   // 0x1E ASL a,x
        {IMP, NOP, 3, 7},           // 0x1F *SLO a,x - unofficial
    },
    // Row 2: 0x20 - 0x2F
    {
        {ABS_A, JSR, 0, 6},       // 0x20 JSR
        {ZP_INDX_IND, AND, 2, 6}, // 0x21 AND (zp,x)
        {IMP, NOP, 1, 2},         // 0x22 *KIL - unofficial
        {IMP, NOP, 2, 8},         // 0x23 *RLA (zp,x) - unofficial
        {ZP, BIT, 2, 3},          // 0x24 BIT zp
        {ZP, AND, 2, 3},          // 0x25 AND zp
        {ZP, ROL, 2, 5},          // 0x26 ROL zp
        {IMP, NOP, 2, 5},         // 0x27 *RLA zp - unofficial
        {IMP, PLP, 1, 4},         // 0x28 PLP
        {IMM, AND, 2, 2},         // 0x29 AND #
        {ACC, ROL, 1, 2},         // 0x2A ROL A
        {IMP, NOP, 2, 2},         // 0x2B *ANC # - unofficial
        {ABS_A, BIT, 3, 4},       // 0x2C BIT a
        {ABS_A, AND, 3, 4},       // 0x2D AND a
        {ABS_A, ROL, 3, 6},       // 0x2E ROL a
        {IMP, NOP, 3, 6},         // 0x2F *RLA a - unofficial
    },
    // Row 3: 0x30 - 0x3F
    {
        {PCR, BMI, 2, 2},           // 0x30 BMI
        {ZP_IND_INDX_Y, AND, 2, 5}, // 0x31 AND (zp),y
        {ZP_IND, AND, 2, 5},        // 0x32 AND (zp)
        {IMP, NOP, 2, 8},           // 0x33 *RLA (zp),y - unofficial
        {ZP_INDX_X, BIT, 2, 4},     // 0x34 BIT zp,x
        {ZP_INDX_X, AND, 2, 4},     // 0x35 AND zp,x
        {ZP_INDX_X, ROL, 2, 6},     // 0x36 ROL zp,x
        {IMP, NOP, 2, 6},           // 0x37 *RLA zp,x - unofficial
        {IMP, SEC, 1, 2},           // 0x38 SEC
        {ABS_INDEX_Y, AND, 3, 4},   // 0x39 AND a,y
        {ACC, DEC, 1, 2},           // 0x3A DEC A
        {IMP, NOP, 3, 7},           // 0x3B *RLA a,y - unofficial
        {ABS_INDEX_X, BIT, 3, 4},   // 0x3C BIT a,x
        {ABS_INDEX_X, AND, 3, 4},   // 0x3D AND a,x
        {ABS_INDEX_X, ROL, 3, 7},   // 0x3E ROL a,x
        {IMP, NOP, 3, 7},           // 0x3F *RLA a,x - unofficial
    },
    // Row 4: 0x40 - 0x4F
    {
        {IMP, RTI, 0, 6},         // 0x40 RTI
        {ZP_INDX_IND, EOR, 2, 6}, // 0x41 EOR (zp,x)
        {IMP, NOP, 1, 2},         // 0x42 *KIL - unofficial
        {IMP, NOP, 2, 8},         // 0x43 *SRE (zp,x) - unofficial
        {IMP, NOP, 2, 3},         // 0x44 *NOP zp - unofficial
        {ZP, EOR, 2, 3},          // 0x45 EOR zp
        {ZP, LSR, 2, 5},          // 0x46 LSR zp
        {IMP, NOP, 2, 5},         // 0x47 *SRE zp - unofficial
        {IMP, PHA, 1, 3},         // 0x48 PHA
        {IMM, EOR, 2, 2},         // 0x49 EOR #
        {ACC, LSR, 1, 2},         // 0x4A LSR A
        {IMP, NOP, 2, 2},         // 0x4B *ALR # - unofficial
        {ABS_A, JMP, 0, 3},       // 0x4C JMP a
        {ABS_A, EOR, 3, 4},       // 0x4D EOR a
        {ABS_A, LSR, 3, 6},       // 0x4E LSR a
        {IMP, NOP, 3, 6},         // 0x4F *SRE a - unofficial
    },
    // Row 5: 0x50 - 0x5F
    {
        {PCR, BVC, 2, 2},           // 0x50 BVC
        {ZP_IND_INDX_Y, EOR, 2, 5}, // 0x51 EOR (zp),y
        {ZP_IND, EOR, 2, 5},        // 0x52 EOR (zp)
        {IMP, NOP, 2, 8},           // 0x53 *SRE (zp),y - unofficial
        {IMP, NOP, 2, 4},           // 0x54 *NOP zp,x - unofficial
        {ZP_INDX_X, EOR, 2, 4},     // 0x55 EOR zp,x
        {ZP_INDX_X, LSR, 2, 6},     // 0x56 LSR zp,x
        {IMP, NOP, 2, 6},           // 0x57 *SRE zp,x - unofficial
        {IMP, CLI, 1, 2},           // 0x58 CLI
        {ABS_INDEX_Y, EOR, 3, 4},   // 0x59 EOR a,y
        {IMP, NOP, 1, 2},           // 0x5A *NOP - unofficial
        {IMP, NOP, 3, 7},           // 0x5B *SRE a,y - unofficial
        {IMP, NOP, 3, 4},           // 0x5C *NOP a,x - unofficial
        {ABS_INDEX_X, EOR, 3, 4},   // 0x5D EOR a,x
        {ABS_INDEX_X, LSR, 3, 7},   // 0x5E LSR a,x
        {IMP, NOP, 3, 7},           // 0x5F *SRE a,x - unofficial
    },
    // Row 6: 0x60 - 0x6F
    {
        {IMP, RTS, 0, 6},         // 0x60 RTS
        {ZP_INDX_IND, ADC, 2, 6}, // 0x61 ADC (zp,x)
        {IMP, NOP, 1, 2},         // 0x62 *KIL - unofficial
        {IMP, NOP, 2, 8},         // 0x63 *RRA (zp,x) - unofficial
        {IMP, NOP, 2, 3},         // 0x64 *NOP zp - unofficial
        {ZP, ADC, 2, 3},          // 0x65 ADC zp
        {ZP, ROR, 2, 5},          // 0x66 ROR zp
        {IMP, NOP, 2, 5},         // 0x67 *RRA zp - unofficial
        {IMP, PLA, 1, 4},         // 0x68 PLA
        {IMM, ADC, 2, 2},         // 0x69 ADC #
        {ACC, ROR, 1, 2},         // 0x6A ROR A
        {IMP, NOP, 2, 2},         // 0x6B *ARR # - unofficial
        {ABS_IND, JMP, 0, 5},     // 0x6C JMP (a)
        {ABS_A, ADC, 3, 4},       // 0x6D ADC a
        {ABS_A, ROR, 3, 6},       // 0x6E ROR a
        {IMP, NOP, 3, 6},         // 0x6F *RRA a - unofficial
    },
    // Row 7: 0x70 - 0x7F
    {
        {PCR, BVS, 2, 2},           // 0x70 BVS
        {ZP_IND_INDX_Y, ADC, 2, 5}, // 0x71 ADC (zp),y
        {ZP_IND, ADC, 2, 5},        // 0x72 ADC (zp)
        {IMP, NOP, 2, 8},           // 0x73 *RRA (zp),y - unofficial
        {IMP, NOP, 2, 4},           // 0x74 *NOP zp,x - unofficial
        {ZP_INDX_X, ADC, 2, 4},     // 0x75 ADC zp,x
        {ZP_INDX_X, ROR, 2, 6},     // 0x76 ROR zp,x
        {IMP, NOP, 2, 6},           // 0x77 *RRA zp,x - unofficial
        {IMP, SEI, 1, 2},           // 0x78 SEI
        {ABS_INDEX_Y, ADC, 3, 4},   // 0x79 ADC a,y
        {IMP, NOP, 1, 2},           // 0x7A *NOP - unofficial
        {IMP, NOP, 3, 7},           // 0x7B *RRA a,y - unofficial
        {IMP, NOP, 3, 4},           // 0x7C *NOP a,x - unofficial
        {ABS_INDEX_X, ADC, 3, 4},   // 0x7D ADC a,x
        {ABS_INDEX_X, ROR, 3, 7},   // 0x7E ROR a,x
        {IMP, NOP, 3, 7},           // 0x7F *RRA a,x - unofficial
    },
    // Row 8: 0x80 - 0x8F
    {
        {IMP, NOP, 2, 2},         // 0x80 *NOP # - unofficial
        {ZP_INDX_IND, STA, 2, 6}, // 0x81 STA (zp,x)
        {IMP, NOP, 2, 2},         // 0x82 *NOP # - unofficial
        {IMP, NOP, 2, 6},         // 0x83 *SAX (zp,x) - unofficial
        {ZP, STY, 2, 3},          // 0x84 STY zp
        {ZP, STA, 2, 3},          // 0x85 STA zp
        {ZP, STX, 2, 3},          // 0x86 STX zp
        {IMP, NOP, 2, 3},         // 0x87 *SAX zp - unofficial
        {IMP, DEY, 1, 2},         // 0x88 DEY
        {IMP, NOP, 2, 2},         // 0x89 *NOP # - unofficial
        {IMP, TXA, 1, 2},         // 0x8A TXA
        {IMP, NOP, 2, 2},         // 0x8B *XAA # - unofficial (unstable)
        {ABS_A, STY, 3, 4},       // 0x8C STY a
        {ABS_A, STA, 3, 4},       // 0x8D STA a
        {ABS_A, STX, 3, 4},       // 0x8E STX a
        {IMP, NOP, 3, 4},         // 0x8F *SAX a - unofficial
    },
    // Row 9: 0x90 - 0x9F
    {
        {PCR, BCC, 2, 2},           // 0x90 BCC
        {ZP_IND_INDX_Y, STA, 2, 6}, // 0x91 STA (zp),y
        {ZP_IND, STA, 2, 5},        // 0x92 STA (zp)
        {IMP, NOP, 2, 6},           // 0x93 *AHX (zp),y - unofficial (unstable)
        {ZP_INDX_X, STY, 2, 4},     // 0x94 STY zp,x
        {ZP_INDX_X, STA, 2, 4},     // 0x95 STA zp,x
        {ZP_INDX_Y, STX, 2, 4},     // 0x96 STX zp,y
        {IMP, NOP, 2, 4},           // 0x97 *SAX zp,y - unofficial
        {IMP, TYA, 1, 2},           // 0x98 TYA
        {ABS_INDEX_Y, STA, 3, 5},   // 0x99 STA a,y
        {IMP, TXS, 1, 2},           // 0x9A TXS
        {IMP, NOP, 3, 5},           // 0x9B *TAS a,y - unofficial (unstable)
        {IMP, NOP, 3, 5},           // 0x9C *SHY a,x - unofficial (unstable)
        {ABS_INDEX_X, STA, 3, 5},   // 0x9D STA a,x
        {IMP, NOP, 3, 5},           // 0x9E *SHX a,y - unofficial (unstable)
        {IMP, NOP, 3, 5},           // 0x9F *AHX a,y - unofficial (unstable)
    },
    // Row A: 0xA0 - 0xAF
    {
        {IMM, LDY, 2, 2},         // 0xA0 LDY #
        {ZP_INDX_IND, LDA, 2, 6}, // 0xA1 LDA (zp,x)
        {IMM, LDX, 2, 2},         // 0xA2 LDX #
        {IMP, NOP, 2, 6},         // 0xA3 *LAX (zp,x) - unofficial
        {ZP, LDY, 2, 3},          // 0xA4 LDY zp
        {ZP, LDA, 2, 3},          // 0xA5 LDA zp
        {ZP, LDX, 2, 3},          // 0xA6 LDX zp
        {IMP, NOP, 2, 3},         // 0xA7 *LAX zp - unofficial
        {IMP, TAY, 1, 2},         // 0xA8 TAY
        {IMM, LDA, 2, 2},         // 0xA9 LDA #
        {IMP, TAX, 1, 2},         // 0xAA TAX
        {IMP, NOP, 2, 2},         // 0xAB *LAX # - unofficial (unstable)
        {ABS_A, LDY, 3, 4},       // 0xAC LDY a
        {ABS_A, LDA, 3, 4},       // 0xAD LDA a
        {ABS_A, LDX, 3, 4},       // 0xAE LDX a
        {IMP, NOP, 3, 4},         // 0xAF *LAX a - unofficial
    },
    // Row B: 0xB0 - 0xBF
    {
        {PCR, BCS, 2, 2},           // 0xB0 BCS
        {ZP_IND_INDX_Y, LDA, 2, 5}, // 0xB1 LDA (zp),y
        {ZP_IND, LDA, 2, 5},        // 0xB2 LDA (zp)
        {IMP, NOP, 2, 5},           // 0xB3 *LAX (zp),y - unofficial
        {ZP_INDX_X, LDY, 2, 4},     // 0xB4 LDY zp,x
        {ZP_INDX_X, LDA, 2, 4},     // 0xB5 LDA zp,x
        {ZP_INDX_Y, LDX, 2, 4},     // 0xB6 LDX zp,y
        {IMP, NOP, 2, 4},           // 0xB7 *LAX zp,y - unofficial
        {IMP, CLV, 1, 2},           // 0xB8 CLV
        {ABS_INDEX_Y, LDA, 3, 4},   // 0xB9 LDA a,y
        {IMP, TSX, 1, 2},           // 0xBA TSX
        {IMP, NOP, 3, 4},           // 0xBB *LAS a,y - unofficial
        {ABS_INDEX_X, LDY, 3, 4},   // 0xBC LDY a,x
        {ABS_INDEX_X, LDA, 3, 4},   // 0xBD LDA a,x
        {ABS_INDEX_Y, LDX, 3, 4},   // 0xBE LDX a,y
        {IMP, NOP, 3, 4},           // 0xBF *LAX a,y - unofficial
    },
    // Row C: 0xC0 - 0xCF
    {
        {IMM, CPY, 2, 2},         // 0xC0 CPY #
        {ZP_INDX_IND, CMP, 2, 6}, // 0xC1 CMP (zp,x)
        {IMP, NOP, 2, 2},         // 0xC2 *NOP # - unofficial
        {IMP, NOP, 2, 8},         // 0xC3 *DCP (zp,x) - unofficial
        {ZP, CPY, 2, 3},          // 0xC4 CPY zp
        {ZP, CMP, 2, 3},          // 0xC5 CMP zp
        {ZP, DEC, 2, 5},          // 0xC6 DEC zp
        {IMP, NOP, 2, 5},         // 0xC7 *DCP zp - unofficial
        {IMP, INY, 1, 2},         // 0xC8 INY
        {IMM, CMP, 2, 2},         // 0xC9 CMP #
        {IMP, DEX, 1, 2},         // 0xCA DEX
        {IMP, NOP, 2, 2},         // 0xCB *AXS # - unofficial
        {ABS_A, CPY, 3, 4},       // 0xCC CPY a
        {ABS_A, CMP, 3, 4},       // 0xCD CMP a
        {ABS_A, DEC, 3, 6},       // 0xCE DEC a
        {IMP, NOP, 3, 6},         // 0xCF *DCP a - unofficial
    },
    // Row D: 0xD0 - 0xDF
    {
        {PCR, BNE, 2, 2},           // 0xD0 BNE
        {ZP_IND_INDX_Y, CMP, 2, 5}, // 0xD1 CMP (zp),y
        {ZP_IND, CMP, 2, 5},        // 0xD2 CMP (zp)
        {IMP, NOP, 2, 8},           // 0xD3 *DCP (zp),y - unofficial
        {IMP, NOP, 2, 4},           // 0xD4 *NOP zp,x - unofficial
        {ZP_INDX_X, CMP, 2, 4},     // 0xD5 CMP zp,x
        {ZP_INDX_X, DEC, 2, 6},     // 0xD6 DEC zp,x
        {IMP, NOP, 2, 6},           // 0xD7 *DCP zp,x - unofficial
        {IMP, CLD, 1, 2},           // 0xD8 CLD
        {ABS_INDEX_Y, CMP, 3, 4},   // 0xD9 CMP a,y
        {IMP, NOP, 1, 2},           // 0xDA *NOP - unofficial
        {IMP, NOP, 3, 7},           // 0xDB *DCP a,y - unofficial
        {IMP, NOP, 3, 4},           // 0xDC *NOP a,x - unofficial
        {ABS_INDEX_X, CMP, 3, 4},   // 0xDD CMP a,x
        {ABS_INDEX_X, DEC, 3, 7},   // 0xDE DEC a,x
        {IMP, NOP, 3, 7},           // 0xDF *DCP a,x - unofficial
    },
    // Row E: 0xE0 - 0xEF
    {
        {IMM, CPX, 2, 2},         // 0xE0 CPX #
        {ZP_INDX_IND, SBC, 2, 6}, // 0xE1 SBC (zp,x)
        {IMP, NOP, 2, 2},         // 0xE2 *NOP # - unofficial
        {IMP, NOP, 2, 8},         // 0xE3 *ISC (zp,x) - unofficial
        {ZP, CPX, 2, 3},          // 0xE4 CPX zp
        {ZP, SBC, 2, 3},          // 0xE5 SBC zp
        {ZP, INC, 2, 5},          // 0xE6 INC zp
        {IMP, NOP, 2, 5},         // 0xE7 *ISC zp - unofficial
        {IMP, INX, 1, 2},         // 0xE8 INX
        {IMM, SBC, 2, 2},         // 0xE9 SBC #
        {IMP, NOP, 1, 2},         // 0xEA NOP (official)
        {IMP, NOP, 2, 2},         // 0xEB *SBC # - unofficial
        {ABS_A, CPX, 3, 4},       // 0xEC CPX a
        {ABS_A, SBC, 3, 4},       // 0xED SBC a
        {ABS_A, INC, 3, 6},       // 0xEE INC a
        {IMP, NOP, 3, 6},         // 0xEF *ISC a - unofficial
    },
    // Row F: 0xF0 - 0xFF
    {
        {PCR, BEQ, 2, 2},           // 0xF0 BEQ
        {ZP_IND_INDX_Y, SBC, 2, 5}, // 0xF1 SBC (zp),y
        {ZP_IND, SBC, 2, 5},        // 0xF2 SBC (zp)
        {IMP, NOP, 2, 8},           // 0xF3 *ISC (zp),y - unofficial
        {IMP, NOP, 2, 4},           // 0xF4 *NOP zp,x - unofficial
        {ZP_INDX_X, SBC, 2, 4},     // 0xF5 SBC zp,x
        {ZP_INDX_X, INC, 2, 6},     // 0xF6 INC zp,x
        {IMP, NOP, 2, 6},           // 0xF7 *ISC zp,x - unofficial
        {IMP, SED, 1, 2},           // 0xF8 SED
        {ABS_INDEX_Y, SBC, 3, 4},   // 0xF9 SBC a,y
        {IMP, NOP, 1, 2},           // 0xFA *NOP - unofficial
        {IMP, NOP, 3, 7},           // 0xFB *ISC a,y - unofficial
        {IMP, NOP, 3, 4},           // 0xFC *NOP a,x - unofficial
        {ABS_INDEX_X, SBC, 3, 4},   // 0xFD SBC a,x
        {ABS_INDEX_X, INC, 3, 7},   // 0xFE INC a,x
        {IMP, NOP, 3, 7},           // 0xFF *ISC a,x - unofficial
    }};

ExecutionInfo get_execution_info(unsigned char opCode)
{
    unsigned char lower = opCode & 0b00001111;
    unsigned char upper = opCode >> 4;
    last_instruction = lookup_table[upper][lower];
    return last_instruction;
}

static const char *instr_name(unsigned char (*exec)(ExecutionInfo *))
{
    if (exec == ADC) return "ADC";
    if (exec == AND) return "AND";
    if (exec == ASL) return "ASL";
    if (exec == BCC) return "BCC";
    if (exec == BCS) return "BCS";
    if (exec == BEQ) return "BEQ";
    if (exec == BIT) return "BIT";
    if (exec == BMI) return "BMI";
    if (exec == BNE) return "BNE";
    if (exec == BPL) return "BPL";
    if (exec == BVC) return "BVC";
    if (exec == BRK) return "BRK";
    if (exec == BVS) return "BVS";
    if (exec == CLC) return "CLC";
    if (exec == CLD) return "CLD";
    if (exec == CLI) return "CLI";
    if (exec == CLV) return "CLV";
    if (exec == CMP) return "CMP";
    if (exec == CPX) return "CPX";
    if (exec == CPY) return "CPY";
    if (exec == DEC) return "DEC";
    if (exec == DEX) return "DEX";
    if (exec == DEY) return "DEY";
    if (exec == EOR) return "EOR";
    if (exec == INC) return "INC";
    if (exec == INX) return "INX";
    if (exec == INY) return "INY";
    if (exec == JMP) return "JMP";
    if (exec == JSR) return "JSR";
    if (exec == LDA) return "LDA";
    if (exec == LDX) return "LDX";
    if (exec == LDY) return "LDY";
    if (exec == LSR) return "LSR";
    if (exec == NOP) return "NOP";
    if (exec == ORA) return "ORA";
    if (exec == PHA) return "PHA";
    if (exec == PHP) return "PHP";
    if (exec == PLA) return "PLA";
    if (exec == PLP) return "PLP";
    if (exec == ROL) return "ROL";
    if (exec == ROR) return "ROR";
    if (exec == RTI) return "RTI";
    if (exec == RTS) return "RTS";
    if (exec == SBC) return "SBC";
    if (exec == SEC) return "SEC";
    if (exec == SED) return "SED";
    if (exec == SEI) return "SEI";
    if (exec == STA) return "STA";
    if (exec == STX) return "STX";
    if (exec == STY) return "STY";
    if (exec == TAX) return "TAX";
    if (exec == TAY) return "TAY";
    if (exec == TSX) return "TSX";
    if (exec == TXA) return "TXA";
    if (exec == TXS) return "TXS";
    if (exec == TYA) return "TYA";
    return "???";
}

static const char *am_name(int (*am)(int))
{
    if (am == ABS_A) return "ABS";
    if (am == ABS_INDX_IND) return "ABS_INDX";
    if (am == ABS_INDEX_X) return "ABS,X";
    if (am == ABS_INDEX_Y) return "ABS,Y";
    if (am == ABS_IND) return "ABS_IND";
    if (am == ACC) return "A";
    if (am == IMM) return "IMM";
    if (am == IMP) return "IMPL";
    if (am == PCR) return "REL";
    if (am == STK) return "STK";
    if (am == ZP) return "ZP";
    if (am == ZP_INDX_IND) return "(ZP,X)";
    if (am == ZP_INDX_X) return "ZP,X";
    if (am == ZP_INDX_Y) return "ZP,Y";
    if (am == ZP_IND) return "(ZP)";
    if (am == ZP_IND_INDX_Y) return "(ZP),Y";
    return "???";
}

void disassemble_opcode(unsigned char opcode, const char **mnemonic, const char **addrmode)
{
    ExecutionInfo info = get_execution_info(opcode);
    *mnemonic = instr_name(info.executor);
    *addrmode = am_name(info.addressing_mode);
}