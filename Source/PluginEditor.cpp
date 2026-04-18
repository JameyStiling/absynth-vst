#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
constexpr const char* absynthUiDevServerURL = "http://localhost:5173";
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

    webView = std::make_unique<juce::WebBrowserComponent> (
        juce::WebBrowserComponent::Options{}
            .withKeepPageLoadedWhenBrowserIsHidden()
            .withNativeIntegrationEnabled()
            .withNativeFunction("sendMidiNote", [this] (const juce::Array<juce::var>& args, juce::WebBrowserComponent::NativeFunctionCompletion completion)
            {
                if (args.size() == 3)
                {
                    int note = args[0];
                    int vel = args[1];
                    bool isNoteOn = args[2];
                    processorRef.handleWebMidiEvent (note, vel, isNoteOn);
                }
                completion (juce::var());
            })
            .withResourceProvider ([] (const auto&) { return std::nullopt; }, absynthUiDevServerURL)
            .withOptionsFrom(cutoffRelay)
            .withOptionsFrom(resonanceRelay)
            .withOptionsFrom(attackRelay)
            .withOptionsFrom(decayRelay)
            .withOptionsFrom(sustainRelay)
            .withOptionsFrom(releaseRelay)
            .withOptionsFrom(oscTypeRelay)
    );
    addAndMakeVisible (*webView);

    setSize (800, 600);

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