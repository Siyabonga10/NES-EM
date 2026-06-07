let ntVisible = false;
let ntCtx = null;
let ntImageData = null;

function initNametableViewer() {
    const canvas = document.getElementById('ntCanvas');
    canvas.width = 32 * 8 * 2;
    canvas.height = 30 * 8 * 2;
    ntCtx = canvas.getContext('2d');
    ntImageData = ntCtx.createImageData(canvas.width, canvas.height);
}

function renderNametableViewer() {
    if (!ntVisible || !nesModule) return;

    const spPtr = nesModule._get_system_palette();
    const spBytes = nesModule.HEAPU8.subarray(spPtr, spPtr + 64 * 4);
    const sysPalette = [];
    for (let i = 0; i < 64; i++) {
        sysPalette[i] = {
            r: spBytes[i * 4],
            g: spBytes[i * 4 + 1],
            b: spBytes[i * 4 + 2],
            a: spBytes[i * 4 + 3]
        };
    }

    const w = 32 * 8 * 2;
    const ntBases = [0x2000, 0x2400, 0x2800, 0x2C00];
    const data = ntImageData.data;

    for (let nt = 0; nt < 4; nt++) {
        const ox = (nt % 2) * 32 * 8;
        const oy = Math.floor(nt / 2) * 30 * 8;
        const base = ntBases[nt];

        for (let row = 0; row < 30; row++) {
            for (let col = 0; col < 32; col++) {
                const tileIdx = nesModule._read_ppu_vram(base + row * 32 + col) & 0xFF;
                const ar = Math.floor(row / 4);
                const ac = Math.floor(col / 4);
                const attr = nesModule._read_ppu_vram(base + 0x3C0 + ar * 8 + ac) & 0xFF;
                const sr = Math.floor((row % 4) / 2);
                const sc = Math.floor((col % 4) / 2);
                const palette = (attr >> ((sr * 2 + sc) * 2)) & 0x03;

                for (let tr = 0; tr < 8; tr++) {
                    const low = nesModule._read_byte_ppu(tileIdx * 16 + tr) & 0xFF;
                    const high = nesModule._read_byte_ppu(tileIdx * 16 + 8 + tr) & 0xFF;
                    for (let tc = 0; tc < 8; tc++) {
                        const bit = 7 - tc;
                        const color = ((low >> bit) & 1) | (((high >> bit) & 1) << 1);
                        const idx = nesModule._read_palette_ram(palette * 4 + color) & 0x3F;
                        const c = sysPalette[idx];
                        const dx = ox + col * 8 + tc;
                        const dy = oy + row * 8 + tr;
                        const pi = (dy * w + dx) * 4;
                        data[pi] = c.r;
                        data[pi + 1] = c.g;
                        data[pi + 2] = c.b;
                        data[pi + 3] = c.a;
                    }
                }
            }
        }
    }

    ntCtx.putImageData(ntImageData, 0, 0);
}
