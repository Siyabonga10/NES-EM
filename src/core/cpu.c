#include "cpu.h"
#include "instructions.h"
#include "bus.h"
#include "ppu.h"
#include "registerOffsets.h"
#include "addressing_modes.h"
#include "ControllerKeyStates.h"
#include "frameData.h"
#include "controller.h"
#include <string.h>
#include <assert.h>
#include <stdio.h>
#define PPU_TICKS_PER_CPU_CYCLE 3

static int PC = 0xFFFC; // starting point of execution
static const int WRAM_SIZE = 0x800;
static unsigned char *cpu_mem;
static bool can_execute_next_instruction = false;
static int remaining_clock_cycles = 0;
static unsigned int elapsed_clock_cycles = 0;

void do_single_tick_and_check_for_nmi(ControllerKeyStates *keyStates)
{
    ppu_tick();
    if (pending_nmi_func())
    {
        update_controller_input(keyStates);
    }
}

void ppu_tick_callback(ControllerKeyStates *keyStates)
{

    for (int i = 0; i < PPU_TICKS_PER_CPU_CYCLE; i++)
        do_single_tick_and_check_for_nmi(keyStates);
}
static int some_counter;
FrameData *tick_cpu(ControllerKeyStates *keyStates)
{
    while (true)
    {
        FrameData *frame = request_frame();
        if (frame->is_new_frame)
            return frame;
        update_controller_input(keyStates);

        if (is_dma_active())
        {
            update_dma_cycles();
            elapsed_clock_cycles += 1;
            ppu_tick_callback(keyStates);
            continue;
        }

        if (can_execute_next_instruction && pending_nmi_func())
        {
            update_controller_input(keyStates);
            execute_nmi();
            can_execute_next_instruction = false;
            elapsed_clock_cycles += 1;
            remaining_clock_cycles = 7;
        }
        if (can_execute_next_instruction && pending_irq_func())
        {
            elapsed_clock_cycles += 1;
            update_controller_input(keyStates);
            execute_irq();
            can_execute_next_instruction = false;
            remaining_clock_cycles = 7;
        }

        if (can_execute_next_instruction)
        {
            elapsed_clock_cycles += 1;
            ExecutionInfo instr = get_next_instruction();
            // if (read_byte(PC) == 0x4c && read_byte(PC + 1) == 0x00 && read_byte(PC + 2) == 0x02) // 4C 00 02
            //     asm("int3");
            remaining_clock_cycles = execute_instruction(instr) - 1;
            can_execute_next_instruction = remaining_clock_cycles <= 0;
            ppu_tick_callback(keyStates);
        }
        else
        {
            elapsed_clock_cycles += 1;
            remaining_clock_cycles--;
            if (remaining_clock_cycles <= 0)
            {
                cpu_instruction_completed();
                ppu_tick_callback(keyStates);
                can_execute_next_instruction = true;
            }
            else
            {
                ppu_tick_callback(keyStates);
            }
        }
    }
}

ExecutionInfo get_next_instruction()
{
    ExecutionInfo next_instruction_var = get_execution_info(read_byte(PC));
    return next_instruction_var;
}

void shutdown_cpu()
{
    free(cpu_mem);
}

int execute_instruction(ExecutionInfo exInfo)
{
    exInfo.executor(&exInfo);
    PC += exInfo.instruction_size;
    return exInfo.clock_cycles;
}

static int status_flag_getter(int index)
{
    assert(index < 8);
    unsigned char status_register = cpu_mem[STATUS_REGISTER_ADDR];
    bool val = (status_register & (1 << index)) ? 1 : 0;
    return val;
}

static void status_flag_setter(int index, bool value)
{
    assert(index < 8);
    if (value)
        cpu_mem[STATUS_REGISTER_ADDR] |= (1 << index);
    else
        cpu_mem[STATUS_REGISTER_ADDR] &= ~(1 << index);
}
static int pc_getter()
{
    return PC;
}
static void pc_setter(int newPC)
{
    PC = newPC;
}
static void cpu_stack_push(unsigned char byte)
{
    cpu_mem[NO_OF_REGISTERS + 0x100 + cpu_mem[STACK_ADDR]] = byte;
    cpu_mem[STACK_ADDR] -= 1;
}
static unsigned char cpu_stack_pop()
{
    cpu_mem[STACK_ADDR] += 1;
    unsigned char byte = cpu_mem[NO_OF_REGISTERS + 0x100 + cpu_mem[STACK_ADDR]];
    return byte;
}

unsigned char cpu_read(int addr)
{
    if (addr < 0x2000)
        return cpu_mem[NO_OF_REGISTERS + (addr % 0x800)];
    else if (addr >= REGISTER_OFFSET)
        return cpu_mem[addr - REGISTER_OFFSET];
    return 0;
}

void cpu_write(int addr, unsigned char value)
{
    if (addr < 0x2000)
        cpu_mem[NO_OF_REGISTERS + (addr % 0x800)] = value;
    else if (addr >= REGISTER_OFFSET)
        cpu_mem[addr - REGISTER_OFFSET] = value;
}

int get_clock_cycles()
{
    return elapsed_clock_cycles;
}
void boot_cpu()
{
    printf("Booting CPU\n");

    PC = 0xFFFC;
    cpu_mem = (unsigned char *)malloc(WRAM_SIZE + NO_OF_REGISTERS);
    memset(cpu_mem, 0, WRAM_SIZE + NO_OF_REGISTERS);
    PC = read_byte(PC) + ((int)read_byte(PC + 1) << 8); // Get the starting address for execution
    cpu_mem[STACK_ADDR] = 0xFF;
    connect_cpu_to_bus(status_flag_getter, status_flag_setter, pc_getter, pc_setter, cpu_stack_push, cpu_stack_pop, cpu_read, get_clock_cycles, cpu_write, NMI);
    can_execute_next_instruction = true;

    printf("Boot complete\n");
}
