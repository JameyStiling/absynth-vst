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
    filter.setCutoffFrequencyHz(juce::jlimit(20.0f, 20000.0f, cutoff));
    filter.setResonance(juce::jlimit(0.0f, 1.0f, resonance));
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
    
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"attack", 1}, "Attack", juce::NormalisableRange<float>(0.001f, 5.0f, 0.01f, 0.3f), 0.01f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"decay", 1}, "Decay", juce::NormalisableRange<float>(0.001f, 5.0f, 0.01f, 0.3f), 0.1f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"sustain", 1}, "Sustain", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"release", 1}, "Release", juce::NormalisableRange<float>(0.001f, 5.0f, 0.01f, 0.3f), 1.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"cutoff", 1}, "Cutoff",
        juce::NormalisableRange<float>(20.0f, 20000.0f, 1.0f, 0.3f), 2000.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"resonance", 1}, "Resonance",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.1f));
    
    // 3 Oscillators
    for (int i = 1; i <= 3; ++i) {
        juce::String idx = juce::String(i);
        params.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"osc" + idx + "Active", 1}, "Osc " + idx + " Active", i == 1));
        params.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"osc" + idx + "Type", 1}, "Osc " + idx + " Type", juce::StringArray{"Sine", "Saw", "Square"}, 0));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"osc" + idx + "Level", 1}, "Osc " + idx + " Level", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.8f));
    }

    params.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"legato", 1}, "Legato", false));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"glideTime", 1}, "Glide Time", juce::NormalisableRange<float>(0.0f, 1000.0f, 1.0f, 0.3f), 50.0f));

    // Modulation parameters
    params.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"modEnabled", 1}, "Mod Enabled", false));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"modRate", 1}, "Mod Rate", juce::NormalisableRange<float>(0.1f, 20.0f, 0.01f, 0.5f), 2.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"modDepth", 1}, "Mod Depth", juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"modDepthMode", 1}, "Mod Depth Mode", juce::StringArray{"%", "Semi", "Oct"}, 2));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"modPolarity", 1}, "Mod Polarity", juce::StringArray{"+", "+/-"}, 1));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"modCenter", 1}, "Mod Center", juce::NormalisableRange<float>(100.0f, 4000.0f, 1.0f, 0.4f), 500.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"modResonance", 1}, "Mod Resonance", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"modFilterType", 1}, "Mod Filter", juce::StringArray{"LPF", "HPF", "BPF"}, 0));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"modSlope", 1}, "Mod Slope", juce::StringArray{"6 dB/Oct", "12 dB/Oct", "18 dB/Oct", "24 dB/Oct"}, 1));

    // Arpeggiator
    params.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"arpEnabled", 1}, "Arp Enabled", false));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"arpRate", 1}, "Arp Rate", 
        juce::StringArray{"1/4", "1/8", "1/16", "1/32", "1/4D", "1/8D", "1/16D", "1/4T", "1/8T", "1/16T"}, 1));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"arpSwing", 1}, "Arp Swing", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"arpMode", 1}, "Arp Mode", juce::StringArray{"Repeat", "Up", "Down", "Up/Dn", "As Played"}, 0));

    {
        juce::StringArray midiChannels { "Omni" };
        for (int ch = 1; ch <= 16; ++ch)
            midiChannels.add ("Ch " + juce::String (ch));
        params.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "midiChannel", 1 },
                                                                        "MIDI Channel",
                                                                        midiChannels,
                                                                        0));
    }

    return { params.begin(), params.end() };
}

int LatticeAudioProcessor::getMidiChannelFilter() const
{
    return (int) apvts.getRawParameterValue ("midiChannel")->load();
}

juce::MidiBuffer LatticeAudioProcessor::filterMidiByChannel (const juce::MidiBuffer& source) const
{
    const int channel = getMidiChannelFilter();
    if (channel <= 0)
        return source;

    juce::MidiBuffer filtered;
    for (const auto metadata : source)
    {
        const auto msg = metadata.getMessage();
        if (msg.getChannel() == channel)
            filtered.addEvent (msg, metadata.samplePosition);
    }
    return filtered;
}

void LatticeAudioProcessor::refreshActiveNotesSnapshot()
{
    const int channel = getMidiChannelFilter();
    std::vector<int> notes;
    notes.reserve (16);

    for (int note = 0; note < 128; ++note)
    {
        bool isOn = false;

        if (channel <= 0)
        {
            for (int ch = 1; ch <= 16 && ! isOn; ++ch)
                isOn = keyboardState.isNoteOn (ch, note);
        }
        else
        {
            isOn = keyboardState.isNoteOn (channel, note);
        }

        if (isOn)
            notes.push_back (note);
    }

    const juce::ScopedLock lock (activeNotesLock);
    activeNotesSnapshot = std::move (notes);
}

juce::var LatticeAudioProcessor::getActiveMidiNotesForUi() const
{
    const juce::ScopedLock lock (activeNotesLock);
    juce::Array<juce::var> result;
    result.ensureStorageAllocated ((int) activeNotesSnapshot.size());

    for (int note : activeNotesSnapshot)
        result.add (note);

    return result;
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
    modEngine.prepare(spec);
    
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

    auto channelMidi = filterMidiByChannel (midiMessages);
    keyboardState.processNextMidiBuffer (channelMidi, 0, buffer.getNumSamples(), true);

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

    const float cutoff = apvts.getRawParameterValue("cutoff")->load();
    const float resonance = apvts.getRawParameterValue("resonance")->load();

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

    processArpeggiator (buffer, channelMidi, getPlayHead());
    synth.renderNextBlock (buffer, channelMidi, 0, buffer.getNumSamples());

    // Apply LFO/DRAW filter modulation post-synth
    bool modEnabled = apvts.getRawParameterValue("modEnabled")->load() > 0.5f;
    if (modEnabled)
    {
        float modRate   = apvts.getRawParameterValue("modRate")->load();
        float modDepth  = apvts.getRawParameterValue("modDepth")->load();
        int   modDepthMode = (int)apvts.getRawParameterValue("modDepthMode")->load();
        int   modPolarity  = (int)apvts.getRawParameterValue("modPolarity")->load();
        float modCenter = apvts.getRawParameterValue("modCenter")->load();
        int   modType   = (int)apvts.getRawParameterValue("modFilterType")->load();
        float modRes    = apvts.getRawParameterValue("modResonance")->load();
        int   modSlope  = (int)apvts.getRawParameterValue("modSlope")->load();

        modEngine.setFilterType(modType, modSlope);
        
        // Scale resonance based on slope
        float finalRes = modRes;
        if (modSlope == 0) finalRes = 0.0f; // 6dB filters are non-resonant
        else if (modSlope == 2) finalRes = modRes * 0.6f; // 18dB filters have slightly reduced resonance peak
        modEngine.filter.setResonance(finalRes);
        
        bool isDraw = isModDrawMode.load();
        float drawVal = modDrawValue.load();
        modEngine.process(buffer, modRate, modDepth, modCenter, isDraw, drawVal, modDepthMode, modPolarity);
    }

    refreshActiveNotesSnapshot();
}

void LatticeAudioProcessor::processArpeggiator(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages, juce::AudioPlayHead* playHead)
{
    bool arpEnabled = apvts.getRawParameterValue("arpEnabled")->load() > 0.5f;
    if (!arpEnabled) {
        arpeggiator.reset();
        return;
    }

    int rateIdx = (int)apvts.getRawParameterValue("arpRate")->load();
    float swing = apvts.getRawParameterValue("arpSwing")->load();
    int arpMode = (int)apvts.getRawParameterValue("arpMode")->load();

    // Map rate index to beat division
    // "1/4", "1/8", "1/16", "1/32", "1/4D", "1/8D", "1/16D", "1/4T", "1/8T", "1/16T"
    float beatDiv = 1.0f; 
    switch (rateIdx) {
        case 0: beatDiv = 1.0f; break; // 1/4 (1 beat)
        case 1: beatDiv = 0.5f; break; // 1/8
        case 2: beatDiv = 0.25f; break; // 1/16
        case 3: beatDiv = 0.125f; break; // 1/32
        case 4: beatDiv = 1.5f; break; // 1/4D
        case 5: beatDiv = 0.75f; break; // 1/8D
        case 6: beatDiv = 0.375f; break; // 1/16D
        case 7: beatDiv = 2.0f/3.0f; break; // 1/4T
        case 8: beatDiv = 1.0f/3.0f; break; // 1/8T
        case 9: beatDiv = 0.5f/3.0f; break; // 1/16T
    }

    if (playHead != nullptr) {
        if (auto pos = playHead->getPosition()) {
            if (pos->getBpm().hasValue()) {
                arpeggiator.bpm = *pos->getBpm();
            }
        }
    }

    float beatsPerSecond = arpeggiator.bpm / 60.0f;
    float samplesPerBeat = arpeggiator.sampleRate / beatsPerSecond;
    float samplesPerDiv = samplesPerBeat * beatDiv;

    juce::MidiBuffer outMidi;
    int samplePos = 0;
    
    for (const auto meta : midiMessages)
    {
        auto msg = meta.getMessage();
        if (msg.isNoteOn()) {
            arpeggiator.noteOn(msg.getNoteNumber());
        } else if (msg.isNoteOff()) {
            arpeggiator.noteOff(msg.getNoteNumber());
        } else {
            outMidi.addEvent(msg, meta.samplePosition);
        }
    }

    // If arpeggiator is empty, flush all playing notes immediately
    if (arpeggiator.heldNotes.isEmpty()) {
        for (int n : arpeggiator.playingNotes) {
            outMidi.addEvent(juce::MidiMessage::noteOff(1, n, 0.0f), 0);
        }
        arpeggiator.playingNotes.clear();
        arpeggiator.samplesElapsed = 0; // reset for next keypress
        midiMessages.swapWith(outMidi);
        return;
    }

    int numSamples = buffer.getNumSamples();
    for (int i = 0; i < numSamples; ++i)
    {
        bool isEvenStep = (arpeggiator.samplesElapsed / (int)samplesPerDiv) % 2 == 0;
        int currentDivSamples = samplesPerDiv;
        if (!isEvenStep) {
            currentDivSamples += currentDivSamples * swing;
        } else {
            currentDivSamples -= currentDivSamples * swing;
        }

        if (arpeggiator.samplesElapsed >= currentDivSamples) {
            arpeggiator.samplesElapsed = 0;
            
            // Turn off any playing notes
            for (int n : arpeggiator.playingNotes) {
                outMidi.addEvent(juce::MidiMessage::noteOff(1, n, 0.0f), i);
            }
            arpeggiator.playingNotes.clear();

            if (!arpeggiator.heldNotes.isEmpty()) {
                int noteToPlay = -1;
                if (arpMode == 0) { // Repeat
                    for (int n : arpeggiator.heldNotes) {
                        outMidi.addEvent(juce::MidiMessage::noteOn(1, n, 0.8f), i);
                        arpeggiator.playingNotes.add(n);
                    }
                } else if (arpMode == 1) { // Up
                    noteToPlay = arpeggiator.heldNotes[arpeggiator.currentNoteIndex];
                    arpeggiator.currentNoteIndex = (arpeggiator.currentNoteIndex + 1) % arpeggiator.heldNotes.size();
                } else if (arpMode == 2) { // Down
                    noteToPlay = arpeggiator.heldNotes[arpeggiator.heldNotes.size() - 1 - arpeggiator.currentNoteIndex];
                    arpeggiator.currentNoteIndex = (arpeggiator.currentNoteIndex + 1) % arpeggiator.heldNotes.size();
                } else if (arpMode == 3) { // UpDown
                    int n = arpeggiator.heldNotes[arpeggiator.currentNoteIndex];
                    outMidi.addEvent(juce::MidiMessage::noteOn(1, n, 0.8f), i);
                    arpeggiator.playingNotes.add(n);

                    if (arpeggiator.goingUp) {
                        arpeggiator.currentNoteIndex++;
                        if (arpeggiator.currentNoteIndex >= arpeggiator.heldNotes.size() - 1) {
                            arpeggiator.goingUp = false;
                        }
                    } else {
                        arpeggiator.currentNoteIndex--;
                        if (arpeggiator.currentNoteIndex <= 0) {
                            arpeggiator.goingUp = true;
                        }
                    }
                } else if (arpMode == 4) { // As Played
                    if (arpeggiator.currentNoteIndex >= arpeggiator.orderedNotes.size()) {
                        arpeggiator.currentNoteIndex = 0;
                    }
                    int n = arpeggiator.orderedNotes[arpeggiator.currentNoteIndex];
                    outMidi.addEvent(juce::MidiMessage::noteOn(1, n, 0.8f), i);
                    arpeggiator.playingNotes.add(n);
                    
                    arpeggiator.currentNoteIndex++;
                    if (arpeggiator.currentNoteIndex >= arpeggiator.orderedNotes.size()) {
                        arpeggiator.currentNoteIndex = 0;
                    }
                }

                if (noteToPlay != -1) {
                    outMidi.addEvent(juce::MidiMessage::noteOn(1, noteToPlay, 0.8f), i);
                    arpeggiator.playingNotes.add(noteToPlay);
                }
            }
        } else if (arpeggiator.samplesElapsed == (int)(samplesPerDiv * 0.5f)) {
            // Note off for all arpeggiated notes to create short plucks
            for (int n : arpeggiator.playingNotes) {
                outMidi.addEvent(juce::MidiMessage::noteOff(1, n, 0.0f), i);
            }
            arpeggiator.playingNotes.clear();
        }

        arpeggiator.samplesElapsed++;
    }

    midiMessages.swapWith(outMidi);
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
