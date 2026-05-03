let nesModule = undefined;
let gamePaused = false;

const NES_WIDTH = 256;
const NES_HEIGHT = 240;
const CLIP_TOP = 8;
const CLIP_BOT = 8;
const VISIBLE_ROWS = NES_HEIGHT - CLIP_TOP - CLIP_BOT;
const SCALE = 3;
const BYTES_PER_PIXEL = 4;

var keyStatesPtr;

const canvas = document.getElementById("myCanvas");
const ctx = canvas.getContext("2d");
ctx.imageSmoothingEnabled = false;

const offscreen = document.createElement('canvas');
offscreen.width = NES_WIDTH;
offscreen.height = VISIBLE_ROWS;
const offCtx = offscreen.getContext('2d');
const imageData = offCtx.createImageData(NES_WIDTH, VISIBLE_ROWS);

const drawPixels = (ptr) => {
    const skip = CLIP_TOP * NES_WIDTH * BYTES_PER_PIXEL;
    const len = VISIBLE_ROWS * NES_WIDTH * BYTES_PER_PIXEL;
    imageData.data.set(nesModule.HEAPU8.subarray(ptr + skip, ptr + skip + len));
    offCtx.putImageData(imageData, 0, 0);
    ctx.drawImage(offscreen, 0, 0, NES_WIDTH * SCALE, VISIBLE_ROWS * SCALE);
};

const renderFrame = () => {
    if (gamePaused) return;
    ctx.fillStyle = '#000';
    ctx.fillRect(0, 0, canvas.width, canvas.height);
    const ptr = nesModule._tick_cpu(keyStatesPtr);
    drawPixels(nesModule.HEAPU32[ptr / 4 + 3]);
    requestAnimationFrame(renderFrame);
};

const initNES = (data) => {
    initialiseNES().then(nes => {
        const ptr = nes._nes_alloc(data.byteLength);
        nes.HEAPU8.set(data, ptr);
        nesModule = nes;

        if (window.controls && window.controls.setNesModule) {
            window.controls.setNesModule(nes);
        }

        nes._load_cartridge_and_connect_to_bus(ptr, data.byteLength);
        nes._boot_nes_audio();
        runGame(nes);
        nes._nes_dealloc(ptr);

        document.getElementById('noGameOverlay').style.display = 'none';
        document.getElementById('pauseBtn').textContent = 'Pause';
    });
};

const runGame = (nesModule) => {
    nesModule._connect_controller_to_console();
    nesModule._boot_ppu();
    nesModule._boot_cpu();
    keyStatesPtr = nesModule._nes_alloc(8);
    if (window.controls && window.controls.setKeyStatesPtr) {
        window.controls.setKeyStatesPtr(keyStatesPtr);
    }
    for (let i = 0; i < 8; i++) nesModule.HEAPU8[keyStatesPtr + i] = 0;
    requestAnimationFrame(renderFrame);
};

window.onload = function () {
    document.getElementById('fileInput').addEventListener('change', function (e) {
        const file = e.target.files[0];
        if (!file) return;
        const reader = new FileReader();
        reader.onload = function (e) {
            initNES(new Uint8Array(e.target.result));
        };
        reader.readAsArrayBuffer(file);
    });

    document.getElementById('pauseBtn').addEventListener('click', () => {
        gamePaused = !gamePaused;
        document.getElementById('pauseBtn').textContent = gamePaused ? 'Resume' : 'Pause';
    });

    const settingsModal = document.getElementById('settingsModal');
    document.getElementById('settingsBtn').addEventListener('click', () => {
        settingsModal.style.display = 'flex';
        if (window.controls && window.controls.populateKeyMappingModal) {
            window.controls.populateKeyMappingModal();
        }
    });

    document.getElementById('closeSettingsBtn').addEventListener('click', () => {
        settingsModal.style.display = 'none';
    });

    window.addEventListener('click', (e) => {
        if (e.target === settingsModal) settingsModal.style.display = 'none';
    });

    document.getElementById('resetKeysBtn').addEventListener('click', () => {
        if (confirm('Reset all key bindings to defaults?')) {
            if (window.controls && window.controls.initKeyMappings) {
                window.controls.initKeyMappings();
                window.controls.populateKeyMappingModal();
            }
        }
    });

    const canvasWrap = document.getElementById('canvasWrap');
    const touchToggle = document.getElementById('touchToggle');

    if (localStorage.getItem('touchControls') === 'true') {
        canvasWrap.classList.add('touch-on');
        document.body.classList.add('touch-mode');
        touchToggle.textContent = 'Touch: ON';
    }

    touchToggle.addEventListener('click', () => {
        const on = canvasWrap.classList.toggle('touch-on');
        document.body.classList.toggle('touch-mode', on);
        touchToggle.textContent = on ? 'Touch: ON' : 'Touch';
        localStorage.setItem('touchControls', on);
    });
};
