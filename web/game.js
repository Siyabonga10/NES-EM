let nesModule = undefined;
let gamePaused = false;
let isMuted = false;
let pixelationEnabled = true;

const no_of_rows = 240;
const no_of_cols = 256;
var keyStatesPtr;
const NUMBER_OF_INPUT_FIELDS = 8;

const canvas = document.getElementById("myCanvas");
const ctx = canvas.getContext("2d");
ctx.imageSmoothingEnabled = false;

const offscreen = document.createElement('canvas');
offscreen.width = no_of_cols;
offscreen.height = no_of_rows;
const offCtx = offscreen.getContext('2d');
offCtx.imageSmoothingEnabled = false;
const imageData = offCtx.createImageData(no_of_cols, no_of_rows);

const drawPixels = (ptr) => {
    const src = nesModule.HEAPU8.subarray(ptr, ptr + no_of_cols * no_of_rows * 4);
    imageData.data.set(src);
    offCtx.putImageData(imageData, 0, 0);
    
    if (pixelationEnabled) {
        ctx.imageSmoothingEnabled = false;
        ctx.mozImageSmoothingEnabled = false;
        ctx.webkitImageSmoothingEnabled = false;
        ctx.msImageSmoothingEnabled = false;
    } else {
        ctx.imageSmoothingEnabled = true;
        ctx.mozImageSmoothingEnabled = true;
        ctx.webkitImageSmoothingEnabled = true;
        ctx.msImageSmoothingEnabled = true;
    }
    ctx.drawImage(offscreen, 0, 0, no_of_cols * 3, no_of_rows * 3);
};

const renderFrame = () => {
    if (gamePaused) return;
    ctx.fillStyle = '#000000ff';
    ctx.fillRect(0, 0, canvas.width, canvas.height);
    const ptr = nesModule._tick_cpu(keyStatesPtr);
    const dataPtr = nesModule.HEAPU32[ptr / 4 + 3];
    drawPixels(dataPtr);
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
        const pauseBtn = document.getElementById('pauseBtn');
        pauseBtn.innerHTML = '<i class="fas fa-pause"></i> Pause';
        pauseBtn.dataset.paused = 'false';
    });
};

const runGame = (nesModule) => {
    nesModule._connect_controller_to_console();
    nesModule._boot_ppu();
    nesModule._boot_cpu();
    keyStatesPtr = nesModule._nes_alloc(NUMBER_OF_INPUT_FIELDS);
    
    if (window.controls && window.controls.setKeyStatesPtr) {
        window.controls.setKeyStatesPtr(keyStatesPtr);
    }
    
    for (let i = 0; i < 8; i++) {
        nesModule.HEAPU8[keyStatesPtr + i] = 0;
    }
    requestAnimationFrame(renderFrame);
};

window.onload = function () {
    loadSettings();
    
    const fileInput = document.getElementById('fileInput');
    fileInput.addEventListener('change', function (e) {
        const file = fileInput.files[0];
        const reader = new FileReader();
        reader.onload = function (e) {
            const data = new Uint8Array(e.target.result);
            initNES(data);
        };
        reader.readAsArrayBuffer(file);
    });
    
    document.getElementById('pauseBtn').addEventListener('click', () => {
        gamePaused = !gamePaused;
        const pauseBtn = document.getElementById('pauseBtn');
        if (gamePaused) {
            pauseBtn.innerHTML = '<i class="fas fa-play"></i> Resume';
        } else {
            pauseBtn.innerHTML = '<i class="fas fa-pause"></i> Pause';
        }
    });
    
    document.getElementById('muteBtn').addEventListener('click', () => {
        isMuted = !isMuted;
        const muteBtn = document.getElementById('muteBtn');
        if (isMuted) {
            muteBtn.innerHTML = '<i class="fas fa-volume-mute"></i> Mute';
        } else {
            muteBtn.innerHTML = '<i class="fas fa-volume-up"></i> Mute';
        }
        saveSettings();
    });
    
    const pixelationToggle = document.getElementById('pixelationToggle');
    pixelationToggle.checked = pixelationEnabled;
    pixelationToggle.addEventListener('change', () => {
        pixelationEnabled = pixelationToggle.checked;
        saveSettings();
    });
    
    const settingsModal = document.getElementById('settingsModal');
    const settingsBtn = document.getElementById('settingsBtn');
    const closeModal = document.querySelector('.close-modal');
    const closeSettingsBtn = document.getElementById('closeSettingsBtn');
    
    settingsBtn.addEventListener('click', () => {
        settingsModal.style.display = 'flex';
        if (window.controls && window.controls.populateKeyMappingModal) {
            window.controls.populateKeyMappingModal();
        }
    });
    
    closeModal.addEventListener('click', () => {
        settingsModal.style.display = 'none';
    });
    
    closeSettingsBtn.addEventListener('click', () => {
        settingsModal.style.display = 'none';
    });
    
    window.addEventListener('click', (e) => {
        if (e.target === settingsModal) {
            settingsModal.style.display = 'none';
        }
    });
    
    document.getElementById('resetKeysBtn').addEventListener('click', () => {
        if (confirm('Reset all key bindings to defaults?')) {
            if (window.controls && window.controls.initKeyMappings) {
                window.controls.initKeyMappings();
                window.controls.populateKeyMappingModal();
            }
        }
    });
};

function loadSettings() {
    const savedPixelation = localStorage.getItem('pixelationEnabled');
    if (savedPixelation !== null) {
        pixelationEnabled = savedPixelation === 'true';
        document.getElementById('pixelationToggle').checked = pixelationEnabled;
    }
    
    const savedMute = localStorage.getItem('isMuted');
    if (savedMute !== null) {
        isMuted = savedMute === 'true';
        const muteBtn = document.getElementById('muteBtn');
        if (isMuted) {
            muteBtn.innerHTML = '<i class="fas fa-volume-mute"></i> Mute';
        }
    }
}

function saveSettings() {
    localStorage.setItem('pixelationEnabled', pixelationEnabled);
    localStorage.setItem('isMuted', isMuted);
}