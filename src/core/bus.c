#include "bus.h"
#include "registerOffsets.h"
#include "instructions.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#define DEBUG_READ 0

unsigned char (*cpu_reader)(int);
void (*cpu_writer)(int, unsigned char);
unsigned char (*ppu_reader_cb)(int);
void (*ppu_writer_cb)(int, unsigned char);
unsigned char (*apu_reader_cb)(int);
void (*apu_writer_cb)(int, unsigned char);
Cartriadge *cartriadge = NULL;
static unsigned char (*controller_reader)(int);
static void (*controller_writer)(int, unsigned char);

unsigned char read_byte(int addr) // Would only ever be used by the CPU tbh
{
    if (addr < 0x2000)
        return cpu_reader(addr);
    else if (0x2000 <= addr && addr < 0x4000)
        return ppu_reader_cb(addr);
    else if (addr >= 0x4000 && addr <= 0x4015)
        return apu_reader_cb(addr);
    else if (addr == 0x4016 || addr == 0x4017)
        return controller_reader(addr);
    else if (0x6000 <= addr && addr < 0x8000 && cartriadge != NULL && cartriadge->chr_ram != NULL)
        return cartriadge->chr_ram[0x6000 - addr];
    else if (0x8000 <= addr && addr <= 0xFFFF && cartriadge != NULL)
        return cartriadge->pg_rom[cartriadge->mapper(cartriadge, addr)];

    else if (addr >= REGISTER_OFFSET)
        return cpu_reader(addr);
    return 0xFF;
}
void write_byte(int addr, unsigned char value)
{
    if (addr < 0x2000)
        cpu_writer(addr, value);
    else if (0x2000 <= addr && addr < 0x4000)
    {
        ppu_writer_cb(addr, value);
    }
    else if (addr >= 0x4000 && addr <= 0x4013)
        apu_writer_cb(addr, value);
    else if (addr == 0x4014)
        ppu_writer_cb(addr, value);
    else if (addr > 0x4014 && addr < 0x4016)
        apu_writer_cb(addr, value);
    else if (addr == 0x4016)
    {
        controller_writer(addr, value);
        apu_writer_cb(addr, value);
    }

    else if (addr == 0x4017)
    {
        controller_writer(addr, value);
        apu_writer_cb(addr, value);
    }

    else if (0x8000 <= addr && addr <= 0xFFFF)
    {
        cartriadge->cart_writer(cartriadge, addr, value);
    }
    else if (0x6000 <= addr && addr < 0x8000)
    {
        cartriadge->chr_ram[cartriadge->mapper(cartriadge, addr)] = value;
    }
    else if (addr >= REGISTER_OFFSET)
        cpu_writer(addr, value);
}

unsigned char read_byte_ppu(int addr)
{
    if (addr < 0x2000)
    {
        return cartriadge->ch_rom[cartriadge->ppu_mapper(cartriadge, addr)];
    }
    return 0;
}

unsigned char fetch_from_cpu(int addr)
{
    // DMA reads: support full address space but skip PPU registers
    // to avoid side effects (reading $2002 would clear vblank, etc.)
    if (addr < 0x2000)
        return cpu_reader(addr);
    else if (addr >= 0x6000 && addr <= 0xFFFF && cartriadge && cartriadge->chr_ram)
        return cartriadge->chr_ram[cartriadge->mapper(cartriadge, addr)];
    return 0;
}

// return addresses to said registers
int get_cpu_stack() { return REGISTER_OFFSET + STACK_ADDR; }
int get_cpu_x_register() { return REGISTER_OFFSET + X_REGISTER_ADDR; }
int get_cpu_y_register() { return REGISTER_OFFSET + Y_REGISTER_ADDR; }
int get_cpu_accumulator() { return REGISTER_OFFSET + ACCUMULATOR_ADDR; }
int get_cpu_status_register() { return REGISTER_OFFSET + STATUS_REGISTER_ADDR; }

static int (*status_flag_getter)(int);
static void (*status_flag_setter)(int, bool);
static int (*pc_getter)();
static void (*pc_setter)(int);
static void (*cpu_stack_push)(unsigned char);
static unsigned char (*cpu_stack_pop)();
static void (*nmi_trigger_fn)();
static int (*get_cpu_clock_cycles)();
void connect_cpu_to_bus(int (*cpu_status_flag_getter)(int),
                        void (*cpu_status_flag_setter)(int, bool),
                        int (*cpu_pc_getter)(),
                        void (*cpu_pc_setter)(int),
                        void (*cpu_stack_push_cb)(unsigned char),
                        unsigned char (*cpu_stack_pop_cb)(),
                        unsigned char (*cpu_read)(int),
                        int (*clock_cycles_getter)(),
                        void (*cpu_write)(int, unsigned char),
                        void (*nmi_cb)())
{
    status_flag_getter = cpu_status_flag_getter;
    status_flag_setter = cpu_status_flag_setter;
    pc_getter = cpu_pc_getter;
    pc_setter = cpu_pc_setter;
    cpu_stack_push = cpu_stack_push_cb;
    cpu_stack_pop = cpu_stack_pop_cb;
    get_cpu_clock_cycles = clock_cycles_getter;
    cpu_reader = cpu_read;
    cpu_writer = cpu_write;
    nmi_trigger_fn = nmi_cb;
}

// These call the methods on the CPU, use function pointers to avoid circular deps, maybe messy
int get_cpu_status_flag(int position) { return status_flag_getter(position); }
void set_cpu_status_flag(int position, bool value) { status_flag_setter(position, value); }
int get_pc() { return pc_getter(); }
void set_pc(int newPC) { pc_setter(newPC); }
void push_to_stack(unsigned char byte) { cpu_stack_push(byte); }
void dump6004()
{
    // printf("%s", cartriadge->mem + 4);
}
void trigger_nmi()
{
    nmi_trigger_fn();
}
unsigned char pop_from_stack() { return cpu_stack_pop(); }

void connect_cartridge_to_bus(Cartriadge *cart)
{
    cartriadge = cart;
}

void connect_controller(unsigned char (*controller_reader_cb)(int), void (*controller_writer_cb)(int, unsigned char))
{
    controller_reader = controller_reader_cb;
    controller_writer = controller_writer_cb;
};

static void (*ppu_tick_callback)();
void connect_ppu_to_bus(void (*ppu_ticker)(), unsigned char (*ppu_reader_fn)(int), void (*ppu_writer_fn)(int, unsigned char))
{
    ppu_tick_callback = ppu_ticker;
    ppu_reader_cb = ppu_reader_fn;
    ppu_writer_cb = ppu_writer_fn;
}
void connect_apu_to_bus(unsigned char (*apu_reader_fn)(int), void (*apu_writer_fn)(int, unsigned char))
{
    apu_reader_cb = apu_reader_fn;
    apu_writer_cb = apu_writer_fn;
}
void ppu_tick()
{
    ppu_tick_callback();
}

int get_elapsed_clock_cycles()
{
    return get_cpu_clock_cycles();
}

Cartriadge *get_cartridge()
{
    return cartriadge;
}
