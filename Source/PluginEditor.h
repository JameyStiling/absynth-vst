#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>

class AbsynthAudioProcessor;

//==============================================================================
class AbsynthAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit AbsynthAudioProcessorEditor (AbsynthAudioProcessor&);
    ~AbsynthAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    AbsynthAudioProcessor& processorRef;

    std::unique_ptr<juce::WebBrowserComponent> webView;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AbsynthAudioProcessorEditor)
};
