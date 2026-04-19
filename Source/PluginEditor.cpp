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
    legatoAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(*processorRef.apvts.getParameter("legato"), legatoRelay, processorRef.apvts.undoManager);
    glideTimeAttachment = std::make_unique<juce::WebSliderParameterAttachment>(*processorRef.apvts.getParameter("glideTime"), glideTimeRelay, processorRef.apvts.undoManager);

    wubEnabledAttachment    = std::make_unique<juce::WebToggleButtonParameterAttachment>(*processorRef.apvts.getParameter("wubEnabled"),    wubEnabledRelay,    processorRef.apvts.undoManager);
    wubRateAttachment       = std::make_unique<juce::WebSliderParameterAttachment>(*processorRef.apvts.getParameter("wubRate"),       wubRateRelay,       processorRef.apvts.undoManager);
    wubDepthAttachment      = std::make_unique<juce::WebSliderParameterAttachment>(*processorRef.apvts.getParameter("wubDepth"),      wubDepthRelay,      processorRef.apvts.undoManager);
    wubCenterAttachment     = std::make_unique<juce::WebSliderParameterAttachment>(*processorRef.apvts.getParameter("wubCenter"),     wubCenterRelay,     processorRef.apvts.undoManager);
    wubResonanceAttachment  = std::make_unique<juce::WebSliderParameterAttachment>(*processorRef.apvts.getParameter("wubResonance"),  wubResonanceRelay,  processorRef.apvts.undoManager);
    wubFilterTypeAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(*processorRef.apvts.getParameter("wubFilterType"), wubFilterTypeRelay, processorRef.apvts.undoManager);

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
            .withOptionsFrom(legatoRelay)
            .withOptionsFrom(glideTimeRelay)
            .withOptionsFrom(wubEnabledRelay)
            .withOptionsFrom(wubRateRelay)
            .withOptionsFrom(wubDepthRelay)
            .withOptionsFrom(wubCenterRelay)
            .withOptionsFrom(wubResonanceRelay)
            .withOptionsFrom(wubFilterTypeRelay)
    );
    addAndMakeVisible (*webView);

    setSize (1200, 780);

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