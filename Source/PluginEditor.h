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
    juce::WebComboBoxRelay midiChannelRelay { "midiChannel" };
    juce::WebComboBoxRelay postFxOrderRelay { "postFxOrder" };

#ifdef LATTICE_HAS_MODULES
    // Nebuli granular cloud reverb
    juce::WebToggleButtonRelay nebuliEnableRelay { "nebuli_enable" };
    juce::WebSliderRelay nebuliMixRelay { "nebuli_mix" };
    juce::WebToggleButtonRelay nebuliFreezeRelay { "nebuli_freeze" };
    juce::WebSliderRelay nebuliGrainCountRelay { "nebuli_grain_count" };
    juce::WebSliderRelay nebuliGrainSizeRelay { "nebuli_grain_size" };
    juce::WebSliderRelay nebuliGrainSizeRandRelay { "nebuli_grain_size_rand" };
    juce::WebSliderRelay nebuliGrainPositionRandRelay { "nebuli_grain_position_rand" };
    juce::WebSliderRelay nebuliSpeedRelay { "nebuli_speed" };
    juce::WebSliderRelay nebuliDetuneRelay { "nebuli_detune" };
    juce::WebComboBoxRelay nebuliFilterTypeRelay { "nebuli_filter_type" };
    juce::WebSliderRelay nebuliFilterCutoffRelay { "nebuli_filter_cutoff" };
    juce::WebSliderRelay nebuliFilterResonanceRelay { "nebuli_filter_resonance" };
    juce::WebSliderRelay nebuliStereoWidthRelay { "nebuli_stereo_width" };
    juce::WebSliderRelay nebuliTailRelay { "nebuli_tail" };

    // Monoizer low-end mono module
    juce::WebToggleButtonRelay monoizerEnableRelay { "monoizer_enable" };
    juce::WebSliderRelay monoizerCutoffRelay { "monoizer_cutoff" };
#endif

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
    std::unique_ptr<juce::WebComboBoxParameterAttachment> midiChannelAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment> postFxOrderAttachment;

#ifdef LATTICE_HAS_MODULES
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> nebuliEnableAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> nebuliMixAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> nebuliFreezeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> nebuliGrainCountAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> nebuliGrainSizeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> nebuliGrainSizeRandAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> nebuliGrainPositionRandAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> nebuliSpeedAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> nebuliDetuneAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment> nebuliFilterTypeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> nebuliFilterCutoffAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> nebuliFilterResonanceAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> nebuliStereoWidthAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> nebuliTailAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> monoizerEnableAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> monoizerCutoffAttachment;
#endif

    std::unique_ptr<juce::WebBrowserComponent> webView;
    bool webViewCreated { false };
    juce::String uiLoadError;

    void ensureWebViewCreated();
    bool shouldUseDevServer() const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LatticeAudioProcessorEditor)
};
