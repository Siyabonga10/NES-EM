#include "audio.h"
#include "bus.h"
#include <string.h>
#include <raylib.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>

#define APU_REGISTER_SIZE 12 // $4000-$400B
#define CPU_CLOCK_SPEED 1790000
#define SAMPLING_RATE 44100

static const unsigned char duty_table[4][8] = {
    {0, 1, 0, 0, 0, 0, 0, 0},
    {0, 1, 1, 0, 0, 0, 0, 0},
    {0, 1, 1, 1, 1, 0, 0, 0},
    {1, 0, 0, 1, 1, 1, 1, 1},
};

static const unsigned char triangle_table[32] = {
    15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0,
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};

static unsigned char registers[APU_REGISTER_SIZE];
static AudioStream pulse_streams[2];
static AudioStream triangle_stream;
static float phase[3] = {0};

static void pulse_callback(void *buffer, unsigned int frames, int pulse_index)
{
  int base = pulse_index * 4;
  unsigned char duty = (registers[base] >> 6) & 0x03;
  unsigned char vol = registers[base] & 0x0F;
  unsigned short timer = registers[base + 2] | ((registers[base + 3] & 0x07) << 8);

  unsigned char *out = (unsigned char *)buffer;

  if (timer < 8)
  {
    memset(out, 128, frames);
    return;
  }

  float freq = (float)CPU_CLOCK_SPEED / (16.0f * (timer + 1));
  float phase_inc = freq / SAMPLING_RATE;
  unsigned char amplitude = (unsigned char)((vol / 15.0f) * 127.0f);

  for (unsigned int i = 0; i < frames; i++)
  {
    int duty_step = (int)(phase[pulse_index] * 8.0f) % 8;
    out[i] = duty_table[duty][duty_step] ? (128 + amplitude) : (128 - amplitude);
    phase[pulse_index] += phase_inc;
    if (phase[pulse_index] >= 1.0f)
      phase[pulse_index] -= 1.0f;
  }
}

static void triangle_callback(void *buffer, unsigned int frames)
{
  // Triangle base is $4008, timer at $400A-$400B
  unsigned short timer = registers[10] | ((registers[11] & 0x07) << 8);
  unsigned char *out = (unsigned char *)buffer;

  if (timer < 2)
  {
    memset(out, 128, frames);
    return;
  }

  // Triangle timer clocks at CPU rate, 32 steps per period
  float freq = (float)CPU_CLOCK_SPEED / (32.0f * (timer + 1));
  float phase_inc = freq / SAMPLING_RATE;

  for (unsigned int i = 0; i < frames; i++)
  {
    int step = (int)(phase[2] * 32.0f) % 32;
    out[i] = 128 + (int)((triangle_table[step] / 15.0f) * 63.0f) - 31;
    phase[2] += phase_inc;
    if (phase[2] >= 1.0f)
      phase[2] -= 1.0f;
  }
}

static void pulse_callback_0(void *buffer, unsigned int frames) { pulse_callback(buffer, frames, 0); }
static void pulse_callback_1(void *buffer, unsigned int frames) { pulse_callback(buffer, frames, 1); }

void boot_nes_audio()
{
  InitAudioDevice();
  connect_apu_to_bus(read_apu, write_apu);

  for (int i = 0; i < 2; i++)
  {
    pulse_streams[i] = LoadAudioStream(SAMPLING_RATE, 8, 1);
    SetAudioStreamCallback(pulse_streams[i], i == 0 ? pulse_callback_0 : pulse_callback_1);
    PlayAudioStream(pulse_streams[i]);
  }

  triangle_stream = LoadAudioStream(SAMPLING_RATE, 8, 1);
  SetAudioStreamCallback(triangle_stream, triangle_callback);
  PlayAudioStream(triangle_stream);
}

unsigned char read_apu(int addr)
{
  size_t index = addr - 0x4000;
  if (index >= APU_REGISTER_SIZE)
    return 0;
  return registers[index];
}

void write_apu(int addr, unsigned char value)
{
  size_t index = addr - 0x4000;
  if (index >= APU_REGISTER_SIZE)
    return;
  registers[index] = value;
}

// void update_apu() {}