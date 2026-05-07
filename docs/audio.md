# Emulator Audio Architecture

## Philosophy

Audio is split into two clear responsibilities that never overlap:

- **Core** — knows how to generate samples, knows nothing about the platform
- **Platform wrapper** — owns the audio API, asks the core for samples when needed

The core never initiates audio output. It waits to be asked. The platform drives
everything through a callback mechanism, and the core simply fills whatever buffer
it is handed with the next N samples.

---

## The Universal Contract

Every audio API on every platform — regardless of how different they look on the
surface — reduces to the same fundamental question directed at the core:

```
"I need N samples right now. Fill this buffer."
```

The core answers that question the same way every time: mix all active channels,
apply gain, write samples, return. It does not know what asked, it does not know
what platform it is on. This is the entire basis for portability.

---

## Core Internals

When asked for samples the core mixes its active channels — pulse, triangle,
noise, DMC — into a single mono float stream. The result is scaled and written
directly into the buffer provided by the platform.

```
  Pulse 1  ---+
  Pulse 2  ---+
  Triangle ---+--->  sum  --->  gain  --->  buffer
  Noise    ---+
  DMC      ---+
```

No internal buffering, no threading, no allocations. Purely synchronous.

---

## Platform Connections

Each platform provides a native callback that fires on a dedicated audio thread
whenever the hardware needs more samples. The wrapper's only job is to receive
that callback and forward it to the core.

```
  +----------------+     +----------------+     +-------------------+
  |    WINDOWS     |     |      WEB       |     |      ANDROID      |
  |                |     |                |     |                   |
  |  Raylib Audio  |     |  AudioWorklet  |     |  AAudio / OpenSL  |
  |   Callback     |     |   Callback     |     |    Callback       |
  +-------+--------+     +-------+--------+     +---------+---------+
          |                      |                         |
          +----------+-----------+-------------------------+
                     |
                     v
          +----------+----------+
          |        CORE         |
          |   master_callback   |
          |  (buffer, frames)   |
          +---------------------+
```

The arrows represent the platform handing a buffer and a frame count down into
the core. Nothing flows the other way. The core has no handle to the platform.

---

## Platform Notes

### Windows — Raylib

Raylib wraps the OS audio system and exposes a stream callback that matches the
core interface almost exactly. Setup is minimal: create a stream with the desired
sample rate and format, register the core callback, and play. Raylib manages the
audio thread internally.

### Web — AudioWorklet

The browser's AudioWorklet runs on a dedicated audio thread, fully isolated from
the main JavaScript thread and immune to GC pauses. The core is compiled to WASM
and exported as a callable function. The worklet fires on a tight schedule set by
the browser, calls into WASM, and hands the filled buffer directly to the audio
hardware. The older ScriptProcessor API should not be used — it runs on the main
thread and will stutter under any rendering load.

### Android — AAudio

AAudio is Android's low-latency audio API and is explicitly designed around the
callback model. The OS manages a high-priority audio thread and fires the callback
on a strict schedule. The core runs entirely on the native side — Java is only
involved at lifecycle boundaries (start on resume, stop on pause). The audio
buffer never crosses the JNI boundary during playback.

---

## Coupling Points

A small number of constants must be agreed upon between the core and the platform.
These are intentional and acceptable:

| Constant        | Notes                                              |
|-----------------|----------------------------------------------------|
| Sample rate     | Typically 44100 Hz, defined per platform build     |
| Bit depth       | Core outputs 32-bit float throughout               |
| Channel count   | Core is mono, platform configures accordingly      |
| Buffer size     | Chosen by the platform, core accepts any size      |

The core makes no assumptions about these values beyond what it is handed at
call time. All configuration lives in the platform wrapper.

---

## Adding a New Platform

The process for any new platform is always the same:

1. Find the platform's audio callback API
2. Write a thin shim that matches its signature
3. Forward the buffer and frame count to the core
4. Hook init and teardown into the platform's lifecycle

The core does not change. The shim is typically only a few lines.

---

## Thread Safety

The audio callback fires on a high-priority thread on every platform. If the CPU
core writes to APU registers while the audio callback reads them, that shared
state must be protected. Mutexes should be avoided inside the callback as they
can cause priority inversion and audible dropouts. Atomic reads and writes are
the preferred approach for any state shared between the CPU loop and the audio
callback.