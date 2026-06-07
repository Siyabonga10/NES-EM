#include "audio.h"
#include "bus.h"
#include "instructions.h"
#include "save_state/register_save_state.h"
#include "save_state/apu_save_state.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
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

static const unsigned short noise_period_table[16] = {
    4, 8, 16, 32, 64, 96, 128, 160,
    202, 254, 380, 508, 762, 1016, 2034, 4068};

static float pulse_table[31];
static float tnd_table[203];

static void init_mixer_tables(void) {
    for (int n = 0; n < 31; n++) {
        if (n == 0)
            pulse_table[n] = 0.0f;
        else
            pulse_table[n] = 95.52f / (8128.0f / n + 100.0f);
    }
    for (int n = 0; n < 203; n++) {
        if (n == 0)
            tnd_table[n] = 0.0f;
        else
            tnd_table[n] = 163.67f / (24329.0f / n + 100.0f);
    }
}

static unsigned char registers[APU_REGISTER_SIZE];
static unsigned char dmc_regs[4];
static bool          channel_enable[5] = {false};
static unsigned char length_counter[4] = {0};

static const unsigned char length_table[32] = {
    10, 254, 20, 2, 40, 4, 80, 6, 160, 8, 60, 10, 14, 12, 26, 14,
    12, 16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30};

static double phase[3] = {0};

static struct {
    bool          start;
    bool          loop;
    bool          constant;
    unsigned char divider;
    unsigned char decay;
    unsigned char volume;
} envelope[3];

static struct {
    bool          enabled;
    bool          negate;
    unsigned char shift;
    unsigned char divider;
    unsigned char period;
    bool          reload;
    bool          mute;
} sweep[2];

static unsigned short pulse_timer[2] = {0};

static struct {
    bool          control;
    unsigned char reload_val;
    unsigned char counter;
    bool          reload;
} linear;

static struct {
    unsigned short lfsr;
    bool           mode;
} noise_ch;

static double noise_cycle_accum = 0.0;

static unsigned int  frame_cycle_accum = 0;
static unsigned char frame_step        = 0;
static bool          frame_5step       = false;
static bool          frame_irq_inhibit = false;
static bool          frame_irq         = false;

#define HP90_COEF  0.98727f
#define HP440_COEF 0.9393f
#define LP14K_COEF 0.864f
static float hp90_prev_out = 0.0f;
static float hp90_prev_in  = 0.0f;
static float hp440_prev_out = 0.0f;
static float hp440_prev_in  = 0.0f;
static float lp14k_prev_out = 0.0f;

static unsigned short dmc_sample_addr;
static unsigned short dmc_sample_len;
static unsigned short dmc_bytes_remaining;
static unsigned char  dmc_shift_register;
static unsigned char  dmc_bits_remaining = 0;
static unsigned char  dmc_output_level;
static bool           dmc_silence     = true;
static double         dmc_cycle_accum = 0.0;
static bool           dmc_irq_enable  = false;
static bool           dmc_irq_flag    = false;

// -------------------- FRAME COUNTER --------------------

static void clock_envelopes(void);
static void clock_sweeps(void);
static void clock_length_counters(void);
static void clock_linear_counter(void);

static void frame_counter_step(void) {
    switch (frame_step) {
    case 0:
        if (frame_5step) {
            clock_envelopes();
            clock_length_counters();
        } else {
            clock_envelopes();
        }
        break;
    case 1:
        if (frame_5step) {
            clock_envelopes();
            clock_sweeps();
        } else {
            clock_envelopes();
            clock_sweeps();
            clock_length_counters();
        }
        break;
    case 2:
        if (frame_5step) {
            clock_envelopes();
            clock_length_counters();
        } else {
            clock_envelopes();
        }
        break;
    case 3:
        if (frame_5step) {
            // nothing
        } else {
            clock_envelopes();
            clock_sweeps();
            clock_length_counters();
            clock_linear_counter();
            if (!frame_irq_inhibit)
                frame_irq = true;
        }
        break;
    case 4:
        // 5-step only
        clock_envelopes();
        clock_sweeps();
        clock_length_counters();
        clock_linear_counter();
        break;
    }

    if (frame_5step)
        frame_step = (frame_step + 1) % 5;
    else
        frame_step = (frame_step + 1) % 4;
}

static void clock_envelopes(void) {
    for (int i = 0; i < 3; i++) {
        if (envelope[i].start) {
            envelope[i].start   = false;
            envelope[i].decay   = 15;
            envelope[i].divider = envelope[i].volume;
        } else {
            if (envelope[i].divider > 0) {
                envelope[i].divider--;
            } else {
                envelope[i].divider = envelope[i].volume;
                if (envelope[i].decay > 0) {
                    envelope[i].decay--;
                } else if (envelope[i].loop) {
                    envelope[i].decay = 15;
                }
            }
        }
    }
}

static void clock_sweeps(void) {
    for (int ch = 0; ch < 2; ch++) {
        if (sweep[ch].reload) {
            sweep[ch].divider = sweep[ch].period;
            sweep[ch].reload  = false;
        } else if (sweep[ch].divider > 0) {
            sweep[ch].divider--;
        } else {
            sweep[ch].divider = sweep[ch].period;
            if (sweep[ch].enabled && sweep[ch].shift > 0) {
                unsigned short shifted = pulse_timer[ch] >> sweep[ch].shift;
                unsigned short target;
                if (sweep[ch].negate) {
                    if (ch == 0) {
                        target = pulse_timer[ch] + (unsigned short)(~shifted);
                    } else {
                        target = pulse_timer[ch] - shifted;
                    }
                } else {
                    target = pulse_timer[ch] + shifted;
                }
                if (target < 8 || target > 0x7FF) {
                    sweep[ch].mute = true;
                } else {
                    pulse_timer[ch] = target;
                    sweep[ch].mute  = false;
                }
            }
        }
    }
}

static void clock_length_counters(void) {
    for (int i = 0; i < 2; i++) {
        unsigned char halt = (registers[i * 4] >> 5) & 1;
        if (!halt && length_counter[i] > 0)
            length_counter[i]--;
    }
    {
        unsigned char halt = (registers[0x08] >> 7) & 1;
        if (!halt && length_counter[2] > 0)
            length_counter[2]--;
    }
    {
        unsigned char halt = (registers[0x0C] >> 5) & 1;
        if (!halt && length_counter[3] > 0)
            length_counter[3]--;
    }
}

static void clock_linear_counter(void) {
    if (linear.reload) {
        linear.counter = linear.reload_val;
    } else if (linear.counter > 0) {
        linear.counter--;
    }
    if (!linear.control) {
        linear.reload = false;
    }
}

// -------------------- CHANNEL GENERATORS --------------------

static float pulse_sample(int pulse_index) {
    if (!channel_enable[pulse_index] || length_counter[pulse_index] == 0) {
        phase[pulse_index] = 0.0;
        return 0.0f;
    }

    if (pulse_timer[pulse_index] < 8 || sweep[pulse_index].mute)
        return 0.0f;

    int base = pulse_index * 4;

    unsigned char duty = (registers[base] >> 6) & 0x03;
    unsigned char vol;
    if (envelope[pulse_index].constant) {
        vol = registers[base] & 0x0F;
    } else {
        vol = envelope[pulse_index].decay;
    }

    double freq      = (double)CPU_CLOCK_SPEED / (16.0 * (pulse_timer[pulse_index] + 1));
    double phase_inc = freq / SAMPLING_RATE;

    int   step = (int)(phase[pulse_index] * 8.0) & 7;

    phase[pulse_index] += phase_inc;
    if (phase[pulse_index] >= 1.0)
        phase[pulse_index] -= 1.0;

    return duty_table[duty][step] ? (float)vol : 0.0f;
}

static float triangle_sample(void) {
    if (!channel_enable[2] || length_counter[2] == 0 || linear.counter == 0) {
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

    phase[2] += phase_inc;
    if (phase[2] >= 1.0)
        phase[2] -= 1.0;

    return (float)triangle_table[step];
}

static float noise_sample(void) {
    if (!channel_enable[3] || length_counter[3] == 0)
        return 0.0f;

    unsigned char  period_index = registers[0x0E] & 0x0F;
    unsigned short period       = noise_period_table[period_index];

    if (period == 0)
        return 0.0f;

    double cpu_cycles_per_sample = (double)CPU_CLOCK_SPEED / SAMPLING_RATE;
    noise_cycle_accum += cpu_cycles_per_sample;

    while (noise_cycle_accum >= period) {
        noise_cycle_accum -= period;
        unsigned char feedback = (noise_ch.lfsr & 1) ^
                                 ((noise_ch.lfsr >> (noise_ch.mode ? 6 : 1)) & 1);
        noise_ch.lfsr >>= 1;
        noise_ch.lfsr |= (unsigned short)(feedback << 14);
        noise_ch.lfsr &= 0x7FFF;
    }

    unsigned char vol;
    if (envelope[2].constant) {
        vol = registers[0x0C] & 0x0F;
    } else {
        vol = envelope[2].decay;
    }

    return (noise_ch.lfsr & 1) ? 0.0f : (float)vol;
}

static float dmc_sample(void) {
    if (channel_enable[4]) {
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
                        if (dmc_irq_enable) {
                            dmc_irq_flag = true;
                            trigger_irq();
                        }
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
    }

    return (float)dmc_output_level;
}

// -------------------- MASTER MIXER --------------------

static int apu_calls_with_the_same_pc = 0;
static int last_pc_value              = 0;

void apu_mix_samples(float *buffer, unsigned int frames) {
    if (get_pc() == last_pc_value)
        apu_calls_with_the_same_pc += 1;
    else
        apu_calls_with_the_same_pc = 0;
    bool should_be_silent = apu_calls_with_the_same_pc >= 10;

    float *out = buffer;
    if (should_be_silent) {
        memset(out, 0, frames * sizeof(float));
        hp90_prev_out  = 0.0f;
        hp90_prev_in   = 0.0f;
        hp440_prev_out = 0.0f;
        hp440_prev_in  = 0.0f;
        lp14k_prev_out = 0.0f;
        return;
    }

    for (unsigned int i = 0; i < frames; i++) {
        float p1 = pulse_sample(0);
        float p2 = pulse_sample(1);
        float tri = triangle_sample();
        float ns  = noise_sample();
        float dmc = dmc_sample();

        unsigned short pulse_sum = (unsigned short)(p1 + p2);
        float pulse_out = pulse_sum ? pulse_table[pulse_sum] : 0.0f;

        unsigned short tnd_sum = (unsigned short)(3.0f * tri + 2.0f * ns + dmc);
        float tnd_out = tnd_sum ? tnd_table[tnd_sum] : 0.0f;

        float s = pulse_out + tnd_out;

        hp90_prev_out  = HP90_COEF * (hp90_prev_out + s - hp90_prev_in);
        hp90_prev_in   = s;
        s               = hp90_prev_out;

        hp440_prev_out = HP440_COEF * (hp440_prev_out + s - hp440_prev_in);
        hp440_prev_in  = s;
        s               = hp440_prev_out;

        lp14k_prev_out += LP14K_COEF * (s - lp14k_prev_out);
        s               = lp14k_prev_out;

        out[i] = s;
    }
    last_pc_value = get_pc();
}

// -------------------- PUBLIC API --------------------

void boot_nes_audio() {
    connect_apu_to_bus(read_apu, write_apu);
    init_mixer_tables();

    memset(registers, 0, APU_REGISTER_SIZE);
    memset(dmc_regs, 0, 4);
    for (int i = 0; i < 5; i++)
        channel_enable[i] = false;
    for (int i = 0; i < 4; i++)
        length_counter[i] = 0;
    phase[0] = phase[1] = phase[2] = 0.0;

    for (int i = 0; i < 3; i++) {
        envelope[i].start    = false;
        envelope[i].loop     = false;
        envelope[i].constant = false;
        envelope[i].divider  = 0;
        envelope[i].decay    = 0;
        envelope[i].volume   = 0;
    }

    for (int i = 0; i < 2; i++) {
        sweep[i].enabled = false;
        sweep[i].negate  = false;
        sweep[i].shift   = 0;
        sweep[i].divider = 0;
        sweep[i].period  = 0;
        sweep[i].reload  = false;
        sweep[i].mute    = false;
        pulse_timer[i]   = 0;
    }

    linear.control     = false;
    linear.reload_val  = 0;
    linear.counter     = 0;
    linear.reload      = false;

    noise_ch.lfsr      = 1;
    noise_ch.mode      = false;
    noise_cycle_accum  = 0.0;

    frame_cycle_accum  = 0;
    frame_step         = 0;
    frame_5step        = false;
    frame_irq_inhibit  = false;
    frame_irq          = false;

    dmc_sample_addr     = 0;
    dmc_sample_len      = 0;
    dmc_bytes_remaining = 0;
    dmc_shift_register  = 0;
    dmc_bits_remaining  = 0;
    dmc_output_level    = 0;
    dmc_silence         = true;
    dmc_cycle_accum     = 0.0;
    dmc_irq_enable      = false;
    dmc_irq_flag        = false;

    hp90_prev_out  = 0.0f;
    hp90_prev_in   = 0.0f;
    hp440_prev_out = 0.0f;
    hp440_prev_in  = 0.0f;
    lp14k_prev_out = 0.0f;
}

unsigned char read_apu(int addr) {
    if (addr == 0x4015) {
        unsigned char status = 0;
        status |= channel_enable[0] ? 0x01 : 0;
        status |= channel_enable[1] ? 0x02 : 0;
        status |= channel_enable[2] ? 0x04 : 0;
        status |= channel_enable[3] ? 0x08 : 0;
        status |= channel_enable[4] ? 0x10 : 0;
        if (frame_irq)
            status |= 0x40;
        if (dmc_irq_flag)
            status |= 0x80;
        frame_irq = false;
        return status;
    }
    size_t index = addr - 0x4000;
    if (index >= APU_REGISTER_SIZE)
        return 0;
    return registers[index];
}

void write_apu(int addr, unsigned char value) {
    if (addr == 0x4015) {
        channel_enable[0] = (value & 0x01) != 0;
        channel_enable[1] = (value & 0x02) != 0;
        channel_enable[2] = (value & 0x04) != 0;
        channel_enable[3] = (value & 0x08) != 0;
        channel_enable[4] = (value & 0x10) != 0;
        if (!channel_enable[0])
            length_counter[0] = 0;
        if (!channel_enable[1])
            length_counter[1] = 0;
        if (!channel_enable[2])
            length_counter[2] = 0;
        if (!channel_enable[3])
            length_counter[3] = 0;
        return;
    }

    if (addr == 0x4017) {
        frame_5step       = (value >> 7) & 1;
        frame_irq_inhibit = (value >> 6) & 1;
        if (!frame_irq_inhibit)
            frame_irq = false;
        if (frame_5step) {
            clock_envelopes();
            clock_length_counters();
        }
        return;
    }

    if (addr >= 0x4000 && addr <= 0x4003) {
        int i = 0;
        if (addr == 0x4000) {
            envelope[0].loop     = (value >> 5) & 1;
            envelope[0].constant = (value >> 4) & 1;
            envelope[0].volume   = value & 0x0F;
        } else if (addr == 0x4001) {
            sweep[0].enabled = (value >> 7) & 1;
            sweep[0].period  = ((value >> 4) & 0x07) + 1;
            sweep[0].negate  = (value >> 3) & 1;
            sweep[0].shift   = value & 0x07;
            sweep[0].reload  = true;
        } else if (addr == 0x4002) {
            pulse_timer[0] = (pulse_timer[0] & 0x700) | value;
        } else if (addr == 0x4003) {
            length_counter[0] = length_table[(value >> 3) & 0x1F];
            pulse_timer[0]    = (pulse_timer[0] & 0xFF) | ((unsigned short)(value & 0x07) << 8);
            phase[0]          = 0.0;
            envelope[0].start = true;
        }
        registers[addr - 0x4000] = value;
        return;
    }

    if (addr >= 0x4004 && addr <= 0x4007) {
        if (addr == 0x4004) {
            envelope[1].loop     = (value >> 5) & 1;
            envelope[1].constant = (value >> 4) & 1;
            envelope[1].volume   = value & 0x0F;
        } else if (addr == 0x4005) {
            sweep[1].enabled = (value >> 7) & 1;
            sweep[1].period  = ((value >> 4) & 0x07) + 1;
            sweep[1].negate  = (value >> 3) & 1;
            sweep[1].shift   = value & 0x07;
            sweep[1].reload  = true;
        } else if (addr == 0x4006) {
            pulse_timer[1] = (pulse_timer[1] & 0x700) | value;
        } else if (addr == 0x4007) {
            length_counter[1] = length_table[(value >> 3) & 0x1F];
            pulse_timer[1]    = (pulse_timer[1] & 0xFF) | ((unsigned short)(value & 0x07) << 8);
            phase[1]          = 0.0;
            envelope[1].start = true;
        }
        registers[addr - 0x4000] = value;
        return;
    }

    if (addr == 0x4008) {
        linear.control    = (value >> 7) & 1;
        linear.reload_val = value & 0x7F;
        registers[0x08]   = value;
        return;
    }

    if (addr == 0x400A) {
        registers[0x0A] = value;
        return;
    }

    if (addr == 0x400B) {
        length_counter[2] = length_table[(value >> 3) & 0x1F];
        linear.reload     = true;
        registers[0x0B]   = value;
        return;
    }

    if (addr == 0x400C) {
        envelope[2].loop     = (value >> 5) & 1;
        envelope[2].constant = (value >> 4) & 1;
        envelope[2].volume   = value & 0x0F;
        registers[0x0C]      = value;
        return;
    }

    if (addr == 0x400E) {
        noise_ch.mode   = (value >> 7) & 1;
        registers[0x0E] = value;
        return;
    }

    if (addr == 0x400F) {
        length_counter[3] = length_table[(value >> 3) & 0x1F];
        envelope[2].start = true;
        registers[0x0F]   = value;
        return;
    }

    if (addr >= 0x4010 && addr <= 0x4013) {
        int i       = addr - 0x4010;
        dmc_regs[i] = value;

        if (addr == 0x4010) {
            dmc_irq_enable = (value >> 7) & 1;
            if (!dmc_irq_enable)
                dmc_irq_flag = false;
        } else if (addr == 0x4011) {
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
    int         current      = get_elapsed_clock_cycles();
    int         delta        = current - apu_last_cycles;
    const unsigned int step_cycles = CPU_CLOCK_SPEED / 240;

    if (delta <= 0)
        return;

    frame_cycle_accum += delta;
    apu_last_cycles    = current;

    while (frame_cycle_accum >= step_cycles) {
        frame_cycle_accum -= step_cycles;
        frame_counter_step();
    }
}

static void apu_save_state(Save_State_Info *save_buffer, uint32_t allowable_content_length) {
    ApuSaveState state;
    memset(&state, 0, sizeof(state));
    memcpy(state.registers, registers, sizeof(registers));
    memcpy(state.dmc_regs, dmc_regs, sizeof(dmc_regs));
    memcpy(state.channel_enable, channel_enable, sizeof(channel_enable));
    memcpy(state.length_counter, length_counter, sizeof(length_counter));
    memcpy(state.phase, phase, sizeof(phase));
    state.dmc_sample_addr     = dmc_sample_addr;
    state.dmc_sample_len      = dmc_sample_len;
    state.dmc_bytes_remaining = dmc_bytes_remaining;
    state.dmc_shift_register  = dmc_shift_register;
    state.dmc_bits_remaining  = dmc_bits_remaining;
    state.dmc_output_level    = dmc_output_level;
    state.dmc_silence         = dmc_silence;
    state.dmc_cycle_accum     = dmc_cycle_accum;
    state.last_cycles         = apu_last_cycles;

    for (int i = 0; i < 3; i++) {
        state.envelope_start[i]    = envelope[i].start;
        state.envelope_loop[i]     = envelope[i].loop;
        state.envelope_constant[i] = envelope[i].constant;
        state.envelope_divider[i]  = envelope[i].divider;
        state.envelope_decay[i]    = envelope[i].decay;
        state.envelope_volume[i]   = envelope[i].volume;
    }
    for (int i = 0; i < 2; i++) {
        state.sweep_enabled[i]  = sweep[i].enabled;
        state.sweep_negate[i]   = sweep[i].negate;
        state.sweep_shift[i]    = sweep[i].shift;
        state.sweep_divider[i]  = sweep[i].divider;
        state.sweep_period[i]   = sweep[i].period;
        state.sweep_reload[i]   = sweep[i].reload;
        state.sweep_mute[i]     = sweep[i].mute;
        state.pulse_timer[i]    = pulse_timer[i];
    }
    state.linear_control    = linear.control;
    state.linear_reload_val = linear.reload_val;
    state.linear_counter    = linear.counter;
    state.linear_reload     = linear.reload;

    state.noise_lfsr       = noise_ch.lfsr;
    state.noise_mode       = noise_ch.mode;
    state.noise_cycle_accum = noise_cycle_accum;

    state.frame_cycle_accum  = frame_cycle_accum;
    state.frame_step         = frame_step;
    state.frame_5step        = frame_5step;
    state.frame_irq_inhibit  = frame_irq_inhibit;
    state.frame_irq          = frame_irq;

    state.dmc_irq_enable = dmc_irq_enable;
    state.dmc_irq_flag   = dmc_irq_flag;

    state.hp90_prev_out  = hp90_prev_out;
    state.hp90_prev_in   = hp90_prev_in;
    state.hp440_prev_out = hp440_prev_out;
    state.hp440_prev_in  = hp440_prev_in;
    state.lp14k_prev_out = lp14k_prev_out;

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

    for (int i = 0; i < 3; i++) {
        envelope[i].start    = state.envelope_start[i];
        envelope[i].loop     = state.envelope_loop[i];
        envelope[i].constant = state.envelope_constant[i];
        envelope[i].divider  = state.envelope_divider[i];
        envelope[i].decay    = state.envelope_decay[i];
        envelope[i].volume   = state.envelope_volume[i];
    }
    for (int i = 0; i < 2; i++) {
        sweep[i].enabled = state.sweep_enabled[i];
        sweep[i].negate  = state.sweep_negate[i];
        sweep[i].shift   = state.sweep_shift[i];
        sweep[i].divider = state.sweep_divider[i];
        sweep[i].period  = state.sweep_period[i];
        sweep[i].reload  = state.sweep_reload[i];
        sweep[i].mute    = state.sweep_mute[i];
        pulse_timer[i]   = state.pulse_timer[i];
    }
    linear.control     = state.linear_control;
    linear.reload_val  = state.linear_reload_val;
    linear.counter     = state.linear_counter;
    linear.reload      = state.linear_reload;

    noise_ch.lfsr       = state.noise_lfsr;
    noise_ch.mode       = state.noise_mode;
    noise_cycle_accum   = state.noise_cycle_accum;

    frame_cycle_accum = state.frame_cycle_accum;
    frame_step        = state.frame_step;
    frame_5step       = state.frame_5step;
    frame_irq_inhibit = state.frame_irq_inhibit;
    frame_irq         = state.frame_irq;

    dmc_irq_enable = state.dmc_irq_enable;
    dmc_irq_flag   = state.dmc_irq_flag;

    hp90_prev_out  = state.hp90_prev_out;
    hp90_prev_in   = state.hp90_prev_in;
    hp440_prev_out = state.hp440_prev_out;
    hp440_prev_in  = state.hp440_prev_in;
    lp14k_prev_out = state.lp14k_prev_out;
}

REGISTER_SAVE_STATE("APU", apu_save_state, apu_load_state);
