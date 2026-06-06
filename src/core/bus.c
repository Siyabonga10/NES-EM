#include "bus.h"
#include "rom_loader.h"
#include <stdint.h>
#include "registerOffsets.h"
#include "instructions.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
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
  unsigned char val;
  if (addr < 0x2000)
    val = cpu_reader(addr);
  else if (0x2000 <= addr && addr < 0x4000)
    val = ppu_reader_cb(addr);
  else if (addr >= 0x4000 && addr <= 0x4015)
    val = apu_reader_cb(addr);
  else if (addr == 0x4016 || addr == 0x4017)
    val = controller_reader(addr);
  else if (0x4020 <= addr && addr <= 0xFFFF && cartriadge != NULL && cartriadge->cpu_read != NULL)
    val = cartriadge->cpu_read(cartriadge, addr);
  else if (addr >= REGISTER_OFFSET)
    val = cpu_reader(addr);
  else
    val = 0xFF;
  return val;
}
void write_byte(int addr, unsigned char value) {
  if (addr < 0x2000)
    cpu_writer(addr, value);
  else if (0x2000 <= addr && addr < 0x4000) {
    ppu_writer_cb(addr, value);
  } else if (addr >= 0x4000 && addr <= 0x4013)
    apu_writer_cb(addr, value);
  else if (addr == 0x4014)
    ppu_writer_cb(addr, value);
  else if (addr > 0x4014 && addr < 0x4016)
    apu_writer_cb(addr, value);
  else if (addr == 0x4016) {
    controller_writer(addr, value);
    apu_writer_cb(addr, value);
  }

  else if (addr == 0x4017) {
    controller_writer(addr, value);
    apu_writer_cb(addr, value);
  }

  else if (0x6000 <= addr && addr <= 0xFFFF && cartriadge->cart_writer != NULL) {
    cartriadge->cart_writer(cartriadge, addr, value);
  } else if (addr >= REGISTER_OFFSET)
    cpu_writer(addr, value);
}
unsigned char read_byte_ppu(int addr) {
  if (cartriadge != NULL && cartriadge->ppu_read != NULL)
    return cartriadge->ppu_read(cartriadge, addr);
  return 0;
}

unsigned char fetch_from_cpu(int addr) {
  // DMA reads: support full address space but skip PPU registers
  if (addr < 0x2000)
    return cpu_reader(addr);
  else if (addr >= 0x6000 && addr <= 0xFFFF && cartriadge && cartriadge->cpu_read)
    return cartriadge->cpu_read(cartriadge, addr);
  return 0;
}

// return addresses to said registers
int get_cpu_stack() {
  return REGISTER_OFFSET + STACK_ADDR;
}
int get_cpu_x_register() {
  return REGISTER_OFFSET + X_REGISTER_ADDR;
}
int get_cpu_y_register() {
  return REGISTER_OFFSET + Y_REGISTER_ADDR;
}
int get_cpu_accumulator() {
  return REGISTER_OFFSET + ACCUMULATOR_ADDR;
}
int get_cpu_status_register() {
  return REGISTER_OFFSET + STATUS_REGISTER_ADDR;
}

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
                        void (*nmi_cb)()) {
  status_flag_getter   = cpu_status_flag_getter;
  status_flag_setter   = cpu_status_flag_setter;
  pc_getter            = cpu_pc_getter;
  pc_setter            = cpu_pc_setter;
  cpu_stack_push       = cpu_stack_push_cb;
  cpu_stack_pop        = cpu_stack_pop_cb;
  get_cpu_clock_cycles = clock_cycles_getter;
  cpu_reader           = cpu_read;
  cpu_writer           = cpu_write;
  nmi_trigger_fn       = nmi_cb;
}

// These call the methods on the CPU, use function pointers to avoid circular deps, maybe messy
int get_cpu_status_flag(int position) {
  return status_flag_getter(position);
}
void set_cpu_status_flag(int position, bool value) {
  status_flag_setter(position, value);
}
int get_pc() {
  return pc_getter();
}
void set_pc(int newPC) {
  pc_setter(newPC);
}
void push_to_stack(unsigned char byte) {
  cpu_stack_push(byte);
}
void dump6004() {
  // printf("%s", cartriadge->mem + 4);
}
void trigger_nmi() {
  nmi_trigger_fn();
}
unsigned char pop_from_stack() {
  return cpu_stack_pop();
}

void connect_cartridge_to_bus(Cartriadge *cart) {
  cartriadge = cart;
}

void connect_controller(unsigned char (*controller_reader_cb)(int), void (*controller_writer_cb)(int, unsigned char)) {
  controller_reader = controller_reader_cb;
  controller_writer = controller_writer_cb;
};

static void (*ppu_tick_callback)();
void connect_ppu_to_bus(void (*ppu_ticker)(), unsigned char (*ppu_reader_fn)(int), void (*ppu_writer_fn)(int, unsigned char)) {
  ppu_tick_callback = ppu_ticker;
  ppu_reader_cb     = ppu_reader_fn;
  ppu_writer_cb     = ppu_writer_fn;
}
void connect_apu_to_bus(unsigned char (*apu_reader_fn)(int), void (*apu_writer_fn)(int, unsigned char)) {
  apu_reader_cb = apu_reader_fn;
  apu_writer_cb = apu_writer_fn;
}
void ppu_tick() {
  ppu_tick_callback();
}

int get_elapsed_clock_cycles() {
  return get_cpu_clock_cycles();
}

Cartriadge *get_cartridge() {
  return cartriadge;
}

#include "save_state_info.h"
#include <stddef.h>
#define MAX_SAVABLE_STATES 20
#define SAVE_STATE_HEADER_OVERHEAD offsetof(Save_State_Info, content)
static save_section_state savable_sections[MAX_SAVABLE_STATES] = {0};
static load_section       loadable_sections[MAX_SAVABLE_STATES] = {0};
static char               load_labels[MAX_SAVABLE_STATES][SECTION_LABEL_SIZE];

void save_section_to_buffer(Save_State_Info *section, unsigned char *start_location, uint32_t max_bytes) {
  assert(SECTION_LABEL_SIZE + sizeof(section->content_length) + section->content_length <= max_bytes);
  memcpy(start_location, section->section_label, SECTION_LABEL_SIZE);
  start_location += SECTION_LABEL_SIZE;
  memcpy(start_location, &section->content_length, sizeof(section->content_length));
  start_location += sizeof(section->content_length);
  memcpy(start_location, section->content, section->content_length);
}
bool save_state(unsigned char *save_buffer, uint32_t buffer_length) {
  uint32_t remaining_save_buffer = buffer_length;

  for (int i = 0; i < MAX_SAVABLE_STATES; i++) {
    if (!savable_sections[i])
      break;

    Save_State_Info section                  = {};
    uint32_t        allowable_content_length = remaining_save_buffer - SAVE_STATE_HEADER_OVERHEAD;
    section.content                          = malloc(allowable_content_length);
    savable_sections[i](&section, allowable_content_length);
    assert(section.content_length + SAVE_STATE_HEADER_OVERHEAD <= remaining_save_buffer);
    save_section_to_buffer(&section, save_buffer + (buffer_length - remaining_save_buffer), remaining_save_buffer);
    remaining_save_buffer -= SECTION_LABEL_SIZE + sizeof(section.content_length) + section.content_length;
    free(section.content);
  }
  return true;
}

uint32_t save_state_size(void) {
  uint32_t total = 0;
  for (int i = 0; i < MAX_SAVABLE_STATES; i++) {
    if (!savable_sections[i])
      break;

    Save_State_Info section          = {};
    uint32_t        max_content_size = 0x10000;
    section.content                  = malloc(max_content_size);
    savable_sections[i](&section, max_content_size);
    total += SECTION_LABEL_SIZE + sizeof(section.content_length) + section.content_length;
    free(section.content);
  }
  return total;
}

// for restoring states
void load_from_state(unsigned char *data, uint32_t size) {
  unsigned char *current                     = data;
  uint32_t       bytes_left                 = size;
  char           header[SECTION_LABEL_SIZE] = {};
  uint32_t       content_len                = 0;
  for (;;) {
    if (bytes_left < SAVE_STATE_HEADER_OVERHEAD)
      break;
    memcpy(header, current, SECTION_LABEL_SIZE);
    if (header[0] == 0)
      break;

    current += SECTION_LABEL_SIZE;
    bytes_left -= SECTION_LABEL_SIZE;
    memcpy(&content_len, current, sizeof(content_len));

    current += sizeof(content_len);
    bytes_left -= sizeof(content_len);

    load_section loader = NULL;
    for (int i = 0; i < MAX_SAVABLE_STATES; i++) {
      if (memcmp(load_labels[i], header, SECTION_LABEL_SIZE) == 0) {
        loader = loadable_sections[i];
        break;
      }
    }
    assert(loader);
    assert(content_len <= bytes_left);
    Save_State_Info section_data = {};
    memcpy(section_data.section_label, header, SECTION_LABEL_SIZE);
    section_data.content_length  = content_len;

    section_data.content = malloc(content_len);
    memcpy(section_data.content, current, content_len);
    loader(&section_data);
    current += content_len;
    bytes_left -= content_len;
    free(section_data.content);
  }
}

void register_savable_component(const char *label, save_section_state saver, load_section loader) {
  for (int i = 0; i < MAX_SAVABLE_STATES; i++) {
    if (!savable_sections[i]) {
      savable_sections[i] = saver;
      loadable_sections[i] = loader;
      strncpy(load_labels[i], label, SECTION_LABEL_SIZE);
      break;
    }
  }
}

bool load_state(const unsigned char *rom_data, uint32_t rom_size,
                const unsigned char *state_data, uint32_t state_size) {
  Cartriadge *old_cart = cartriadge;
  if (old_cart) {
    free(old_cart->pg_rom);
    free(old_cart->ch_rom);
    free(old_cart->prg_ram);
    free(old_cart->chr_ram);
    free(old_cart);
  }

  Cartriadge *new_cart = malloc(sizeof(Cartriadge));
  if (!new_cart)
    return false;

  int result = load_cartridge_from_memory((unsigned char *)rom_data, (int)rom_size, new_cart);
  if (result != 0) {
    free(new_cart);
    return false;
  }

  load_from_state((unsigned char *)state_data, state_size);
  return true;
}