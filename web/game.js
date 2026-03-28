let nesModule = undefined
const initNES = (data) => {
    initialiseNES().then(nes => {
        const ptr = nes._nes_alloc(data.byteLength)
        nes.HEAPU8.set(data, ptr)
        nesModule = nes;
        nes._loadCartriadgeAndConnectToBus(ptr, data.byteLength)
        runGame(nes)
        nes._nes_dealloc(ptr)
    })
}
const width = 768
const height = 720

const no_of_rows = 240;
const no_of_cols = 256;

const NUMBER_OF_INPUT_FIELDS = 8

const canvas = document.getElementById("myCanvas")
const ctx = canvas.getContext("2d");


let keyStatesPtr = undefined;

function rgbToHex(r, g, b, a) {
    return `${r.toString(16).padStart(2, '0')}${g.toString(16).padStart(2, '0')}${b.toString(16).padStart(2, '0')}${a.toString(16).padStart(2, '0')}`
}

const drawPixels = (ptr) => {
    for (let i = 0; i < no_of_rows; i++) {
        for (let j = 0; j < no_of_cols; j++) {
            ctx.fillStyle = rgbToHex(
                nesModule.HEAPU8[ptr + i * no_of_cols * 4 + j * 4],
                nesModule.HEAPU8[ptr + i * no_of_cols * 4 + j * 4 + 1],
                nesModule.HEAPU8[ptr + i * no_of_cols * 4 + j * 4 + 2],
                nesModule.HEAPU8[ptr + i * no_of_cols * 4 + j * 4 + 3],

            )
            ctx.fillRect(j * 3, i * 3, 3, 3);
        }
    }
}

const renderFrame = () => {
    const ptr = nesModule._tickCPU(keyStatesPtr)

    const isNewFrame = nesModule.HEAPU8[ptr] !== 0;

    const width = nesModule.HEAPU32[ptr / 4 + 1];
    const height = nesModule.HEAPU32[ptr / 4 + 2];
    const dataPtr = nesModule.HEAPU32[ptr / 4 + 3];

    drawPixels(dataPtr)
    requestAnimationFrame(renderFrame)
    console.log("rendered frame")
}


const runGame = (nesModule) => {
    nesModule._connectControllerToConsole();
    nesModule._bootPPU();
    nesModule._bootCPU();
    keyStatesPtr = nesModule._nes_alloc(NUMBER_OF_INPUT_FIELDS)
    for (let i = 0; i < 8; i++) {
        nesModule.HEAPU8[keyStatesPtr + i] = 0;
    }
    requestAnimationFrame(renderFrame)
}

window.onload = function () {
    const fileInput = document.getElementById('fileInput')
    const fileDisplayArea = document.getElementById('fileDisplayArea')

    fileInput.addEventListener('change', function (e) {
        const file = fileInput.files[0] // Get the first selected file
        const reader = new FileReader() // Create a FileReader object

        // Define what happens once the file is loaded
        reader.onload = function (e) {
            const data = new Uint8Array(e.target.result)
            initNES(data)
        }
        reader.readAsArrayBuffer(file)
    })
}
