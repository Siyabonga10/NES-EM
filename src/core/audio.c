#include "audio.h"
#include "bus.h"
#include "save_state/register_save_state.h"
#include "save_state/apu_save_state.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <stdbool.h>
#include <assert.h>

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
static bool          channel_enable[5] = {false}; // pulse1, pulse2, triangle, noise, dmc
static unsigned char length_counter[3] = {0};     // pulse1, pulse2, triangle

// Length counter lookup table (NTSC)
static const unsigned char length_table[32] = {
    10, 254, 20, 2, 40, 4, 80, 6, 160, 8, 60, 10, 14, 12, 26, 14,
    12, 16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30};

// phase accumulators (double for stability)
static double phase[3] = {0};

// DMC state
static unsigned short dmc_sample_addr;
static unsigned short dmc_sample_len;
static unsigned short dmc_bytes_remaining;
static unsigned char  dmc_shift_register;
static unsigned char  dmc_bits_remaining = 0;
static unsigned char  dmc_output_level;
static bool           dmc_silence     = true;
static double         dmc_cycle_accum = 0.0;

// -------------------- CHANNEL GENERATORS --------------------

static float pulse_sample(int pulse_index) {
  if (!channel_enable[pulse_index] || length_counter[pulse_index] == 0) {
    phase[pulse_index] = 0.0;
    return 0.0f;
  }

  int base = pulse_index * 4;

  unsigned char  duty  = (registers[base] >> 6) & 0x03;
  unsigned char  vol   = registers[base] & 0x0F;
  unsigned short timer = registers[base + 2] |
                         ((registers[base + 3] & 0x07) << 8);

  if (timer < 8)
    return 0.0f;

  double freq      = (double)CPU_CLOCK_SPEED / (16.0 * (timer + 1));
  double phase_inc = freq / SAMPLING_RATE;

  int   step = (int)(phase[pulse_index] * 8.0) & 7;
  float amp  = (vol / 15.0f) * 0.5f;

  float out = duty_table[duty][step] ? amp : -amp;

  phase[pulse_index] += phase_inc;
  if (phase[pulse_index] >= 1.0)
    phase[pulse_index] -= 1.0;

  return out;
}

static float triangle_sample(void) {
  if (!channel_enable[2] || length_counter[2] == 0) {
    phase[2] = 0.0;
    return 0.0f;
  }

  unsigned short timer = registers[10] |
                         ((registers[11] & 0x07) << 8);

  if (timer < 2)
    return 0.0f;

  double freq      = (double)CPU_CLOCK_SPEED / (32.0 * (timer + 1));
  double phase_inc = freq / SAMPLING_RATE;

  int   step = (int)(phase[2] * 32.0) & 31;
  float out  = ((triangle_table[step] / 15.0f) * 2.0f - 1.0f) * 0.5f;

  phase[2] += phase_inc;
  if (phase[2] >= 1.0)
    phase[2] -= 1.0;

  return out;
}

static float dmc_sample(void) {
  if (!channel_enable[4])
    return 0.0f;

  unsigned char  rate_index = dmc_regs[0] & 0x0F;
  unsigned short rate       = dmc_rate_table[rate_index];

  double cpu_cycles_per_sample = (double)CPU_CLOCK_SPEED / SAMPLING_RATE;
  dmc_cycle_accum += cpu_cycles_per_sample;

  while (dmc_cycle_accum >= rate) {
    dmc_cycle_accum -= rate;

    if (dmc_bits_remaining == 0) {
      if (dmc_bytes_remaining == 0) {
        bool loop = (dmc_regs[0] >> 6) & 1;
        if (loop) {
          dmc_sample_addr     = 0xC000 + ((unsigned short)dmc_regs[2] << 6);
          dmc_bytes_remaining = ((unsigned short)dmc_regs[3] << 4) + 1;
          dmc_silence         = false;
        } else {
          dmc_silence = true;
        }
      } else {
        dmc_shift_register = read_byte(dmc_sample_addr);
        dmc_sample_addr    = (dmc_sample_addr == 0xFFFF) ? 0x8000 : dmc_sample_addr + 1;
        dmc_bytes_remaining--;
        dmc_bits_remaining = 8;
        dmc_silence        = false;
      }
    }

    if (!dmc_silence) {
      if (dmc_shift_register & 1) {
        if (dmc_output_level <= 125)
          dmc_output_level += 2;
      } else {
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

static int apu_calls_with_the_same_pc = 0;
static int last_pc_value              = 0;

void apu_mix_samples(float *buffer, unsigned int frames) {
  // handle pauses sort of
  if (get_pc() == last_pc_value)
    apu_calls_with_the_same_pc += 1;
  else
    apu_calls_with_the_same_pc = 0;
  bool should_be_silent = apu_calls_with_the_same_pc >= 10;

  float *out = buffer;
  if (should_be_silent) {
    memset(out, 0, frames * sizeof(float));
    return;
  }

  for (unsigned int i = 0; i < frames; i++) {
    float sample = 0.0f;

    sample += pulse_sample(0);
    sample += pulse_sample(1);
    sample += triangle_sample();
    sample += dmc_sample();

    // Remove extremely small values that could cause denormal noise
    if (fabs(sample) < 1e-6f) {
      sample = 0.0f;
    }

    out[i] = sample * 0.25f;
  }
  last_pc_value = get_pc();
}

// -------------------- PUBLIC API --------------------

void boot_nes_audio() {
  connect_apu_to_bus(read_apu, write_apu);

  // Initialize APU state
  memset(registers, 0, APU_REGISTER_SIZE);
  memset(dmc_regs, 0, 4);
  for (int i = 0; i < 5; i++) {
    channel_enable[i] = false;
  }
  phase[0] = phase[1] = phase[2] = 0.0;
  dmc_sample_addr                = 0;
  dmc_sample_len                 = 0;
  dmc_bytes_remaining            = 0;
  dmc_shift_register             = 0;
  dmc_bits_remaining             = 0;
  dmc_output_level               = 0;
  dmc_silence                    = true;
  dmc_cycle_accum                = 0.0;
}

unsigned char read_apu(int addr) {
  if (addr == 0x4015) {
    unsigned char status = 0;
    if (channel_enable[0])
      status |= 0x01; // pulse1
    if (channel_enable[1])
      status |= 0x02; // pulse2
    if (channel_enable[2])
      status |= 0x04; // triangle
    // noise not implemented
    if (channel_enable[4])
      status |= 0x10; // dmc active
    // TODO: DMC interrupt flag (bit 7)
    return status;
  }
  size_t index = addr - 0x4000;
  if (index >= APU_REGISTER_SIZE)
    return 0;
  return registers[index];
}

void write_apu(int addr, unsigned char value) {
  if (addr == 0x4015) {
    // channel enable bits
    channel_enable[0] = (value & 0x01) != 0; // pulse1
    channel_enable[1] = (value & 0x02) != 0; // pulse2
    channel_enable[2] = (value & 0x04) != 0; // triangle
    // noise not implemented
    channel_enable[4] = (value & 0x10) != 0; // dmc
    // Clear length counter when channel is disabled
    if (!channel_enable[0])
      length_counter[0] = 0;
    if (!channel_enable[1])
      length_counter[1] = 0;
    if (!channel_enable[2])
      length_counter[2] = 0;
    // if dmc enabled, start DMC? For now just set enable flag.
    return;
  }
  if (addr == 0x4017) {
    // frame counter, ignore for now
    return;
  }

  // Length counter loads
  if (addr == 0x4003) {
    length_counter[0] = length_table[(value >> 3) & 0x1F];
  } else if (addr == 0x4007) {
    length_counter[1] = length_table[(value >> 3) & 0x1F];
  } else if (addr == 0x400B) {
    length_counter[2] = length_table[(value >> 3) & 0x1F];
  }

  if (addr >= 0x4010 && addr <= 0x4013) {
    int i       = addr - 0x4010;
    dmc_regs[i] = value;

    if (addr == 0x4011) {
      dmc_output_level = value & 0x7F;
    } else if (addr == 0x4012) {
      dmc_sample_addr = 0xC000 + ((unsigned short)value << 6);
    } else if (addr == 0x4013) {
      dmc_bytes_remaining = ((unsigned short)value << 4) + 1;
      dmc_sample_len      = dmc_bytes_remaining;
    }
    return;
  }

  size_t index = addr - 0x4000;
  if (index >= APU_REGISTER_SIZE)
    return;
  registers[index] = value;
}

static int apu_last_cycles = 0;

void update_apu() {
  int current   = get_elapsed_clock_cycles();
  int delta     = current - apu_last_cycles;
  int        threshold   = (CPU_CLOCK_SPEED / 240) * 2;
  if (delta < threshold)
    return;
  int steps = delta / threshold;
  apu_last_cycles += steps * threshold;

  for (int s = 0; s < steps; s++) {
    for (int i = 0; i < 3; i++) {
      if (length_counter[i] > 0)
        length_counter[i]--;
    }
  }
}

static void apu_save_state(Save_State_Info *save_buffer, uint32_t allowable_content_length) {
  ApuSaveState state;
  memcpy(state.registers, registers, sizeof(registers));
  memcpy(state.dmc_regs, dmc_regs, sizeof(dmc_regs));
  memcpy(state.channel_enable, channel_enable, sizeof(channel_enable));
  memcpy(state.length_counter, length_counter, sizeof(length_counter));
  memcpy(state.phase, phase, sizeof(phase));
  state.dmc_sample_addr    = dmc_sample_addr;
  state.dmc_sample_len     = dmc_sample_len;
  state.dmc_bytes_remaining = dmc_bytes_remaining;
  state.dmc_shift_register = dmc_shift_register;
  state.dmc_bits_remaining = dmc_bits_remaining;
  state.dmc_output_level   = dmc_output_level;
  state.dmc_silence        = dmc_silence;
  state.dmc_cycle_accum    = dmc_cycle_accum;
  state.last_cycles        = apu_last_cycles;

  memset(save_buffer->section_label, 0, SECTION_LABEL_SIZE);
  strncpy(save_buffer->section_label, "APU", SECTION_LABEL_SIZE - 1);
  save_buffer->content_length = sizeof(ApuSaveState);
  assert(sizeof(ApuSaveState) <= allowable_content_length);
  memcpy(save_buffer->content, &state, sizeof(ApuSaveState));
}

static void apu_load_state(Save_State_Info *section_data) {
  ApuSaveState state;
  memcpy(&state, section_data->content, sizeof(ApuSaveState));

  memcpy(registers, state.registers, sizeof(registers));
  memcpy(dmc_regs, state.dmc_regs, sizeof(dmc_regs));
  memcpy(channel_enable, state.channel_enable, sizeof(channel_enable));
  memcpy(length_counter, state.length_counter, sizeof(length_counter));
  memcpy(phase, state.phase, sizeof(phase));
  dmc_sample_addr     = state.dmc_sample_addr;
  dmc_sample_len      = state.dmc_sample_len;
  dmc_bytes_remaining = state.dmc_bytes_remaining;
  dmc_shift_register  = state.dmc_shift_register;
  dmc_bits_remaining  = state.dmc_bits_remaining;
  dmc_output_level    = state.dmc_output_level;
  dmc_silence         = state.dmc_silence;
  dmc_cycle_accum     = state.dmc_cycle_accum;
  apu_last_cycles     = state.last_cycles;
}

REGISTER_SAVE_STATE("APU", apu_save_state, apu_load_state);