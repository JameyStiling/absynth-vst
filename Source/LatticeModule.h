#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

class LatticeModule
{
public:
    virtual ~LatticeModule() = default;

    // Identity
    virtual juce::String getModuleId() const = 0;    // e.g. "nebuli"
    virtual juce::String getModuleName() const = 0;   // e.g. "Nebuli Cloud Reverb"
    virtual bool isEnabled() const = 0;               // License/feature gate

    // Lifecycle
    virtual void prepare(const juce::dsp::ProcessSpec& spec) = 0;
    virtual void reset() = 0;

    // DSP — called per block in the Lattice signal chain
    virtual void process(juce::AudioBuffer<float>& buffer) = 0;

    // Parameters — registered into Lattice's APVTS
    virtual void addParameters(
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> &parameters
    ) = 0;
    virtual void updateFromAPVTS(juce::AudioProcessorValueTreeState& apvts) = 0;

    // State — for preset save/load
    virtual void getState(juce::MemoryBlock& destData) = 0;
    virtual void setState(const void* data, int sizeInBytes) = 0;
};
