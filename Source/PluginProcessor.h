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
    void updateOscType(int type); // 0=Sine, 1=Saw, 2=Square

    void setGlideTime(float glideTimeMs);
    void triggerGlide(int newNote);

private:
    juce::dsp::Oscillator<float> osc;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> smoothedFreq;
    juce::dsp::LadderFilter<float> filter;
    juce::ADSR adsr;
    juce::dsp::Gain<float> gain;
    bool isPrepared { false };
    int oscType { 1 };
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

class AbsynthAudioProcessor : public juce::AudioProcessor
{
public:
    AbsynthAudioProcessor();
    ~AbsynthAudioProcessor() override;

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

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AbsynthAudioProcessor)
};
