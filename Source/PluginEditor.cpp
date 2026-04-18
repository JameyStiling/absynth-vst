#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
constexpr const char* absynthUiDevServerURL = "http://127.0.0.1:5173";
}

//==============================================================================
AbsynthAudioProcessorEditor::AbsynthAudioProcessorEditor (AbsynthAudioProcessor& p)
    : AudioProcessorEditor (p), processorRef (p)
{
    // Initialize parameter attachments
    cutoffAttachment = std::make_unique<juce::WebSliderParameterAttachment>(*processorRef.apvts.getParameter("cutoff"), cutoffRelay, processorRef.apvts.undoManager);
    resonanceAttachment = std::make_unique<juce::WebSliderParameterAttachment>(*processorRef.apvts.getParameter("resonance"), resonanceRelay, processorRef.apvts.undoManager);
    attackAttachment = std::make_unique<juce::WebSliderParameterAttachment>(*processorRef.apvts.getParameter("attack"), attackRelay, processorRef.apvts.undoManager);
    decayAttachment = std::make_unique<juce::WebSliderParameterAttachment>(*processorRef.apvts.getParameter("decay"), decayRelay, processorRef.apvts.undoManager);
    sustainAttachment = std::make_unique<juce::WebSliderParameterAttachment>(*processorRef.apvts.getParameter("sustain"), sustainRelay, processorRef.apvts.undoManager);
    releaseAttachment = std::make_unique<juce::WebSliderParameterAttachment>(*processorRef.apvts.getParameter("release"), releaseRelay, processorRef.apvts.undoManager);
    oscTypeAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(*processorRef.apvts.getParameter("oscType"), oscTypeRelay, processorRef.apvts.undoManager);

    setSize (800, 600);

    webView = std::make_unique<juce::WebBrowserComponent> (
        juce::WebBrowserComponent::Options{}
            .withKeepPageLoadedWhenBrowserIsHidden()
            .withOptionsFrom(cutoffRelay)
            .withOptionsFrom(resonanceRelay)
            .withOptionsFrom(attackRelay)
            .withOptionsFrom(decayRelay)
            .withOptionsFrom(sustainRelay)
            .withOptionsFrom(releaseRelay)
            .withOptionsFrom(oscTypeRelay)
    );
    addAndMakeVisible (*webView);

    juce::MessageManager::callAsync ([this]
                                     {
                                         if (webView != nullptr)
                                             webView->goToURL (absynthUiDevServerURL);
                                     });
}

AbsynthAudioProcessorEditor::~AbsynthAudioProcessorEditor()
{
}

//==============================================================================
void AbsynthAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void AbsynthAudioProcessorEditor::resized()
{
    if (webView != nullptr)
        webView->setBounds (getLocalBounds());
}