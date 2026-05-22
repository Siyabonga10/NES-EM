#include "dev.h"
#include <imgui.h>
#include <stdio.h>
#include "core_bindings.h"

static int   bp_addr       = 0;
static bool  step_requested = false;
static int   prev_pc        = -1;
static char  bp_buf[8]      = {0};
static int   cycle_target  = 0;
static bool  cycle_break_on = false;
static char  cycle_buf[16]  = {0};

int get_bp_addr(void) { return bp_addr; }
int get_cycle_target(void) { return cycle_target; }
bool get_cycle_break_on(void) { return cycle_break_on; }

static void init(void) {}

static void cpu_tick(ControllerKeyStates *keys) {
    static ControllerKeyStates zero = {0};
    tick_cpu_once(keys ? keys : &zero);
}

static void update(void) {
    if (!rom_loaded) {
        debug_paused = false;
        step_requested = false;
        return;
    }

    if (!debug_paused) return;

    if (!step_requested) return;

    step_requested = false;
    int start_pc = get_pc();
    do {
        cpu_tick(NULL);
    } while (get_pc() == start_pc);

    FrameData *f = request_frame();
    if (f && f->is_new_frame) {
        SDL_Texture *gt = get_game_texture();
        if (gt) SDL_UpdateTexture(gt, NULL, f->data, 256 * (int)sizeof(NesColor));
        f->is_new_frame = false;
    }
    prev_pc = get_pc();
}

static void render(void) {
    ImGui::Begin("Debug", NULL, ImGuiWindowFlags_AlwaysAutoResize);

    if (rom_loaded) {
        int pc  = get_pc();
        int a   = read_byte(get_cpu_accumulator());
        int x   = read_byte(get_cpu_x_register());
        int y   = read_byte(get_cpu_y_register());
        int sp  = read_byte(get_cpu_stack());
        int sr  = read_byte(get_cpu_status_register());

        ImGui::Text("PC:  $%04X", pc);
        ImGui::Text("A:   $%02X", a);
        ImGui::Text("X:   $%02X", x);
        ImGui::Text("Y:   $%02X", y);
        ImGui::Text("SP:  $%02X", sp);
        ImGui::Text("SR:  $%02X  [%c%c--%c%c%c%c]",
                    sr,
                    (sr & 0x80) ? 'N' : 'n',
                    (sr & 0x40) ? 'V' : 'v',
                    (sr & 0x08) ? 'D' : 'd',
                    (sr & 0x04) ? 'I' : 'i',
                    (sr & 0x02) ? 'Z' : 'z',
                    (sr & 0x01) ? 'C' : 'c');

        ImGui::Separator();

        int addr = pc;
        for (int line = 0; line < 6; line++) {
            int  op   = read_byte(addr);
            ExecutionInfo info = get_instruction_info(op);
            if (line == 0)
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.0f, 1.0f));
            ImGui::Text("$%04X  ", addr);
            ImGui::SameLine();
            for (int i = 0; i < info.instruction_size; i++) {
                ImGui::Text("%02X ", read_byte(addr + i));
                ImGui::SameLine();
            }
            for (int i = info.instruction_size; i < 3; i++) {
                ImGui::Text("   ");
                ImGui::SameLine();
            }
            ImGui::Text("%s ", info.name);
            int ea = info.addressing_mode(addr + 1);
            if (ea >= 0) {
                ImGui::SameLine();
                ImGui::Text("$%04X", ea);
            }
            if (line == 0)
                ImGui::PopStyleColor();
            addr += info.instruction_size;
        }
    }

    ImGui::InputText("Cycles", cycle_buf, sizeof(cycle_buf),
                     ImGuiInputTextFlags_CharsDecimal);
    ImGui::SameLine();
    if (ImGui::Button("Break at")) {
        sscanf(cycle_buf, "%d", &cycle_target);
        cycle_break_on = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Clear Break")) {
        cycle_break_on = false;
    }
    if (cycle_break_on) {
        ImGui::Text("Break at: %d (now: %d)", cycle_target, rom_loaded ? get_elapsed_clock_cycles() : 0);
    }

    ImGui::Separator();

    ImGui::InputText("BP", bp_buf, sizeof(bp_buf),
                     ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_AutoSelectAll);
    ImGui::SameLine();
    if (ImGui::Button("Set")) {
        sscanf(bp_buf, "%x", &bp_addr);
        bp_addr &= 0xFFFF;
    }
    ImGui::SameLine();
    if (ImGui::Button("Clear")) {
        bp_addr = 0;
        bp_buf[0] = 0;
    }

    ImGui::Separator();

    if (debug_paused) {
        if (ImGui::Button("Step Over")) {
            step_requested = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Run")) {
            debug_paused = false;
        }
    } else {
        if (rom_loaded && ImGui::Button("Pause")) {
            debug_paused = true;
            step_requested = false;
        }
    }

    ImGui::End();
}

static void shutdown(void) {}

static DebugModule _mod = {
    "Debug", init, update, render, shutdown, false, false
};
REGISTER_DEBUG_MODULE(_mod)
