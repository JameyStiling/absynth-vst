#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>

class LatticeAudioProcessor;

class LatticeAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit LatticeAudioProcessorEditor (LatticeAudioProcessor&);
    ~LatticeAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    LatticeAudioProcessor& processorRef;

    juce::WebSliderRelay cutoffRelay { "cutoff" };
    juce::WebSliderRelay resonanceRelay { "resonance" };
    juce::WebSliderRelay attackRelay { "attack" };
    juce::WebSliderRelay decayRelay { "decay" };
    juce::WebSliderRelay sustainRelay { "sustain" };
    juce::WebSliderRelay releaseRelay { "release" };
    juce::WebComboBoxRelay oscTypeRelay { "oscType" };
    juce::WebToggleButtonRelay legatoRelay { "legato" };
    juce::WebSliderRelay glideTimeRelay { "glideTime" };

    // Wub
    juce::WebToggleButtonRelay wubEnabledRelay { "wubEnabled" };
    juce::WebSliderRelay wubRateRelay { "wubRate" };
    juce::WebSliderRelay wubDepthRelay { "wubDepth" };
    juce::WebSliderRelay wubCenterRelay { "wubCenter" };
    juce::WebSliderRelay wubResonanceRelay { "wubResonance" };
    juce::WebComboBoxRelay wubFilterTypeRelay { "wubFilterType" };

    std::unique_ptr<juce::WebSliderParameterAttachment> cutoffAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> resonanceAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> attackAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> decayAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> sustainAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> releaseAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment> oscTypeAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> legatoAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> glideTimeAttachment;

    // Wub
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> wubEnabledAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> wubRateAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> wubDepthAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> wubCenterAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> wubResonanceAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment> wubFilterTypeAttachment;

    std::unique_ptr<juce::WebBrowserComponent> webView;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LatticeAudioProcessorEditor)
};
