const initNES = (data) => {
    initialiseNES().then(nes => {
        console.log(nes)
        const ptr = nes._nes_alloc(data.byteLength)
        nes.HEAPU8.set(data, ptr)

        nes._loadCartriadgeAndConnectToBus(ptr, data.byteLength)
        nes._nes_dealloc(ptr)
    })
}

const runGame = (nesModule) => {

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
