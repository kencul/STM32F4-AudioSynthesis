# STM32 Polyphonic Wavetable Synthesizer

A bare-metal, 8-voice polyphonic wavetable synthesizer built on the STM32F407G-DISC1. Receives MIDI over USB, processes audio in real-time on the Cortex-M4 FPU, and outputs to the on-board CS43L22 DAC, with all timing interrupt-driven.

**[Demo Video](https://youtu.be/QuVIntSHrEU)** | **[Case Study](https://www.kenmusic.net/projects/stm32-synth)**

![Demo](demo/thumbnail.jpg)

---

## Features

- **8-voice polyphony** with priority-based voice stealing (idle → released → oldest)
- **Dual-slot wavetable morphing** — crossfade between two waveforms in real time
- **Zero-Delay Feedback State Variable Filter** — algebraic loop solver for analog-accurate resonance
- **Per-voice ADSR** with soft-kill ramp (~5ms) to eliminate clicks on voice steal
- **USB MIDI device** — plug-and-play with any DAW or controller (Note On/Off, Pitch Bend, Mod Wheel, Panic)
- **8 hardware knobs** via ADC + multiplexer: Volume, Cutoff, Resonance, Morph, Attack, Decay, Sustain, Release
- **SSD1306 OLED** with context-sensitive views (waveform preview, filter curve, ADSR shape)
- **PCA9685 LED controller** displaying per-voice amplitude in real time
- **CCMRAM placement** for `VoiceManager` to eliminate bus contention on the audio path
- **DWT cycle counter** for measuring audio block processing time in µs

---

## Architecture

```
USB OTG ──► MIDI Parser
                │
                ▼
          VoiceManager  (CCMRAM)
          ┌─────┴──────────────────────────────┐
       Voice 0  ···  Voice 3  ···  Voice 7     │
          │              │              │       │
        [Osc]          [Osc]          [Osc]    │  ← Wavetable phase accumulator
     (Morph A↔B)   (Morph A↔B)   (Morph A↔B) │     + pitch bend (2^x formula)
          │              │              │       │
        [SVF]          [SVF]          [SVF]    │  ← Zero-delay feedback
          │              │              │       │
       [ADSR]         [ADSR]         [ADSR]    │  ← Soft-kill on voice steal
          └─────┬──────────────────────────────┘
                │  sum + VOICE_GAIN_SCALAR
                ▼
         DMA Circular Buffer  (128 samples × int16, stereo)
                │
                ▼  Half-transfer / transfer-complete IRQ fills each half
            I2S3 (DMA, 48 kHz, 16-bit)
                │
                ▼
          CS43L22 Codec  (initialized via I2C)
                │
                ▼
        3.5 mm Headphone Jack


Hardware Control (main loop, polled)
  TIM4 IRQ ──► ADC DMA ──► PotBank (8 knobs via CD4051 mux)
  GPIO IRQ ──► Button debounce (25 ms) ──► waveform cycle

UI (main loop)
  OLED SSD1306 (I2C) — wavetable / filter / ADSR view, 1.2 s timeout
  PCA9685 (I2C)      — 8 voice-level LEDs, updated every 20 ms
```

---

## Build Environment

| Tool | Version |
|------|---------|
| arm-none-eabi-gcc | 14.3.1 (GNU Tools for STM32 14.3.1+st.2) |
| CMake | ≥ 3.22 |
| C standard | C11 |
| C++ standard | C++17 |
| STM32CubeMX | Used to generate HAL init code (`.ioc` included) |

The toolchain file targets **Cortex-M4** with the **FPV4-SP-D16** FPU (`-mfloat-abi=hard`). Floating-point synthesis runs natively on hardware without software emulation.

---

## Dependencies

All dependencies are vendored in the repository.

| Dependency | Location | Notes |
|------------|----------|-------|
| CMSIS 5 | `Drivers/CMSIS/` | Core headers for Cortex-M4 |
| STM32F4xx HAL | `Drivers/STM32F4xx_HAL_Driver/` | ST HAL for F4 series |
| STM32 USB Device Library | `Middlewares/ST/STM32_USB_Device_Library/` | USB Audio class (repurposed as MIDI) |

---

## Building

```bash
# Configure (Debug)
cmake -B build/Debug \
  -DCMAKE_TOOLCHAIN_FILE=cmake/gcc-arm-none-eabi.cmake \
  -DCMAKE_BUILD_TYPE=Debug

# Build
cmake --build build/Debug

# The flashable binary is at:
# build/Debug/basicSynth.elf
```

Flash and debug using the [STM32 VS Code Extension](https://marketplace.visualstudio.com/items?itemName=stmicroelectronics.stm32-vscode-extension) (ST's official extension). The `.vscode/launch.json` is pre-configured with an ST-Link GDB server launch target. Press **F5** or use the Run and Debug panel to build, flash, and attach in one step.

---

## Knob Mapping

| Knob | Parameter | Scale |
|------|-----------|-------|
| 0 | Volume | Exponential |
| 1 | Filter Cutoff | Exponential, 20 Hz – 20 kHz |
| 2 | Resonance | Power curve (deadzone at heel) |
| 3 | Wavetable Morph | Linear, A → B |
| 4 | Attack | Exponential, 0 – 2 s |
| 5 | Decay | Exponential, 0 – 2 s |
| 6 | Sustain | Linear, 0 – 1 |
| 7 | Release | Exponential, 0 – 2 s |

---

## Dev Notes

See [DEVLOG.md](DEVLOG.md) for detailed notes on the I2C/I2S/DMA setup, CubeMX clock configuration, and the design decisions made during development.

---

## License

MIT: [LICENSE](LICENSE).
