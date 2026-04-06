# AbsynthSynth

A JUCE-based audio synthesizer plugin that generates sound using subtractive synthesis techniques.

## Features

- **Synthesizer**: Creates audio using DSP algorithms
- **MIDI Input Support**: Accepts MIDI input for playing notes
- **Cross-Platform**: Supports VST3 and AU plugin formats
- **GUI**: Custom editor interface built with JUCE

## Requirements

- **Operating System**: macOS (currently configured for Mac/Bitwig)
- **Build Tools**:
  - CMake 3.22 or later
  - Ninja build system (`brew install ninja`)
  - C++ compiler (AppleClang on macOS)
- **Dependencies**:
  - JUCE framework (included as Git submodule)

## Building

### Prerequisites

1. Install Ninja build system:
   ```bash
   brew install ninja
   ```

2. Ensure JUCE submodule is initialized:
   ```bash
   git submodule update --init --recursive
   ```

### Build Steps

1. **Clean and Configure**:
   ```bash
   rm -rf build
   cmake . -B build -G Ninja -DJUCE_BUILD_EXAMPLES=OFF
   ```

2. **Build the Plugin**:
   ```bash
   cmake --build build --target AbsynthSynth_VST3
   ```
   Or for AU format:
   ```bash
   cmake --build build --target AbsynthSynth_AU
   ```

The built plugins will be located in:
- `build/AbsynthSynth_artefacts/VST3/Absynth.vst3`
- `build/AbsynthSynth_artefacts/AU/Absynth.component`

## Installation

1. Copy the built plugin files to your DAW's plugin directory:
   - **VST3**: `/Library/Audio/Plug-Ins/VST3/`
   - **AU**: `/Library/Audio/Plug-Ins/Components/`

2. Restart your DAW or rescan plugins.

## Usage

1. Open your DAW (e.g., Bitwig, Logic Pro)
2. Create a new instrument track
3. Load "Absynth" from the plugin list
4. Play MIDI notes to generate sound

## Development

### Project Structure

```
├── CMakeLists.txt          # CMake build configuration
├── JUCE/                   # JUCE framework submodule
├── Source/
│   ├── PluginProcessor.cpp # Audio processing logic
│   ├── PluginProcessor.h   # Processor class definition
│   ├── PluginEditor.cpp    # GUI implementation
│   └── PluginEditor.h      # Editor class definition
└── build/                  # Build output directory (ignored)
```

### Adding Features

- Modify `PluginProcessor.cpp` for audio processing
- Update `PluginEditor.cpp` for GUI changes
- Rebuild after changes

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test the build
5. Submit a pull request

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Acknowledgments

- Built with the [JUCE framework](https://juce.com/)
- Inspired by classic subtractive synthesizers