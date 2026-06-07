let nesModule = undefined;
let gamePaused = false;
let audio = undefined;
let romData = null;
let romFileName = '';

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
    const ptr = nesModule._tick_cpu(keyStatesPtr);
    nesModule._update_apu();
    drawPixels(nesModule.HEAPU32[ptr / 4 + 3]);
    requestAnimationFrame(renderFrame);
};

const initNES = (data) => {
    initialiseNES().then(nes => {
        const ptr = nes._nes_alloc(data.byteLength);
        nes.HEAPU8.set(data, ptr);
        nesModule = nes;
        romData = new Uint8Array(data);

        if (window.controls && window.controls.setNesModule) {
            window.controls.setNesModule(nes);
        }

        nes._load_cartridge_and_connect_to_bus(ptr, data.byteLength);
        nes._boot_nes_audio();

        audio = new NesAudio(nes);
        audio.init();
        audio.resume();

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
        romFileName = file.name;
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

    document.addEventListener('keydown', (event) => {
        if (event.key === '=' || event.key === '+') {
            event.preventDefault();
            if (audio) audio.setVolume(Math.min(audio.volume + 0.05, 1.0));
        } else if (event.key === '-') {
            event.preventDefault();
            if (audio) audio.setVolume(Math.max(audio.volume - 0.05, 0.0));
        }
    });

    document.getElementById('saveStateBtn').addEventListener('click', () => {
        if (!nesModule || !romData) return;
        const BUF_SIZE = 1024 * 1024;
        const ptr = nesModule._nes_alloc(BUF_SIZE);
        const written = nesModule._save_state(ptr, BUF_SIZE);
        if (written === 0) { nesModule._nes_dealloc(ptr); return; }

        const stateBytes = nesModule.HEAPU8.slice(ptr, ptr + written);
        const totalSize = 4 + romData.length + stateBytes.length;
        const blob = new Uint8Array(totalSize);
        blob[0] = romData.length & 0xFF;
        blob[1] = (romData.length >> 8) & 0xFF;
        blob[2] = (romData.length >> 16) & 0xFF;
        blob[3] = (romData.length >> 24) & 0xFF;
        blob.set(romData, 4);
        blob.set(stateBytes, 4 + romData.length);

        let b64 = '';
        for (let i = 0; i < blob.length; i++)
            b64 += String.fromCharCode(blob[i]);
        localStorage.setItem('nes_save_' + romFileName, btoa(b64));
        nesModule._nes_dealloc(ptr);
    });

    document.getElementById('loadStateBtn').addEventListener('click', () => {
        if (!nesModule || !romFileName) return;
        const key = 'nes_save_' + romFileName;
        const b64 = localStorage.getItem(key);
        if (!b64) return;

        const raw = atob(b64);
        const blob = new Uint8Array(raw.length);
        for (let i = 0; i < raw.length; i++)
            blob[i] = raw.charCodeAt(i);

        const romLen = blob[0] | (blob[1] << 8) | (blob[2] << 16) | (blob[3] << 24);
        const stateLen = blob.length - 4 - romLen;

        const romPtr = nesModule._nes_alloc(romLen);
        const statePtr = nesModule._nes_alloc(stateLen);
        nesModule.HEAPU8.set(blob.subarray(4, 4 + romLen), romPtr);
        nesModule.HEAPU8.set(blob.subarray(4 + romLen), statePtr);

        nesModule._load_state(romPtr, romLen, statePtr, stateLen);
        nesModule._nes_dealloc(romPtr);
        nesModule._nes_dealloc(statePtr);
    });
};
