#ifndef NES_EM_CORE_H
#define NES_EM_CORE_H

typedef struct Cartriadge {
  unsigned char *pg_rom;
  unsigned char *ch_rom;

  unsigned char *chr_ram;
  unsigned char *prg_ram;

  unsigned char (*cpu_read)(struct Cartriadge *, int);
  unsigned char (*ppu_read)(struct Cartriadge *, int);
  void (*ppu_write)(struct Cartriadge *, int, unsigned char);
  void (*cart_writer)(struct Cartriadge *, int, unsigned char);
  void (*scanline_tick)(struct Cartriadge *);
  int size;
  int pg_rom_size;
  int ch_ram_size;
  int prg_ram_size;
  int mirroring_mode;

  int pg_rom_bank_size;
  int pg_rom_bank_count;

  int ch_rom_bank_size;
  int ch_rom_bank_count;

  char board_type[24];
  bool has_battery;

} Cartriadge;

typedef struct {
  unsigned char r;
  unsigned char g;
  unsigned char b;
  unsigned char a;
} NesColor;

typedef struct {
  volatile bool is_new_frame;
  size_t        width;
  size_t        height;
  NesColor     *data;
} FrameData;

void          boot_ppu();
void          kill_ppu();
FrameData    *request_frame();
void          boot_cpu();


#endif