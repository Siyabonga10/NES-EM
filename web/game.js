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
ctx.fillStyle = "green";
ctx.fillRect(0, 0, width, height);

let keyStatesPtr = undefined;

const renderFrame = () => {
    const ptr = nesModule._tickCPU(keyStatesPtr)

    const isNewFrame = nesModule.HEAPU8[ptr] !== 0;
    if (!isNewFrame) { // check not needed anymore
        requestAnimationFrame(renderFrame)
        return;
    }
    const width = nesModule.HEAPU32[ptr / 4 + 1];
    const height = nesModule.HEAPU32[ptr / 4 + 2];
    const dataPtr = nesModule.HEAPU32[ptr / 4 + 3];

    // Read pixel data
    const pixels = new Uint8Array(nesModule.HEAPU8.buffer, dataPtr, no_of_cols * no_of_rows * 4);
    console.log(pixels)
    requestAnimationFrame(renderFrame)
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
