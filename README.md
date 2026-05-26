# Lattice VST (absynth-vst)

JUCE 8 instrument: subtractive synth DSP with a Vue 3 UI in `WKWebView`, VST3 + Standalone on macOS.

Companion repo: [absynth-ui](https://github.com/JameyStiling/absynth-ui).  
Recommended: build from the parent **`absynth`** monorepo when available (`npm run build:vst3` at repo root).

**Public demo downloads** (landing page zip): package and publish from the monorepo — see [`../RELEASE.md`](../RELEASE.md) (`npm run package:demo`, GitHub Actions on `v*` tags).

---

## Features

| Module | Description |
|--------|-------------|
| **Oscillators** | Three independent sine / saw / square sources |
| **Filter** | Per-voice ladder filter (cutoff, resonance) |
| **ADSR** | Amplitude envelope |
| **Legato / glide** | Monophonic legato with smoothed pitch |
| **Mod engine** | LFO or MSEG-draw filter modulation |
| **Arpeggiator** | Rate, swing, modes |
| **UI** | Embedded web UI via JUCE `WebBrowserComponent` + native integration |

---

## Layout

```
Source/
  PluginProcessor.*   DSP, APVTS, voices, mod/arp
  PluginEditor.*      WebView host, relays, native functions
JUCE/                 Vendored JUCE (local copy; not in git — run `scripts/fetch-juce.sh` if missing)
CMakeLIsts.txt
PARAMETERS.md         Parameter ID reference
```

---

## Prerequisites

| Tool | Install |
|------|---------|
| CMake ≥ 3.22 | `brew install cmake` |
| Ninja | `brew install ninja` |
| Xcode CLT | `xcode-select --install` |
| Node.js ≥ 20 | For UI bundle (monorepo builds UI first) |

---

## Build (this repo alone)

```bash
# UI bundle must exist first — from absynth-ui:
#   npm run build-plugin

cmake . -B build -G Ninja -DJUCE_BUILD_EXAMPLES=OFF
cmake --build build --parallel 8
```

Outputs:

- **Standalone:** `build/Lattice_artefacts/Standalone/Lattice.app`
- **VST3:** `build/Lattice_artefacts/VST3/Lattice.vst3`

Copy UI into the bundle before shipping:

```bash
# From monorepo (preferred):
../scripts/copy-ui-bundle.sh
```

---

## Monorepo workflow (recommended)

From `absynth` root:

```bash
npm run setup
npm run build:vst3
```

This will:

1. Build `Lattice.vst3` / `Lattice.app`
2. Run `build-plugin` in `absynth-ui` (single-file HTML)
3. Embed UI under `Contents/Resources/ui`
4. Install to `~/Library/Audio/Plug-Ins/VST3/Lattice.vst3` and ad-hoc sign

### Bitwig

1. Run `npm run build:vst3` from the monorepo.
2. **Settings → Locations → Plug-ins** — use `~/Library/Audio/Plug-Ins/VST3`; remove broad **Documents** scan paths.
3. Rescan; load Lattice from the Library path.
4. Quit and reopen Bitwig if the UI was cached.

**Standalone dev:** UI loads `http://localhost:5173` (run `npm run dev` in monorepo).  
**VST3 in a DAW:** UI loads from embedded `Resources/ui` via JUCE resource provider (not localhost).

---

## Development (Standalone + live UI)

Terminal 1 — UI (monorepo or `absynth-ui`):

```bash
npm run dev
```

Terminal 2 — open app after native build:

```bash
open build/Lattice_artefacts/Standalone/Lattice.app
```

`CMakeLists.txt` adds App Transport Security exceptions so the Standalone WebView can reach `http://localhost:5173`.

---

## UI hosting (C++)

`PluginEditor` registers:

- `WebSliderRelay` / attachments for APVTS parameters
- Native functions: `sendMidiNote`, `sendModDrawState`
- Resource provider serving files from `Contents/Resources/ui`
- Standalone: `goToURL("http://localhost:5173")`
- Plugin: `goToURL(getResourceProviderRoot())` with on-disk bundle (resolved via plugin binary path / bundle ID)

---

## Signal chain

```
MIDI → MidiKeyboardState → CustomSynth (voices: osc → filter → ADSR)
      → ModEngine (bus filter) → output
```

See [`PARAMETERS.md`](./PARAMETERS.md) for every parameter ID.

---

## Development notes

- Product name in CMake: **Lattice** (`com.sherdaudio.lattice`).
- `JUCE_WEB_BROWSER=1` is set on the target.
- Do not enable Vue devtools in the UI project when targeting WKWebView.

---

## License

MIT — see `LICENSE` if present.
