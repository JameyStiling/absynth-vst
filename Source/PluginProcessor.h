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

// LFO-driven wub filter (applied post-synth on the master bus)
struct WubEngine
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

    void setFilterType(int type) // 0 = LPF, 1 = BPF
    {
        filter.setMode(type == 1 ? juce::dsp::LadderFilterMode::BPF24
                                 : juce::dsp::LadderFilterMode::LPF24);
    }

    // Process a buffer in-place, sweeping the filter cutoff via LFO
    void process(juce::AudioBuffer<float>& buffer, float rate, float depth, float center)
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
            float lfo = (float)std::sin(twoPi * lfoPhase);
            lfoPhase += phaseIncrement;
            if (lfoPhase >= 1.0) lfoPhase -= 1.0;

            float halfRange = center * depth;
            float cutoff = juce::jlimit(20.0f, 20000.0f, center + lfo * halfRange);
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

class LatticeAudioProcessor : public juce::AudioProcessor
{
public:
    LatticeAudioProcessor();
    ~LatticeAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void handleWebMidiEvent(int note, int velocity, bool isNoteOn);
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

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameters();
    
    CustomSynth synth;
    WubEngine wubEngine;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LatticeAudioProcessor)
};
