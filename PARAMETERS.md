# Absynth — Parameter Reference

> Complete documentation for every control in the Absynth synthesizer.

---

## Table of Contents

- [Oscillator](#-oscillator)
- [Filter](#-filter)
- [Envelope (ADSR)](#-envelope-adsr)
- [Legato & Portamento](#-legato--portamento)
- [Wub Generator](#-wub-generator)
- [Virtual Keyboard](#-virtual-keyboard)
- [Sound Design Tips](#-sound-design-tips)

---

## 🎛 Oscillator

The oscillator is the starting point of every sound. It generates a continuous raw waveform at the pitch of the pressed MIDI note.

### Waveform

| Waveform | Character | Best For |
|----------|-----------|----------|
| **Sine** | Pure, round, warm | Sub bass, pads, smooth leads |
| **Saw** | Bright, rich in harmonics | Leads, bass lines, classic synth tones |
| **Square** | Hollow, woody, nasal | Retro leads, clarinets, chiptune |

**Tip:** Saw wave is the classic choice for subtractive synthesis because it contains all harmonics — the filter then carves the tone from that rich source.

---

## 🎛 Filter

A 24dB/octave Ladder Filter modeled after the famous Moog ladder topology. It removes frequencies from the oscillator's output to shape the tonal character of the sound.

### Cutoff

- **Range:** 20 Hz – 20,000 Hz  
- **Curve:** Logarithmic (small knob changes have more effect at low values)

Controls the frequency above (LPF mode) which harmonics are attenuated. Lower cutoff = darker, more muffled sound. Higher cutoff = brighter, more open sound.

**Tip:** Most of the expressiveness in subtractive synthesis comes from automating or modulating the cutoff over time.

### Resonance

- **Range:** 0.0 – 1.0

Boosts frequencies right at the cutoff point, creating a characteristic "peak" or "squelch". At high resonance values the filter begins to self-oscillate, producing a pure sine tone at the cutoff frequency.

**Tip:** A resonance value around 0.3–0.6 combined with a moving cutoff creates classic analog-style filter sweeps.

---

## 🎛 Envelope (ADSR)

The ADSR envelope controls the **amplitude** (volume) shape of each note over time. It describes how the sound evolves from the moment a key is pressed to after it is released.

```
Volume
  │        ╭──── Decay
  │       ╱       ╲
  │      ╱         ╲___________ Sustain
  │     ╱ Attack               ╲
  │    ╱                        ╲ Release
  └────────────────────────────────── Time
       ↑ Key On                  ↑ Key Off
```

### Attack

- **Range:** 1ms – 5s  
- **Default:** 10ms

Time for the sound to ramp from silence to full volume after a key is pressed. Short = percussive/plucked. Long = pad/strings swell.

### Decay

- **Range:** 1ms – 5s  
- **Default:** 100ms

Time for the sound to fall from its peak volume down to the sustain level. Controls how quickly the initial "transient" settles.

### Sustain

- **Range:** 0.0 – 1.0  
- **Default:** 1.0

The volume level held for as long as a key is held down (after the decay phase). At 0, sound dies after the decay even if you hold the key. At 1, full volume is held indefinitely.

### Release

- **Range:** 1ms – 5s  
- **Default:** 1s

Time for the sound to fade to silence after a key is released. Short = abrupt cutoff. Long = reverberant, drifting tail.

---

## 🎛 Legato & Portamento

These controls appear in the side panel to the left of the virtual keyboard.

### Legato (Toggle)

When **ON**, Absynth switches to **monophonic** mode:

- Only one note sounds at a time.
- Playing a new note while holding another **does not re-trigger the ADSR** — the envelope continues from where it is, and the pitch glides to the new note.
- When you release the top note, the pitch glides back to the note still held below it (last-note priority).
- When all held notes are released, the note off is sent normally.

When **OFF**, Absynth is **polyphonic** (up to 4 simultaneous voices, each with its own independent envelope).

**Tip:** Legato mode is essential for realistic lead lines and bass slides. Turn it on, set a moderate Glide time, and drag across the keyboard for smooth glissandos.

### Glide (Portamento Time)

- **Range:** 0ms – 1000ms  
- **Default:** 50ms

Controls how long it takes the pitch to slide from one note to the next when Legato is active. Uses a `SmoothedValue` internally so the frequency interpolation is sample-accurate with no zipper noise.

- **0ms** — Instant pitch jump (monophonic but no glide)
- **50ms** — Subtle slide, barely noticeable
- **200–500ms** — Classic dubstep bass / synth lead glide
- **1000ms** — Slow, dramatic portamento swoop

---

## 🎛 Wub Generator

The Wub Generator is a post-synth effect that applies an **LFO (Low Frequency Oscillator)** to modulate a dedicated filter's cutoff frequency. This creates the classic rhythmic "wub wub" sound associated with dubstep and bass music.

The wub filter is applied to the **master stereo bus** after all synth voices are mixed together, so it affects all playing notes simultaneously.

### Enable (Toggle)

Gates the entire wub effect. When OFF, audio passes through completely unaffected.

### Rate

- **Range:** 0.1 Hz – 20 Hz  
- **Default:** 2 Hz

The speed of the LFO — how fast the filter cutoff sweeps up and down per second.

| Rate | Character |
|------|-----------|
| 0.1–0.5 Hz | Slow, hypnotic sweep |
| 1–3 Hz | Classic dubstep "wub" tempo feel |
| 4–8 Hz | Aggressive, fast tremolo-like wub |
| 8–20 Hz | Extreme, almost distortion-like modulation |

**Tip:** Sync Rate to your BPM mentally: at 120 BPM, 2 Hz ≈ one wub per beat.

### Depth

- **Range:** 0.0 – 1.0  
- **Default:** 0.8

Controls the width of the LFO's sweep around the Center frequency. At 0, the filter stays fixed at Center with no movement. At 1.0, the cutoff sweeps ±100% of the Center value.

- **Low depth (0.0–0.3):** Subtle wobble, tonal color
- **Mid depth (0.4–0.7):** Expressive, musical wub
- **High depth (0.8–1.0):** Extreme open/close sweep

### Center

- **Range:** 100 Hz – 4,000 Hz  
- **Default:** 500 Hz

The base frequency the LFO orbits around. The LFO sweeps the filter cutoff between `Center × (1 - Depth)` and `Center × (1 + Depth)`.

- **Low center (100–300 Hz):** Deep, subby wub — good for sub bass
- **Mid center (300–800 Hz):** Classic wub zone — most aggressive character
- **High center (1–4 kHz):** Bright, vocal/formant-like wub

### Resonance

- **Range:** 0.0 – 1.0  
- **Default:** 0.5

Adds a resonant peak at the wub filter's current cutoff frequency, following it as it sweeps. Higher resonance = more "squelchy", characteristic wub tone.

**Tip:** Resonance between 0.4–0.7 gives the most satisfying wub character. Beyond 0.85 the filter begins to self-oscillate and the wub becomes extremely aggressive.

### Filter Type

| Mode | Character |
|------|-----------|
| **LPF** (Low-Pass 24dB) | Classic wub — removes highs, sweeps warmth in and out. Most common choice. |
| **BPF** (Band-Pass 24dB) | Tighter, more "vocal" wub — good for mid-range formant effects and more aggressive bass music. |

---

## 🎛 Virtual Keyboard

A drag-to-play chromatic keyboard spanning one octave (C–B).

### Playing Notes

- **Click** a key to trigger a note on; release to trigger note off.
- **Click and drag** across keys to play multiple notes in sequence (glissando). Active notes are highlighted in indigo. Only one note is active per key at a time.
- **Touch support** is also implemented for touchscreens.

### Octave Control

- **`-` button:** Shift the keyboard down one octave (minimum: Octave 0)
- **`+` button:** Shift the keyboard up one octave (maximum: Octave 8)
- Current octave is displayed between the buttons.

**MIDI note mapping:** The keyboard generates standard MIDI note numbers. C4 (middle C) = MIDI note 60.

### Key Layout

Keys are laid out chromatically from left to right:

```
C  C# D  D# E  F  F# G  G# A  A# B
│  ██ │  ██ │  │  ██ │  ██ │  ██ │
```

White keys = natural notes. Black keys = sharps/flats, rendered shorter and overlapping in the traditional piano style.

---

## 💡 Sound Design Tips

### Classic Subtractive Lead
- Waveform: **Saw**
- Cutoff: **2–4 kHz**, Resonance: **0.3**
- Attack: **5ms**, Decay: **80ms**, Sustain: **0.8**, Release: **300ms**
- Legato: **ON**, Glide: **60ms**

### Deep Sub Bass
- Waveform: **Sine**
- Cutoff: **200 Hz**, Resonance: **0.1**
- Attack: **2ms**, Decay: **50ms**, Sustain: **1.0**, Release: **100ms**
- Legato: **OFF**

### Dubstep Wub Bass
- Waveform: **Saw**
- Cutoff: **800 Hz**, Resonance: **0.4** (voice filter)
- Attack: **5ms**, Decay: **100ms**, Sustain: **1.0**, Release: **200ms**
- Legato: **ON**, Glide: **80ms**
- **Wub ON:** Rate: **2 Hz**, Depth: **0.85**, Center: **500 Hz**, Resonance: **0.6**, Filter: **LPF**

### Chiptune / Retro
- Waveform: **Square**
- Cutoff: **8 kHz**, Resonance: **0.0**
- Attack: **1ms**, Decay: **30ms**, Sustain: **0.5**, Release: **50ms**
- Legato: **OFF**

### Pad / Atmosphere
- Waveform: **Saw**
- Cutoff: **1.5 kHz**, Resonance: **0.2**
- Attack: **800ms**, Decay: **200ms**, Sustain: **0.7**, Release: **2s**
- Legato: **OFF**
- **Wub ON:** Rate: **0.2 Hz**, Depth: **0.3**, Center: **1.2 kHz**, Filter: **BPF** — adds subtle movement

---

*Part of the [Absynth](https://github.com/JameyStiling/absynth-vst) project.*
