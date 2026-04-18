#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
SynthVoice::SynthVoice()
{
    // Setup oscillator
    osc.initialise([](float x) { return std::sin(x); }); // Default sine
    
    // Setup filter
    filter.setMode(juce::dsp::LadderFilterMode::LPF24);
    
    // Gain
    gain.setGainLinear(1.0f);
}

bool SynthVoice::canPlaySound(juce::SynthesiserSound* sound)
{
    return dynamic_cast<SynthSound*>(sound) != nullptr;
}

void SynthVoice::startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound*, int /*currentPitchWheelPosition*/)
{
    float targetFreq = juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber);
    smoothedFreq.setCurrentAndTargetValue(targetFreq);
    osc.setFrequency(targetFreq);
    adsr.noteOn();
}

void SynthVoice::stopNote(float /*velocity*/, bool allowTailOff)
{
    if (allowTailOff)
    {
        adsr.noteOff();
    }
    else
    {
        clearCurrentNote();
        adsr.reset();
    }
}

void SynthVoice::pitchWheelMoved(int /*newPitchWheelValue*/) {}
void SynthVoice::controllerMoved(int /*controllerNumber*/, int /*newControllerValue*/) {}

void SynthVoice::prepareToPlay(double sampleRate, int samplesPerBlock, int outputChannels)
{
    juce::dsp::ProcessSpec spec;
    spec.maximumBlockSize = samplesPerBlock;
    spec.sampleRate = sampleRate;
    spec.numChannels = outputChannels;

    osc.prepare(spec);
    filter.prepare(spec);
    gain.prepare(spec);
    
    adsr.setSampleRate(sampleRate);
    smoothedFreq.reset(sampleRate, 0.05); // Default 50ms glide
    
    isPrepared = true;
}

void SynthVoice::updateFilter(float cutoff, float resonance)
{
    filter.setCutoffFrequencyHz(cutoff);
    filter.setResonance(resonance);
}

void SynthVoice::updateADSR(const juce::ADSR::Parameters& envParams)
{
    adsr.setParameters(envParams);
}

void SynthVoice::updateOscType(int type)
{
    if (oscType == type) return;
    oscType = type;
    
    switch (oscType)
    {
        case 0: // Sine
            osc.initialise([](float x) { return std::sin(x); });
            break;
        case 1: // Saw
            osc.initialise([](float x) { return x / juce::MathConstants<float>::pi; });
            break;
        case 2: // Square
            osc.initialise([](float x) { return x < 0.0f ? -1.0f : 1.0f; });
            break;
        default:
            jassertfalse;
            break;
    }
}

void SynthVoice::setGlideTime(float glideTimeMs)
{
    smoothedFreq.reset(getSampleRate(), glideTimeMs * 0.001);
}

void SynthVoice::triggerGlide(int newNote)
{
    float targetFreq = juce::MidiMessage::getMidiNoteInHertz(newNote);
    smoothedFreq.setTargetValue(targetFreq);
}

void SynthVoice::renderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples)
{
    if (!isPrepared) return;

    if (!adsr.isActive())
    {
        clearCurrentNote();
        return;
    }

    juce::AudioBuffer<float> synthBuffer(outputBuffer.getNumChannels(), numSamples);
    synthBuffer.clear();

    auto numChannels = synthBuffer.getNumChannels();
    for (int s = 0; s < numSamples; ++s)
    {
        if (smoothedFreq.isSmoothing())
        {
            osc.setFrequency(smoothedFreq.getNextValue());
        }
        float sample = osc.processSample(0.0f);
        for (int ch = 0; ch < numChannels; ++ch)
        {
            synthBuffer.setSample(ch, s, sample);
        }
    }

    juce::dsp::AudioBlock<float> audioBlock(synthBuffer);
    juce::dsp::ProcessContextReplacing<float> context(audioBlock);
    
    filter.process(context);
    gain.process(context);

    // Apply ADSR
    adsr.applyEnvelopeToBuffer(synthBuffer, 0, numSamples);

    for (int channel = 0; channel < outputBuffer.getNumChannels(); ++channel)
    {
        outputBuffer.addFrom(channel, startSample, synthBuffer, channel, 0, numSamples);
        if (!adsr.isActive())
        {
            clearCurrentNote();
        }
    }
}

//==============================================================================
void CustomSynth::noteOn(int midiChannel, int midiNoteNumber, float velocity)
{
    if (isLegato && getNumVoices() > 0)
    {
        heldNotes.add(midiNoteNumber);
        auto* voice = getVoice(0);
        if (voice->getCurrentlyPlayingNote() >= 0)
        {
            if (auto* synthVoice = dynamic_cast<SynthVoice*>(voice))
                synthVoice->triggerGlide(midiNoteNumber);
            return;
        }
        initialLegatoNote = midiNoteNumber;
    }
    juce::Synthesiser::noteOn(midiChannel, midiNoteNumber, velocity);
}

void CustomSynth::noteOff(int midiChannel, int midiNoteNumber, float velocity, bool allowTailOff)
{
    if (isLegato)
    {
        heldNotes.removeAllInstancesOf(midiNoteNumber);
        if (heldNotes.size() > 0)
        {
            auto* voice = getVoice(0);
            if (voice->getCurrentlyPlayingNote() >= 0)
            {
                if (auto* synthVoice = dynamic_cast<SynthVoice*>(voice))
                    synthVoice->triggerGlide(heldNotes.getLast());
                return;
            }
        }
        else
        {
            juce::Synthesiser::noteOff(midiChannel, initialLegatoNote, velocity, allowTailOff);
            initialLegatoNote = -1;
            return;
        }
    }
    juce::Synthesiser::noteOff(midiChannel, midiNoteNumber, velocity, allowTailOff);
}

//==============================================================================
AbsynthAudioProcessor::AbsynthAudioProcessor()
     : AudioProcessor (BusesProperties()
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
       apvts (*this, nullptr, "Parameters", createParameters())
{
    synth.addSound(new SynthSound());
    for (int i = 0; i < 8; ++i)
    {
        synth.addVoice(new SynthVoice());
    }
}

AbsynthAudioProcessor::~AbsynthAudioProcessor()
{
}

juce::AudioProcessorValueTreeState::ParameterLayout AbsynthAudioProcessor::createParameters()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"cutoff", 1}, "Cutoff", juce::NormalisableRange<float>(20.0f, 20000.0f, 1.0f, 0.3f), 2000.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"resonance", 1}, "Resonance", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.1f));
    
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"attack", 1}, "Attack", juce::NormalisableRange<float>(0.001f, 5.0f, 0.01f, 0.3f), 0.01f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"decay", 1}, "Decay", juce::NormalisableRange<float>(0.001f, 5.0f, 0.01f, 0.3f), 0.1f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"sustain", 1}, "Sustain", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"release", 1}, "Release", juce::NormalisableRange<float>(0.001f, 5.0f, 0.01f, 0.3f), 1.0f));
    
    params.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"oscType", 1}, "Osc Type", juce::StringArray{"Sine", "Saw", "Square"}, 1));

    params.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"legato", 1}, "Legato", false));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"glideTime", 1}, "Glide Time", juce::NormalisableRange<float>(0.0f, 1000.0f, 1.0f, 0.3f), 50.0f));

    return { params.begin(), params.end() };
}

//==============================================================================
const juce::String AbsynthAudioProcessor::getName() const { return JucePlugin_Name; }
bool AbsynthAudioProcessor::acceptsMidi() const { return true; }
bool AbsynthAudioProcessor::producesMidi() const { return false; }
bool AbsynthAudioProcessor::isMidiEffect() const { return false; }
double AbsynthAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int AbsynthAudioProcessor::getNumPrograms() { return 1; }
int AbsynthAudioProcessor::getCurrentProgram() { return 0; }
void AbsynthAudioProcessor::setCurrentProgram (int index) { juce::ignoreUnused(index); }
const juce::String AbsynthAudioProcessor::getProgramName (int index) { juce::ignoreUnused(index); return {}; }
void AbsynthAudioProcessor::changeProgramName (int index, const juce::String& newName) { juce::ignoreUnused(index, newName); }

//==============================================================================
void AbsynthAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    synth.setCurrentPlaybackSampleRate(sampleRate);
    keyboardState.reset();
    
    for (int i = 0; i < synth.getNumVoices(); ++i)
    {
        if (auto voice = dynamic_cast<SynthVoice*>(synth.getVoice(i)))
        {
            voice->prepareToPlay(sampleRate, samplesPerBlock, getTotalNumOutputChannels());
        }
    }
}

void AbsynthAudioProcessor::releaseResources()
{
}

bool AbsynthAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    return true;
}

void AbsynthAudioProcessor::handleWebMidiEvent(int note, int velocity, bool isNoteOn)
{
    if (isNoteOn)
        keyboardState.noteOn(1, note, velocity / 127.0f);
    else
        keyboardState.noteOff(1, note, 0.0f);
}

void AbsynthAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    keyboardState.processNextMidiBuffer(midiMessages, 0, buffer.getNumSamples(), true);

    float cutoff = apvts.getRawParameterValue("cutoff")->load();
    float resonance = apvts.getRawParameterValue("resonance")->load();
    
    juce::ADSR::Parameters envParams;
    envParams.attack = apvts.getRawParameterValue("attack")->load();
    envParams.decay = apvts.getRawParameterValue("decay")->load();
    envParams.sustain = apvts.getRawParameterValue("sustain")->load();
    envParams.release = apvts.getRawParameterValue("release")->load();
    
    int oscType = static_cast<int>(apvts.getRawParameterValue("oscType")->load());
    
    synth.isLegato = apvts.getRawParameterValue("legato")->load() > 0.5f;
    synth.glideTimeMs = apvts.getRawParameterValue("glideTime")->load();

    for (int i = 0; i < synth.getNumVoices(); ++i)
    {
        if (auto voice = dynamic_cast<SynthVoice*>(synth.getVoice(i)))
        {
            voice->updateFilter(cutoff, resonance);
            voice->updateADSR(envParams);
            voice->updateOscType(oscType);
            voice->setGlideTime(synth.glideTimeMs);
        }
    }

    synth.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());
}

//==============================================================================
bool AbsynthAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* AbsynthAudioProcessor::createEditor()
{
    return new AbsynthAudioProcessorEditor (*this);
}

//==============================================================================
void AbsynthAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void AbsynthAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState != nullptr)
        if (xmlState->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AbsynthAudioProcessor();
}
