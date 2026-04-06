#include "audio.h"
#include "bus.h"
#include <string.h>
#include <raylib.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>

#define APU_REGISTER_SIZE 16 // $4000-$400F
#define CPU_CLOCK_SPEED 1790000
#define SAMPLING_RATE 44100

static const unsigned char duty_table[4][8] = {
    {0, 1, 0, 0, 0, 0, 0, 0},
    {0, 1, 1, 0, 0, 0, 0, 0},
    {0, 1, 1, 1, 1, 0, 0, 0},
    {1, 0, 0, 1, 1, 1, 1, 1},
};

static const unsigned char triangle_table[32] = {
    15, 14, 13, 12, 11, 10, 9, 8,
    7, 6, 5, 4, 3, 2, 1, 0,
    0, 1, 2, 3, 4, 5, 6, 7,
    8, 9, 10, 11, 12, 13, 14, 15};

static const unsigned short dmc_rate_table[16] = {
    428, 380, 340, 320, 286, 254, 226, 214,
    190, 160, 142, 128, 106, 84, 72, 54};

static unsigned char registers[APU_REGISTER_SIZE];
static unsigned char dmc_regs[4];

static AudioStream master_stream;

// phase accumulators (double for stability)
static double phase[3] = {0};

// DMC state
static unsigned short dmc_sample_addr;
static unsigned short dmc_sample_len;
static unsigned short dmc_bytes_remaining;
static unsigned char dmc_shift_register;
static unsigned char dmc_bits_remaining = 0;
static unsigned char dmc_output_level;
static bool dmc_silence = true;
static double dmc_cycle_accum = 0.0;

// -------------------- CHANNEL GENERATORS --------------------

static float pulse_sample(int pulse_index)
{
  int base = pulse_index * 4;

  unsigned char duty = (registers[base] >> 6) & 0x03;
  unsigned char vol = registers[base] & 0x0F;
  unsigned short timer = registers[base + 2] |
                         ((registers[base + 3] & 0x07) << 8);

  if (timer < 8)
    return 0.0f;

  double freq = (double)CPU_CLOCK_SPEED / (16.0 * (timer + 1));
  double phase_inc = freq / SAMPLING_RATE;

  int step = (int)(phase[pulse_index] * 8.0) & 7;
  float amp = (vol / 15.0f) * 0.5f;

  float out = duty_table[duty][step] ? amp : -amp;

  phase[pulse_index] += phase_inc;
  if (phase[pulse_index] >= 1.0)
    phase[pulse_index] -= 1.0;

  return out;
}

static float triangle_sample(void)
{
  unsigned short timer = registers[10] |
                         ((registers[11] & 0x07) << 8);

  if (timer < 2)
    return 0.0f;

  double freq = (double)CPU_CLOCK_SPEED / (32.0 * (timer + 1));
  double phase_inc = freq / SAMPLING_RATE;

  int step = (int)(phase[2] * 32.0) & 31;
  float out = ((triangle_table[step] / 15.0f) * 2.0f - 1.0f) * 0.5f;

  phase[2] += phase_inc;
  if (phase[2] >= 1.0)
    phase[2] -= 1.0;

  return out;
}

static float dmc_sample(void)
{
  unsigned char rate_index = dmc_regs[0] & 0x0F;
  unsigned short rate = dmc_rate_table[rate_index];

  double cpu_cycles_per_sample = (double)CPU_CLOCK_SPEED / SAMPLING_RATE;
  dmc_cycle_accum += cpu_cycles_per_sample;

  while (dmc_cycle_accum >= rate)
  {
    dmc_cycle_accum -= rate;

    if (dmc_bits_remaining == 0)
    {
      if (dmc_bytes_remaining == 0)
      {
        bool loop = (dmc_regs[0] >> 6) & 1;
        if (loop)
        {
          dmc_sample_addr = 0xC000 + ((unsigned short)dmc_regs[2] << 6);
          dmc_bytes_remaining = ((unsigned short)dmc_regs[3] << 4) + 1;
          dmc_silence = false;
        }
        else
        {
          dmc_silence = true;
        }
      }
      else
      {
        dmc_shift_register = readByte(dmc_sample_addr);
        dmc_sample_addr = (dmc_sample_addr == 0xFFFF) ? 0x8000 : dmc_sample_addr + 1;
        dmc_bytes_remaining--;
        dmc_bits_remaining = 8;
        dmc_silence = false;
      }
    }

    if (!dmc_silence)
    {
      if (dmc_shift_register & 1)
      {
        if (dmc_output_level <= 125)
          dmc_output_level += 2;
      }
      else
      {
        if (dmc_output_level >= 2)
          dmc_output_level -= 2;
      }

      dmc_shift_register >>= 1;
      dmc_bits_remaining--;
    }
  }

  return dmc_silence ? 0.0f
                     : ((dmc_output_level / 127.0f) - 1.0f) * 0.5f;
}

// -------------------- MASTER MIXER --------------------

static void master_callback(void *buffer, unsigned int frames)
{
  float *out = (float *)buffer;

  for (unsigned int i = 0; i < frames; i++)
  {
    float sample = 0.0f;

    sample += pulse_sample(0);
    sample += pulse_sample(1);
    sample += triangle_sample();
    sample += dmc_sample();

    out[i] = sample * 0.25f;
  }
}

// -------------------- PUBLIC API --------------------

void boot_nes_audio()
{
  InitAudioDevice();
  connect_apu_to_bus(read_apu, write_apu);

  master_stream = LoadAudioStream(SAMPLING_RATE, 32, 1);
  SetAudioStreamCallback(master_stream, master_callback);
  PlayAudioStream(master_stream);
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
  if (addr >= 0x4010 && addr <= 0x4013)
  {
    int i = addr - 0x4010;
    dmc_regs[i] = value;

    if (addr == 0x4011)
    {
      dmc_output_level = value & 0x7F;
    }
    else if (addr == 0x4012)
    {
      dmc_sample_addr = 0xC000 + ((unsigned short)value << 6);
    }
    else if (addr == 0x4013)
    {
      dmc_bytes_remaining = ((unsigned short)value << 4) + 1;
      dmc_sample_len = dmc_bytes_remaining;
    }
    return;
  }

  size_t index = addr - 0x4000;
  if (index >= APU_REGISTER_SIZE)
    return;
  registers[index] = value;
}