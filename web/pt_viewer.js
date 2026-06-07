let ptVisible = false;
let ptCtx = null;
let ptImageData = null;

function initPatternTableViewer() {
    const canvas = document.getElementById('ptCanvas');
    canvas.width = 16 * 8 * 2;
    canvas.height = 16 * 8 + 24;
    ptCtx = canvas.getContext('2d');
    ptImageData = ptCtx.createImageData(canvas.width, canvas.height);
}

function renderPatternTableViewer() {
    if (!ptVisible || !nesModule) return;

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

    const data = ptImageData.data;
    const pitch = 16 * 8 * 2;

    function renderTable(tableBase, offsetX) {
        for (let tile = 0; tile < 256; tile++) {
            const tx = (tile % 16) * 8 + offsetX;
            const ty = Math.floor(tile / 16) * 8;
            for (let row = 0; row < 8; row++) {
                const low = nesModule._read_byte_ppu(tableBase + tile * 16 + row) & 0xFF;
                const high = nesModule._read_byte_ppu(tableBase + tile * 16 + 8 + row) & 0xFF;
                for (let col = 0; col < 8; col++) {
                    const bit = 7 - col;
                    const color = ((low >> bit) & 1) | (((high >> bit) & 1) << 1);
                    const idx = nesModule._read_palette_ram(color) & 0x3F;
                    const c = sysPalette[idx];
                    const pi = ((ty + row) * pitch + (tx + col)) * 4;
                    data[pi] = c.r;
                    data[pi + 1] = c.g;
                    data[pi + 2] = c.b;
                    data[pi + 3] = c.a;
                }
            }
        }
    }

    renderTable(0x0000, 0);
    renderTable(0x1000, 16 * 8);

    const palY = 16 * 8 + 4;
    for (let i = 0; i < 32; i++) {
        const idx = nesModule._read_palette_ram(i) & 0x3F;
        const c = sysPalette[idx];
        const sx = i * 8;
        for (let r = 0; r < 16; r++) {
            for (let col = 0; col < 8; col++) {
                const pi = ((palY + r) * pitch + (sx + col)) * 4;
                data[pi] = c.r;
                data[pi + 1] = c.g;
                data[pi + 2] = c.b;
                data[pi + 3] = 255;
            }
        }
    }

    ptCtx.putImageData(ptImageData, 0, 0);
}
