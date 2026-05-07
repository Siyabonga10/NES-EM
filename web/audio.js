const SAMPLE_RATE = 44100;
const BUFFER_SIZE = 1024;

class NesAudio {
    constructor(nesModule) {
        this.nes = nesModule;
        this.ctx = null;
        this.node = null;
        this.volume = 0.3;
        this.wasmBuf = 0;
    }

    init() {
        this.ctx = new AudioContext({ sampleRate: SAMPLE_RATE });
        this.wasmBuf = this.nes._nes_alloc(BUFFER_SIZE * 4);

        this.node = this.ctx.createScriptProcessor(BUFFER_SIZE, 0, 1);
        this.node.onaudioprocess = (e) => {
            const out = e.outputBuffer.getChannelData(0);
            this.nes._apu_mix_samples(this.wasmBuf, BUFFER_SIZE);
            const src = new Float32Array(this.nes.HEAPU8.buffer, this.wasmBuf, BUFFER_SIZE);
            for (let i = 0; i < BUFFER_SIZE; i++) {
                out[i] = src[i] * this.volume;
            }
        };
        this.node.connect(this.ctx.destination);
    }

    setVolume(v) {
        this.volume = Math.max(0.0, Math.min(1.0, v));
    }

    resume() {
        if (this.ctx && this.ctx.state === 'suspended') {
            this.ctx.resume();
        }
    }
}
