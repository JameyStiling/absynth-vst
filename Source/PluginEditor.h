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
    juce::WebSliderRelay attackRelay { "attack" };
    juce::WebSliderRelay decayRelay { "decay" };
    juce::WebSliderRelay sustainRelay { "sustain" };
    juce::WebSliderRelay releaseRelay { "release" };
    juce::WebSliderRelay cutoffRelay { "cutoff" };
    juce::WebSliderRelay resonanceRelay { "resonance" };
    juce::WebToggleButtonRelay osc1ActiveRelay { "osc1Active" };
    juce::WebComboBoxRelay osc1TypeRelay { "osc1Type" };
    juce::WebSliderRelay osc1LevelRelay { "osc1Level" };
    juce::WebToggleButtonRelay osc2ActiveRelay { "osc2Active" };
    juce::WebComboBoxRelay osc2TypeRelay { "osc2Type" };
    juce::WebSliderRelay osc2LevelRelay { "osc2Level" };
    juce::WebToggleButtonRelay osc3ActiveRelay { "osc3Active" };
    juce::WebComboBoxRelay osc3TypeRelay { "osc3Type" };
    juce::WebSliderRelay osc3LevelRelay { "osc3Level" };
    juce::WebToggleButtonRelay legatoRelay { "legato" };
    juce::WebSliderRelay glideTimeRelay { "glideTime" };

    // Mod
    juce::WebToggleButtonRelay modEnabledRelay { "modEnabled" };
    juce::WebSliderRelay modRateRelay { "modRate" };
    juce::WebSliderRelay modDepthRelay { "modDepth" };
    juce::WebSliderRelay modCenterRelay { "modCenter" };
    juce::WebSliderRelay modResonanceRelay { "modResonance" };
    juce::WebComboBoxRelay modFilterTypeRelay { "modFilterType" };
    juce::WebComboBoxRelay modDepthModeRelay { "modDepthMode" };
    juce::WebComboBoxRelay modPolarityRelay { "modPolarity" };
    juce::WebComboBoxRelay modSlopeRelay { "modSlope" };

    juce::WebToggleButtonRelay arpEnabledRelay { "arpEnabled" };
    juce::WebComboBoxRelay arpRateRelay { "arpRate" };
    juce::WebSliderRelay arpSwingRelay { "arpSwing" };
    juce::WebComboBoxRelay arpModeRelay { "arpMode" };

    std::unique_ptr<juce::WebSliderParameterAttachment> attackAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> decayAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> sustainAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> releaseAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> cutoffAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> resonanceAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> osc1ActiveAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment> osc1TypeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> osc1LevelAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> osc2ActiveAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment> osc2TypeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> osc2LevelAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> osc3ActiveAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment> osc3TypeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> osc3LevelAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> legatoAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> glideTimeAttachment;

    // Mod
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> modEnabledAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> modRateAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> modDepthAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> modCenterAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> modResonanceAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment> modFilterTypeAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment> modDepthModeAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment> modPolarityAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment> modSlopeAttachment;

    std::unique_ptr<juce::WebToggleButtonParameterAttachment> arpEnabledAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment> arpRateAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> arpSwingAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment> arpModeAttachment;

    std::unique_ptr<juce::WebBrowserComponent> webView;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LatticeAudioProcessorEditor)
};
