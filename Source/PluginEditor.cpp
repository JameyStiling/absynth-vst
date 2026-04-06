#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AbsynthAudioProcessorEditor::AbsynthAudioProcessorEditor (AbsynthAudioProcessor& p)
    : AudioProcessorEditor (p), processorRef (p)
{
    juce::ignoreUnused (processorRef);
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (800, 600);

    webView = std::make_unique<juce::WebBrowserComponent>();
    addAndMakeVisible (*webView);
    webView->goToURL ("http://localhost:5173");
}

AbsynthAudioProcessorEditor::~AbsynthAudioProcessorEditor()
{
}

//==============================================================================
void AbsynthAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour (juce::Colours::white);
    g.setFont (15.0f);
    g.drawFittedText ("Hello World!", getLocalBounds(), juce::Justification::centred, 1);
}

void AbsynthAudioProcessorEditor::resized()
{
    if (webView != nullptr)
        webView->setBounds (getLocalBounds());
}