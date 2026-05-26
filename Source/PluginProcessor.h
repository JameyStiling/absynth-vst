#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <juce_audio_devices/juce_audio_devices.h>

class SynthVoice : public juce::SynthesiserVoice
{
public:
    SynthVoice();
    bool canPlaySound(juce::SynthesiserSound* sound) override;
    void startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound* sound, int currentPitchWheelPosition) override;
    void stopNote(float velocity, bool allowTailOff) override;
    void pitchWheelMoved(int newPitchWheelValue) override;
    void controllerMoved(int controllerNumber, int newControllerValue) override;
    void prepareToPlay(double sampleRate, int samplesPerBlock, int outputChannels);
    void renderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override;

    void updateFilter(float cutoff, float resonance);
    void updateADSR(const juce::ADSR::Parameters& envParams);
    void updateOsc(int index, bool active, int type, float level);

    void setGlideTime(float glideTimeMs);
    void triggerGlide(int newNote);

private:
    struct OscUnit {
        juce::dsp::Oscillator<float> osc;
        int type { 0 };
        float level { 1.0f };
        bool active { false };
    };

    OscUnit oscs[3];
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> smoothedFreq;
    juce::dsp::LadderFilter<float> filter;
    juce::ADSR adsr;
    juce::dsp::Gain<float> gain;
    bool isPrepared { false };
};

class SynthSound : public juce::SynthesiserSound
{
public:
    bool appliesToNote(int) override { return true; }
    bool appliesToChannel(int) override { return true; }
};

class CustomSynth : public juce::Synthesiser
{
public:
    bool isLegato { false };
    float glideTimeMs { 50.0f };
    juce::Array<int> heldNotes;
    int initialLegatoNote { -1 };

    void noteOn(int midiChannel, int midiNoteNumber, float velocity) override;
    void noteOff(int midiChannel, int midiNoteNumber, float velocity, bool allowTailOff) override;
};

// LFO-driven modulation filter (applied post-synth on the master bus)
struct ModEngine
{
    juce::dsp::LadderFilter<float> filter;
    double sampleRate { 44100.0 };
    double lfoPhase  { 0.0 };

    void prepare(const juce::dsp::ProcessSpec& spec)
    {
        sampleRate = spec.sampleRate;
        filter.prepare(spec);
        filter.setMode(juce::dsp::LadderFilterMode::LPF24);
        filter.setResonance(0.5f);
        lfoPhase = 0.0;
    }

    void setFilterType(int type, int slope) // type: 0 = LPF, 1 = HPF, 2 = BPF; slope: 0 = 6dB, 1 = 12dB, 2 = 18dB, 3 = 24dB
    {
        bool is12 = (slope == 0 || slope == 1);
        if (type == 1) // HPF
            filter.setMode(is12 ? juce::dsp::LadderFilterMode::HPF12 : juce::dsp::LadderFilterMode::HPF24);
        else if (type == 2) // BPF
            filter.setMode(is12 ? juce::dsp::LadderFilterMode::BPF12 : juce::dsp::LadderFilterMode::BPF24);
        else // LPF
            filter.setMode(is12 ? juce::dsp::LadderFilterMode::LPF12 : juce::dsp::LadderFilterMode::LPF24);
    }

    // Process a buffer in-place, sweeping the filter cutoff via LFO or DRAW mod
    void process(juce::AudioBuffer<float>& buffer, float rate, float depth, float center, bool isDrawMode, float drawVal, int depthMode, int polarity)
    {
        const double twoPi = juce::MathConstants<double>::twoPi;
        const double phaseIncrement = rate / sampleRate;
        int numSamples  = buffer.getNumSamples();
        int numChannels = buffer.getNumChannels();

        // Process one sample at a time using a 1-sample AudioBlock so we can
        // update the cutoff per-sample for the LFO sweep.
        juce::AudioBuffer<float> oneSample(numChannels, 1);

        for (int s = 0; s < numSamples; ++s)
        {
            float lfo = 0.0f;
            if (isDrawMode)
            {
                lfo = drawVal;
            }
            else
            {
                lfo = (float)std::sin(twoPi * lfoPhase);
                lfoPhase += phaseIncrement;
                if (lfoPhase >= 1.0) lfoPhase -= 1.0;
            }

            float lfoVal = lfo; // Expected -1.0 to 1.0
            if (polarity == 0) { // Unipolar (+)
                lfoVal = (lfoVal + 1.0f) * 0.5f;
            }

            float actualDepth = depth;
            if (depthMode == 1) actualDepth = depth * 60.0f; // 5 Octaves = 60 Semitones
            else if (depthMode == 2) actualDepth = depth * 5.0f; // 5 Octaves

            float cutoff = center;
            if (depthMode == 0) { // % mode (Dynamic Headroom)
                if (polarity == 0) { // Unipolar (+)
                    float headroom = 20000.0f - center;
                    cutoff = center + (lfoVal * depth * headroom);
                } else { // Bipolar (+/-)
                    float headroom = (lfoVal > 0.0f) ? (20000.0f - center) : (center - 20.0f);
                    cutoff = center + (lfoVal * depth * headroom);
                }
            } else if (depthMode == 1) { // Semi
                cutoff = center * std::pow(2.0f, (lfoVal * actualDepth) / 12.0f);
            } else if (depthMode == 2) { // Oct
                cutoff = center * std::pow(2.0f, lfoVal * actualDepth);
            }
            cutoff = juce::jlimit(20.0f, 20000.0f, cutoff);

            filter.setCutoffFrequencyHz(cutoff);

            for (int ch = 0; ch < numChannels; ++ch)
                oneSample.setSample(ch, 0, buffer.getSample(ch, s));

            juce::dsp::AudioBlock<float> block(oneSample);
            juce::dsp::ProcessContextReplacing<float> ctx(block);
            filter.process(ctx);

            for (int ch = 0; ch < numChannels; ++ch)
                buffer.setSample(ch, s, oneSample.getSample(ch, 0));
        }
    }
};

struct Arpeggiator
{
    bool enabled { false };
    float rate { 0.25f }; // Quarter note default
    float swing { 0.0f }; // 0 to 1.0
    int mode { 0 }; // 0: Repeat, 1: Up, 2: Down, 3: UpDown

    double sampleRate { 44100.0 };
    int samplesPerBeat { 10000 };
    int samplesElapsed { 0 };
    float bpm { 120.0f };

    juce::Array<int> heldNotes;
    juce::Array<int> orderedNotes;
    juce::Array<int> playingNotes;
    int currentNoteIndex { 0 };
    bool goingUp { true };
    
    void prepare(double sr)
    {
        sampleRate = sr;
        samplesElapsed = 0;
    }

    void noteOn(int note)
    {
        if (!heldNotes.contains(note))
        {
            if (heldNotes.isEmpty()) {
                samplesElapsed = 9999999; // Force immediate trigger
            }
            heldNotes.add(note);
            heldNotes.sort();
            orderedNotes.add(note);
        }
    }

    void noteOff(int note)
    {
        heldNotes.removeAllInstancesOf(note);
        orderedNotes.removeAllInstancesOf(note);
        if (heldNotes.isEmpty()) {
            currentNoteIndex = 0;
        }
    }

    void reset()
    {
        heldNotes.clear();
        orderedNotes.clear();
        playingNotes.clear();
        currentNoteIndex = 0;
        samplesElapsed = 0;
    }
};

class LatticeAudioProcessor : public juce::AudioProcessor
{
public:
    LatticeAudioProcessor();
    ~LatticeAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void handleWebMidiEvent(int note, int velocity, bool isNoteOn);
    juce::var getActiveMidiNotesForUi() const;
    juce::MidiKeyboardState keyboardState;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    // Bridge variables for streaming custom MSEG modulation
    std::atomic<bool> isModDrawMode { false };
    std::atomic<float> modDrawValue { 0.0f };

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameters();
    
    CustomSynth synth;
    ModEngine modEngine;
    Arpeggiator arpeggiator;

    void processArpeggiator(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages, juce::AudioPlayHead* playHead);

    int getMidiChannelFilter() const;
    void refreshActiveNotesSnapshot();
    juce::MidiBuffer filterMidiByChannel (const juce::MidiBuffer& source) const;

    mutable juce::CriticalSection activeNotesLock;
    std::vector<int> activeNotesSnapshot;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LatticeAudioProcessor)
};
