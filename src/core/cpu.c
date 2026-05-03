#include "cpu.h"
#include "instructions.h"
#include "bus.h"
#include "ppu.h"
#include "registerOffsets.h"
#include "addressing_modes.h"
#include "ControllerKeyStates.h"
#include "frameData.h"
#include "controller.h"
#include "mappers/m002.h"
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
            remaining_clock_cycles = 6;
        }
        if (can_execute_next_instruction && pending_irq_func())
        {
            elapsed_clock_cycles += 1;
            update_controller_input(keyStates);
            execute_irq();
            can_execute_next_instruction = false;
            remaining_clock_cycles = 6;
        }

        if (can_execute_next_instruction)
        {
            elapsed_clock_cycles += 1;
            ExecutionInfo instr = get_next_instruction();
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

#define TRACE_BUF 128
static struct { int pc; unsigned char op; unsigned char a,x,y,sp,sr; int ea; unsigned char mv; const char *mn; const char *am; unsigned char prg_bank; unsigned short indir_base; } trace_ring[TRACE_BUF];
static int trace_pos = 0;
static bool trace_on = false;

void trace_set_enabled(bool on) { trace_on = on; }

static void trace_record(int pc, unsigned char op, int ea, unsigned char mv, const char *mn, const char *am, unsigned char prg, unsigned short ibase)
{
    trace_ring[trace_pos].pc = pc;
    trace_ring[trace_pos].op = op;
    trace_ring[trace_pos].a  = cpu_mem[ACCUMULATOR_ADDR];
    trace_ring[trace_pos].x  = cpu_mem[X_REGISTER_ADDR];
    trace_ring[trace_pos].y  = cpu_mem[Y_REGISTER_ADDR];
    trace_ring[trace_pos].sp = cpu_mem[STACK_ADDR];
    trace_ring[trace_pos].sr = cpu_mem[STATUS_REGISTER_ADDR];
    trace_ring[trace_pos].ea = ea;
    trace_ring[trace_pos].mv = mv;
    trace_ring[trace_pos].mn = mn;
    trace_ring[trace_pos].am = am;
    trace_ring[trace_pos].prg_bank = prg;
    trace_ring[trace_pos].indir_base = ibase;
    trace_pos = (trace_pos + 1) % TRACE_BUF;
}

void trace_dump_ringbuffer(void)
{
    int oldest = trace_pos;
    printf("=== TRACE DUMP (%d instructions) ===\n", TRACE_BUF);
    for (int i = 0; i < TRACE_BUF; i++)
    {
        int idx = (oldest + i) % TRACE_BUF;
        printf("$%04X: %02X %-6s %-8s A=%02X X=%02X Y=%02X SP=%02X P=%02X",
               trace_ring[idx].pc, trace_ring[idx].op,
               trace_ring[idx].mn, trace_ring[idx].am,
               trace_ring[idx].a, trace_ring[idx].x, trace_ring[idx].y,
               trace_ring[idx].sp, trace_ring[idx].sr);
        if (trace_ring[idx].ea >= 0)
            printf(" EA=$%04X MV=$%02X", trace_ring[idx].ea, trace_ring[idx].mv);
        if (trace_ring[idx].indir_base)
            printf(" IB=$%04X", trace_ring[idx].indir_base);
        if (trace_ring[idx].ea >= 0x8000 && trace_ring[idx].ea <= 0xFFFF)
            printf(" bank=%d", trace_ring[idx].prg_bank);
        printf("\n");
    }
    printf("zp: $80=%02X $81=%02X $82=%02X $83=%02X $84=%02X $85=%02X $86=%02X $87=%02X $88=%02X $89=%02X\n",
           read_byte(0x80), read_byte(0x81), read_byte(0x82), read_byte(0x83), read_byte(0x84),
           read_byte(0x85), read_byte(0x86), read_byte(0x87), read_byte(0x88), read_byte(0x89));
    printf("=== END TRACE ===\n");
}

int execute_instruction(ExecutionInfo exInfo)
{
    unsigned char opcode = read_byte(PC);
    int ea = -1;
    unsigned char mv = 0;
    unsigned char prg = 0;
    unsigned short ibase = 0;

    const char *mn, *am;
    disassemble_opcode(opcode, &mn, &am);

    if (exInfo.addressing_mode != IMP && exInfo.addressing_mode != ACC &&
        exInfo.addressing_mode != STK && exInfo.addressing_mode != PCR)
    {
        ea = exInfo.addressing_mode(PC + 1);
        unsigned char (*exec)(ExecutionInfo *) = exInfo.executor;
        bool store = (exec == STA || exec == STX || exec == STY ||
                       exec == ASL || exec == LSR || exec == ROL || exec == ROR ||
                       exec == INC || exec == DEC);
        if (store)
            mv = read_byte(get_cpu_accumulator());
        else if (ea >= 0 && ea < 0x2000)
            mv = read_byte(ea);

        if (exInfo.addressing_mode == ZP_IND_INDX_Y)
        {
            unsigned char zp = read_byte(PC + 1);
            ibase = read_byte(zp) | ((unsigned short)read_byte((zp + 1) & 0xFF) << 8);
        }
    }

    if (ea >= 0x8000)
        prg = m002_get_prg_bank();

    exInfo.executor(&exInfo);
    PC += exInfo.instruction_size;

    trace_record(PC - exInfo.instruction_size, opcode, ea, mv, mn, am, prg, ibase);

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
