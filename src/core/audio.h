#ifndef AUDIO_H
#define AUDIO_H

void boot_nes_audio();
unsigned char read_apu(int addr);
void write_apu(int addr, unsigned char value);
void update_apu();

#endif