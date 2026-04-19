#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
SynthVoice::SynthVoice()
{
    for (int i = 0; i < 3; ++i) {
        oscs[i].osc.initialise([](float x) { return std::sin(x); });
    }
    filter.setMode(juce::dsp::LadderFilterMode::LPF24);
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
    for (int i = 0; i < 3; ++i) {
        oscs[i].osc.setFrequency(targetFreq);
    }
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

    for (int i = 0; i < 3; ++i) {
        oscs[i].osc.prepare(spec);
    }
    filter.prepare(spec);
    gain.prepare(spec);
    
    adsr.setSampleRate(sampleRate);
    smoothedFreq.reset(sampleRate, 0.05);
    
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

void SynthVoice::updateOsc(int index, bool active, int type, float level)
{
    if (index < 0 || index >= 3) return;
    
    oscs[index].active = active;
    oscs[index].level = level;

    if (oscs[index].type != type) {
        oscs[index].type = type;
        switch (type)
        {
            case 0: // Sine
                oscs[index].osc.initialise([](float x) { return std::sin(x); });
                break;
            case 1: // Saw
                oscs[index].osc.initialise([](float x) { return x / juce::MathConstants<float>::pi; });
                break;
            case 2: // Square
                oscs[index].osc.initialise([](float x) { return x < 0.0f ? -1.0f : 1.0f; });
                break;
        }
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
    float freq = smoothedFreq.getNextValue();

    for (int i = 0; i < 3; ++i) {
        if (!oscs[i].active) continue;
        
        oscs[i].osc.setFrequency(freq);
        float amp = oscs[i].level;

        for (int s = 0; s < numSamples; ++s) {
            float sample = oscs[i].osc.processSample(0.0f) * amp;
            for (int ch = 0; ch < numChannels; ++ch) {
                synthBuffer.addSample(ch, s, sample / 3.0f); // Simple mix
            }
        }
    }

    juce::dsp::AudioBlock<float> audioBlock(synthBuffer);
    juce::dsp::ProcessContextReplacing<float> context(audioBlock);
    
    filter.process(context);
    gain.process(context);

    adsr.applyEnvelopeToBuffer(synthBuffer, 0, numSamples);

    for (int channel = 0; channel < outputBuffer.getNumChannels(); ++channel)
    {
        outputBuffer.addFrom(channel, startSample, synthBuffer, channel, 0, numSamples);
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
LatticeAudioProcessor::LatticeAudioProcessor()
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

LatticeAudioProcessor::~LatticeAudioProcessor()
{
}

juce::AudioProcessorValueTreeState::ParameterLayout LatticeAudioProcessor::createParameters()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"cutoff", 1}, "Cutoff", juce::NormalisableRange<float>(20.0f, 20000.0f, 1.0f, 0.3f), 2000.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"resonance", 1}, "Resonance", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.1f));
    
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"attack", 1}, "Attack", juce::NormalisableRange<float>(0.001f, 5.0f, 0.01f, 0.3f), 0.01f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"decay", 1}, "Decay", juce::NormalisableRange<float>(0.001f, 5.0f, 0.01f, 0.3f), 0.1f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"sustain", 1}, "Sustain", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"release", 1}, "Release", juce::NormalisableRange<float>(0.001f, 5.0f, 0.01f, 0.3f), 1.0f));
    
    // 3 Oscillators
    for (int i = 1; i <= 3; ++i) {
        juce::String idx = juce::String(i);
        params.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"osc" + idx + "Active", 1}, "Osc " + idx + " Active", i == 1));
        params.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"osc" + idx + "Type", 1}, "Osc " + idx + " Type", juce::StringArray{"Sine", "Saw", "Square"}, 0));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"osc" + idx + "Level", 1}, "Osc " + idx + " Level", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.8f));
    }

    params.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"legato", 1}, "Legato", false));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"glideTime", 1}, "Glide Time", juce::NormalisableRange<float>(0.0f, 1000.0f, 1.0f, 0.3f), 50.0f));

    // Wub parameters
    params.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"wubEnabled", 1}, "Wub Enabled", false));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"wubRate", 1}, "Wub Rate", juce::NormalisableRange<float>(0.1f, 20.0f, 0.01f, 0.5f), 2.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"wubDepth", 1}, "Wub Depth", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.8f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"wubCenter", 1}, "Wub Center", juce::NormalisableRange<float>(100.0f, 4000.0f, 1.0f, 0.4f), 500.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"wubResonance", 1}, "Wub Resonance", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"wubFilterType", 1}, "Wub Filter", juce::StringArray{"LPF", "BPF"}, 0));

    return { params.begin(), params.end() };
}

//==============================================================================
const juce::String LatticeAudioProcessor::getName() const { return JucePlugin_Name; }
bool LatticeAudioProcessor::acceptsMidi() const { return true; }
bool LatticeAudioProcessor::producesMidi() const { return false; }
bool LatticeAudioProcessor::isMidiEffect() const { return false; }
double LatticeAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int LatticeAudioProcessor::getNumPrograms() { return 1; }
int LatticeAudioProcessor::getCurrentProgram() { return 0; }
void LatticeAudioProcessor::setCurrentProgram (int index) { juce::ignoreUnused(index); }
const juce::String LatticeAudioProcessor::getProgramName (int index) { juce::ignoreUnused(index); return {}; }
void LatticeAudioProcessor::changeProgramName (int index, const juce::String& newName) { juce::ignoreUnused(index, newName); }

//==============================================================================
void LatticeAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    synth.setCurrentPlaybackSampleRate(sampleRate);
    keyboardState.reset();

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = (juce::uint32)samplesPerBlock;
    spec.numChannels = (juce::uint32)getTotalNumOutputChannels();
    wubEngine.prepare(spec);
    
    for (int i = 0; i < synth.getNumVoices(); ++i)
    {
        if (auto voice = dynamic_cast<SynthVoice*>(synth.getVoice(i)))
        {
            voice->prepareToPlay(sampleRate, samplesPerBlock, getTotalNumOutputChannels());
        }
    }
}

void LatticeAudioProcessor::releaseResources()
{
}

bool LatticeAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    return true;
}

void LatticeAudioProcessor::handleWebMidiEvent(int note, int velocity, bool isNoteOn)
{
    if (isNoteOn)
        keyboardState.noteOn(1, note, velocity / 127.0f);
    else
        keyboardState.noteOff(1, note, 0.0f);
}

void LatticeAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
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
    
    struct OscParams { bool active; int type; float level; };
    OscParams oscP[3];
    for (int i = 0; i < 3; ++i) {
        juce::String idx = juce::String(i + 1);
        oscP[i].active = apvts.getRawParameterValue("osc" + idx + "Active")->load() > 0.5f;
        oscP[i].type = (int)apvts.getRawParameterValue("osc" + idx + "Type")->load();
        oscP[i].level = apvts.getRawParameterValue("osc" + idx + "Level")->load();
    }
    
    synth.isLegato = apvts.getRawParameterValue("legato")->load() > 0.5f;
    synth.glideTimeMs = apvts.getRawParameterValue("glideTime")->load();

    for (int i = 0; i < synth.getNumVoices(); ++i)
    {
        if (auto voice = dynamic_cast<SynthVoice*>(synth.getVoice(i)))
        {
            voice->updateFilter(cutoff, resonance);
            voice->updateADSR(envParams);
            for (int j = 0; j < 3; ++j) {
                voice->updateOsc(j, oscP[j].active, oscP[j].type, oscP[j].level);
            }
            voice->setGlideTime(synth.glideTimeMs);
        }
    }

    synth.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());

    // Apply wub LFO filter post-synth
    bool wubEnabled = apvts.getRawParameterValue("wubEnabled")->load() > 0.5f;
    if (wubEnabled)
    {
        float wubRate   = apvts.getRawParameterValue("wubRate")->load();
        float wubDepth  = apvts.getRawParameterValue("wubDepth")->load();
        float wubCenter = apvts.getRawParameterValue("wubCenter")->load();
        int   wubType   = (int)apvts.getRawParameterValue("wubFilterType")->load();
        float wubRes    = apvts.getRawParameterValue("wubResonance")->load();

        wubEngine.setFilterType(wubType);
        wubEngine.filter.setResonance(wubRes);
        wubEngine.process(buffer, wubRate, wubDepth, wubCenter);
    }
}

//==============================================================================
bool LatticeAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* LatticeAudioProcessor::createEditor()
{
    return new LatticeAudioProcessorEditor (*this);
}

//==============================================================================
void LatticeAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void LatticeAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState != nullptr)
        if (xmlState->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new LatticeAudioProcessor();
}
