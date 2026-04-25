#ifndef BUS_H
#define BUS_H
#include <stdbool.h>
#include "cartriadge.h"

unsigned char read_byte(int addr);
void write_byte(int addr, unsigned char value);

unsigned char read_byte_ppu(int addr);

int get_cpu_status_flag(int position);
void set_cpu_status_flag(int position, bool value);
int get_pc();
void set_pc(int newPC);

void connect_cpu_to_bus(int (*cpu_status_flag_getter)(int),
                     void (*cpu_status_flag_setter)(int, bool),
                     int (*cpu_pc_getter)(),
                     void (*cpu_pc_setter)(int),
                     void (*cpu_stack_push_cb)(unsigned char),
                     unsigned char (*cpu_stack_pop_cb)(),
                     unsigned char (*cpu_read)(int),
                     void (*cpu_write)(int, unsigned char),
                     void (*nmi_cb)());
void connect_cartridge_to_bus(Cartriadge *cart);
void connect_controller(unsigned char (*controller_reader_cb)(int), void (*controller_writer_cb)(int, unsigned char));
// return addresses to said registers
int get_cpu_stack();
int get_cpu_x_register();
int get_cpu_y_register();
int get_cpu_accumulator();
int get_cpu_status_register();
void push_to_stack(unsigned char byte);
void dump6004();
void trigger_nmi();
unsigned char pop_from_stack();

/*=====================================================================================
PPU related functionality
=======================================================================================*/
// TODO: Standardize the names, cant be using different conventions
void connect_ppu_to_bus(void (*ppu_ticker)(), unsigned char (*ppu_reader_fn)(int), void (*ppu_writer_fn)(int, unsigned char));
void connect_apu_to_bus(unsigned char (*apu_reader_fn)(int), void (*apu_writer_fn)(int, unsigned char));
void ppu_tick();
bool is_dma_active();
void update_dma_cycles();

Cartriadge *get_cartridge();
unsigned char fetch_from_cpu(int addr);
#endif