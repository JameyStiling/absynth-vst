#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>

class AbsynthAudioProcessor;

class AbsynthAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit AbsynthAudioProcessorEditor (AbsynthAudioProcessor&);
    ~AbsynthAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    AbsynthAudioProcessor& processorRef;

    juce::WebSliderRelay cutoffRelay { "cutoff" };
    juce::WebSliderRelay resonanceRelay { "resonance" };
    juce::WebSliderRelay attackRelay { "attack" };
    juce::WebSliderRelay decayRelay { "decay" };
    juce::WebSliderRelay sustainRelay { "sustain" };
    juce::WebSliderRelay releaseRelay { "release" };
    juce::WebComboBoxRelay oscTypeRelay { "oscType" };
    juce::WebToggleButtonRelay legatoRelay { "legato" };
    juce::WebSliderRelay glideTimeRelay { "glideTime" };

    std::unique_ptr<juce::WebSliderParameterAttachment> cutoffAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> resonanceAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> attackAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> decayAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> sustainAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> releaseAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment> oscTypeAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> legatoAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> glideTimeAttachment;

    std::unique_ptr<juce::WebBrowserComponent> webView;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AbsynthAudioProcessorEditor)
};
