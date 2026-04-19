# ABSYNTH

> A JUCE 8 subtractive synthesizer with a Vue 3 WebView UI, playable virtual keyboard, legato/portamento engine, and LFO-driven wub generator.

---

## Overview

**Absynth** is a standalone synthesizer and VST3/AU plugin built on the [JUCE](https://juce.com/) framework. Its entire user interface is a Vue 3 web application running inside a `WKWebView`, communicating with the C++ DSP engine through JUCE 8's native WebView bridge. The result is a fully reactive, hot-reloadable synth UI tightly coupled to a high-performance audio engine.

---

## Feature Summary

| Module | Description |
|--------|-------------|
| **Oscillator** | Sine, Sawtooth, or Square wave with MIDI note pitch tracking |
| **Filter** | 24dB/oct Ladder Filter with Cutoff & Resonance |
| **Envelope (ADSR)** | Attack, Decay, Sustain, Release amplitude envelope |
| **Legato / Portamento** | Monophonic legato mode with sample-accurate pitch glide |
| **Wub Generator** | LFO → filter modulation (LPF or BPF) for dubstep-style wub effects |
| **Virtual Keyboard** | Drag-to-play chromatic keyboard with octave selection |

---

## Architecture

```
absynth-vst/                   ← This repo (C++ / JUCE)
│
├── Source/
│   ├── PluginProcessor.h/.cpp ← DSP engine (synth voices, wub LFO, ADSR, filter)
│   └── PluginEditor.h/.cpp    ← WebView host + JS↔C++ parameter bridge
│
└── JUCE/                      ← JUCE framework (git submodule)

absynth-ui/                    ← Companion repo (Vue 3 / TypeScript)
│
└── src/
    ├── App.vue                ← Root layout (all synth sections)
    ├── components/
    │   ├── JuceKnob.vue       ← Rotary knob bound to JUCE SliderRelay
    │   ├── JuceSelect.vue     ← Dropdown bound to JUCE ComboBoxRelay
    │   ├── JuceToggle.vue     ← Toggle switch bound to JUCE ToggleButtonRelay
    │   └── VirtualKeyboard.vue← Chromatic keyboard → sendMidiNote native bridge
    └── vite.config.ts         ← Vite config (HMR dev server on localhost:5173)
```

### JS ↔ C++ Bridge

JUCE 8's `WebBrowserComponent` exposes two-way communication channels:

- **Parameters** (knobs, selects, toggles) use `WebSliderRelay`, `WebComboBoxRelay`, and `WebToggleButtonRelay` on the C++ side, paired with `juce-framework-frontend` hooks on the Vue side. Changes flow in both directions automatically.
- **MIDI** is sent from JS to C++ via a registered native function: `window.__JUCE__.backend.sendMidiNote(note, velocity, isNoteOn)`. In Vue this is called through `Juce.getNativeFunction("sendMidiNote")`.

---

## Building

### Prerequisites

| Tool | Install |
|------|---------|
| CMake ≥ 3.22 | `brew install cmake` |
| Ninja | `brew install ninja` |
| Xcode CLT | `xcode-select --install` |
| Node.js ≥ 18 | [nodejs.org](https://nodejs.org) |

Initialize JUCE submodule:

```bash
git submodule update --init --recursive
```

### Configure & Build

```bash
# Configure
cmake . -B build -G Ninja -DJUCE_BUILD_EXAMPLES=OFF

# Build all targets (Standalone + VST3)
cmake --build build --parallel 8
```

Outputs:
- **Standalone**: `build/AbsynthSynth_artefacts/Standalone/Absynth.app`
- **VST3**: `build/AbsynthSynth_artefacts/VST3/Absynth.vst3`

### Running with Live UI (Development)

Start the Vue dev server in the `absynth-ui` repo first:

```bash
cd ../absynth-ui
npm install
npm run dev        # → http://localhost:5173
```

Then launch the standalone app:

```bash
open build/AbsynthSynth_artefacts/Standalone/Absynth.app
```

The standalone loads `http://localhost:5173` inside its WebView. Vite HMR means any UI change is reflected instantly — no rebuild required.

### Install as Plugin

Copy the built plugin to your system plugin directory:

```bash
# VST3
cp -r build/AbsynthSynth_artefacts/VST3/Absynth.vst3 /Library/Audio/Plug-Ins/VST3/

# AU
cp -r build/AbsynthSynth_artefacts/AU/Absynth.component /Library/Audio/Plug-Ins/Components/
```

Rescan plugins in your DAW.

---

## Signal Chain

```
MIDI Input (keyboard / DAW)
        │
        ▼
  MidiKeyboardState  ──────────────────── Virtual Keyboard (JS → C++)
        │
        ▼
  CustomSynth (juce::Synthesiser)
  ┌─────────────────────────┐
  │  SynthVoice (×4)        │
  │   Oscillator (osc type) │
  │   → SmoothedFreq (glide)│
  │   → LadderFilter (LPF24)│
  │   → Gain                │
  │   → ADSR envelope       │
  └─────────────────────────┘
        │
        ▼  (mixed stereo bus)
  WubEngine (global post-FX)
   LFO → LadderFilter (LPF or BPF)
        │
        ▼
   Audio Output
```

---

## Parameter Reference

See [`PARAMETERS.md`](./PARAMETERS.md) for a full description of every knob, toggle, and selector.

---

## Development Notes

- The `CMakeLists.txt` post-build step injects `NSAppTransportSecurity` into the Standalone `Info.plist` to allow the WebView to connect to `http://localhost:5173`.
- The Vue dev server **must** run on `localhost` (not `127.0.0.1`) for JUCE's WKWebView security context to treat it as a secure origin.
- `vueDevTools()` is disabled in `vite.config.ts` — it crashes WKWebView on macOS.

---

## License

MIT — see `LICENSE` for details.

## Acknowledgments

Built with [JUCE](https://juce.com/) · UI powered by [Vue 3](https://vuejs.org/) + [Vite](https://vitejs.dev/)