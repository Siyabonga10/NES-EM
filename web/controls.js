const CONTROLLER_INDICES = {
    A: 0,
    B: 1,
    UP: 2,
    DOWN: 3,
    LEFT: 4,
    RIGHT: 5,
    START: 6,
    SELECT: 7
};

const DEFAULT_KEY_MAP = {
    'a': CONTROLLER_INDICES.A,
    'b': CONTROLLER_INDICES.B,
    'i': CONTROLLER_INDICES.UP,
    'k': CONTROLLER_INDICES.DOWN,
    'j': CONTROLLER_INDICES.LEFT,
    'l': CONTROLLER_INDICES.RIGHT,
    ' ': CONTROLLER_INDICES.START,
    'enter': CONTROLLER_INDICES.SELECT
};

const DEFAULT_INDEX_MAP = {
    [CONTROLLER_INDICES.A]: 'A',
    [CONTROLLER_INDICES.B]: 'B',
    [CONTROLLER_INDICES.UP]: 'I',
    [CONTROLLER_INDICES.DOWN]: 'K',
    [CONTROLLER_INDICES.LEFT]: 'J',
    [CONTROLLER_INDICES.RIGHT]: 'L',
    [CONTROLLER_INDICES.START]: 'Space',
    [CONTROLLER_INDICES.SELECT]: 'Enter'
};

let keyMap = {};
let indexMap = {};
var keyStatesPtr = undefined;
let _nesModule = undefined;
let isRemapping = false;
let currentRemapButton = null;

function initKeyMappings() {
    const saved = localStorage.getItem('nesKeyMap');
    if (saved) {
        try {
            keyMap = JSON.parse(saved);
            indexMap = {};
            for (const [key, index] of Object.entries(keyMap)) {
                indexMap[index] = key;
            }
        } catch (e) {
            console.error('Failed to parse saved key map', e);
            loadDefaultMappings();
        }
    } else {
        loadDefaultMappings();
    }
}

function loadDefaultMappings() {
    keyMap = { ...DEFAULT_KEY_MAP };
    indexMap = { ...DEFAULT_INDEX_MAP };
    saveKeyMappings();
}

function saveKeyMappings() {
    localStorage.setItem('nesKeyMap', JSON.stringify(keyMap));
}

function getKeyDisplay(index) {
    const key = indexMap[index];
    if (!key) return '?';
    if (key === ' ') return 'Space';
    if (key === 'enter') return 'Enter';
    if (key === 'arrowup') return '\u2191';
    if (key === 'arrowdown') return '\u2193';
    if (key === 'arrowleft') return '\u2190';
    if (key === 'arrowright') return '\u2192';
    return key.toUpperCase();
}

function getKeyFromEvent(event) {
    let key = event.key.toLowerCase();
    if (key === ' ') return ' ';
    if (key === 'enter') return 'enter';
    if (key.startsWith('arrow')) return key;
    return key;
}

const updateKeyState = (index, setToTrue) => {
    if (keyStatesPtr === undefined || !_nesModule) return;
    _nesModule.HEAPU8[keyStatesPtr + index] = setToTrue ? 1 : 0;
};

function handleKeyDown(event) {
    const key = getKeyFromEvent(event);
    if (key in keyMap) {
        event.preventDefault();
        updateKeyState(keyMap[key], true);
    }
}

function handleKeyUp(event) {
    const key = getKeyFromEvent(event);
    if (key in keyMap) {
        event.preventDefault();
        updateKeyState(keyMap[key], false);
    }
}

function getControllerIndex(key) {
    switch (key) {
        case 'a': return CONTROLLER_INDICES.A;
        case 'b': return CONTROLLER_INDICES.B;
        case 'up': return CONTROLLER_INDICES.UP;
        case 'down': return CONTROLLER_INDICES.DOWN;
        case 'left': return CONTROLLER_INDICES.LEFT;
        case 'right': return CONTROLLER_INDICES.RIGHT;
        case 'start': return CONTROLLER_INDICES.START;
        case 'select': return CONTROLLER_INDICES.SELECT;
        default: return -1;
    }
}

function startRemap(button) {
    if (isRemapping) cancelRemap();
    isRemapping = true;
    currentRemapButton = button;
    button.classList.add('recording');
    button.textContent = 'Press any key...';

    const captureKey = (event) => {
        event.preventDefault();
        event.stopPropagation();

        const key = getKeyFromEvent(event);
        const controllerIndex = getControllerIndex(button.dataset.key);
        if (controllerIndex === -1) return;

        const oldKey = indexMap[controllerIndex];
        if (oldKey) delete keyMap[oldKey];

        keyMap[key] = controllerIndex;
        indexMap[controllerIndex] = key;

        document.removeEventListener('keydown', captureKey);
        document.removeEventListener('click', cancelOnClick);
        isRemapping = false;
        button.classList.remove('recording');
        updateKeyButton(button, controllerIndex);
        saveKeyMappings();
        populateKeyMappingModal();
    };

    const cancelOnClick = (event) => {
        if (event.target !== button) {
            document.removeEventListener('keydown', captureKey);
            document.removeEventListener('click', cancelOnClick);
            isRemapping = false;
            button.classList.remove('recording');
            updateKeyButton(button, getControllerIndex(button.dataset.key));
        }
    };

    document.addEventListener('keydown', captureKey);
    document.addEventListener('click', cancelOnClick, { once: true });
}

function updateKeyButton(button, index) {
    button.textContent = getKeyDisplay(index);
}

function cancelRemap() {
    if (currentRemapButton) {
        currentRemapButton.classList.remove('recording');
        currentRemapButton = null;
    }
    isRemapping = false;
}

function populateKeyMappingModal() {
    const container = document.getElementById('keyMappingContainer');
    if (!container) return;

    container.innerHTML = '';

    const mappingList = [
        { label: 'A Button', key: 'a', index: CONTROLLER_INDICES.A },
        { label: 'B Button', key: 'b', index: CONTROLLER_INDICES.B },
        { label: 'Up', key: 'up', index: CONTROLLER_INDICES.UP },
        { label: 'Down', key: 'down', index: CONTROLLER_INDICES.DOWN },
        { label: 'Left', key: 'left', index: CONTROLLER_INDICES.LEFT },
        { label: 'Right', key: 'right', index: CONTROLLER_INDICES.RIGHT },
        { label: 'Start', key: 'start', index: CONTROLLER_INDICES.START },
        { label: 'Select', key: 'select', index: CONTROLLER_INDICES.SELECT }
    ];

    mappingList.forEach(item => {
        const div = document.createElement('div');
        div.className = 'key-mapping-row';

        const label = document.createElement('span');
        label.textContent = item.label;

        const btn = document.createElement('button');
        btn.className = 'key-btn';
        btn.textContent = getKeyDisplay(item.index);
        btn.dataset.key = item.key;
        btn.addEventListener('click', () => startRemap(btn));

        div.appendChild(label);
        div.appendChild(btn);
        container.appendChild(div);
    });
}

document.addEventListener('DOMContentLoaded', () => {
    initKeyMappings();

    document.addEventListener('keydown', handleKeyDown);
    document.addEventListener('keyup', handleKeyUp);

    const preventDefaultKeys = [' ', 'ArrowUp', 'ArrowDown', 'ArrowLeft', 'ArrowRight'];
    document.addEventListener('keydown', (event) => {
        if (preventDefaultKeys.includes(event.key)) {
            if (!event.target.matches('input, textarea, select, button')) {
                event.preventDefault();
            }
        }
    });

    const canvas = document.getElementById('myCanvas');
    if (canvas) {
        canvas.addEventListener('contextmenu', (e) => e.preventDefault());
    }
});

window.controls = {
    setNesModule: (module) => { _nesModule = module; },
    setKeyStatesPtr: (ptr) => { keyStatesPtr = ptr; },
    updateKeyState,
    populateKeyMappingModal,
    initKeyMappings
};
