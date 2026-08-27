#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <cstring>

#if JUCE_MAC
 #include <CoreFoundation/CoreFoundation.h>
 #include <dlfcn.h>
 #include <limits.h>
#endif

namespace
{
constexpr const char* kDevServerURL = "http://localhost:5173";

juce::File uiDirFromExecutable (const juce::File& executable)
{
    if (executable.getParentDirectory().getFileName() == "MacOS")
    {
        return executable.getParentDirectory().getParentDirectory()
                           .getChildFile ("Resources")
                           .getChildFile ("ui");
    }

    return executable.getParentDirectory()
                     .getParentDirectory()
                     .getChildFile ("Resources")
                     .getChildFile ("ui");
}

juce::File getBundledUiRoot()
{
    const auto fromExe = uiDirFromExecutable (
        juce::File::getSpecialLocation (juce::File::currentExecutableFile));

    if (fromExe.isDirectory())
        return fromExe;

#if JUCE_MAC
    if (CFBundleRef pluginBundle = CFBundleGetBundleWithIdentifier (CFSTR ("com.sherdaudio.lattice")))
    {
        if (CFURLRef bundleUrl = CFBundleCopyBundleURL (pluginBundle))
        {
            char path[PATH_MAX] = {};
            if (CFURLGetFileSystemRepresentation (bundleUrl, true, reinterpret_cast<UInt8*> (path), PATH_MAX))
            {
                const auto bundleRoot = juce::File (path);
                CFRelease (bundleUrl);

                const auto uiDir = bundleRoot.getChildFile ("Contents")
                                             .getChildFile ("Resources")
                                             .getChildFile ("ui");

                if (uiDir.isDirectory())
                    return uiDir;
            }
            else
            {
                CFRelease (bundleUrl);
            }
        }
    }

    Dl_info info {};
    if (dladdr (reinterpret_cast<const void*> (&getBundledUiRoot), &info) != 0 && info.dli_fname != nullptr)
    {
        const auto fromDladdr = uiDirFromExecutable (juce::File { juce::String::fromUTF8 (info.dli_fname) });

        if (fromDladdr.isDirectory())
            return fromDladdr;
    }
#endif

    const auto fromInstall = juce::File::getSpecialLocation (juce::File::userHomeDirectory)
                                 .getChildFile ("Library/Audio/Plug-Ins/VST3/Lattice.vst3/Contents/Resources/ui");

    if (fromInstall.isDirectory())
        return fromInstall;

    return {};
}

juce::String mimeTypeForExtension (const juce::String& extension)
{
    if (extension == ".html" || extension == ".htm") return "text/html";
    if (extension == ".js" || extension == ".mjs") return "application/javascript";
    if (extension == ".css") return "text/css";
    if (extension == ".svg") return "image/svg+xml";
    if (extension == ".json") return "application/json";
    if (extension == ".png") return "image/png";
    if (extension == ".woff2") return "font/woff2";
    if (extension == ".woff") return "font/woff";
    if (extension == ".ttf") return "font/ttf";
    if (extension == ".ico") return "image/x-icon";
    return "application/octet-stream";
}

juce::String normaliseResourcePath (juce::String path)
{
    if (path.contains ("://"))
        path = juce::URL (path).getSubPath();

    const auto query = path.indexOfChar ('?');
    if (query > 0)
        path = path.substring (0, query);

    if (path.isEmpty() || path == "/")
        return "index.html";

    if (path.startsWithChar ('/'))
        return path.substring (1);

    return path;
}

std::optional<juce::WebBrowserComponent::Resource> loadBundledResource (const juce::String& path)
{
    const auto uiRoot = getBundledUiRoot();
    if (! uiRoot.isDirectory())
        return std::nullopt;

    const auto file = uiRoot.getChildFile (normaliseResourcePath (path));
    if (! file.existsAsFile())
        return std::nullopt;

    juce::MemoryBlock data;
    if (! file.loadFileAsData (data))
        return std::nullopt;

    juce::WebBrowserComponent::Resource resource;
    resource.mimeType = mimeTypeForExtension (file.getFileExtension());
    resource.data.resize (data.getSize());
    std::memcpy (resource.data.data(), data.getData(), data.getSize());
    return resource;
}
}

bool LatticeAudioProcessorEditor::shouldUseDevServer() const
{
    return processorRef.wrapperType == juce::AudioProcessor::wrapperType_Standalone;
}

//==============================================================================
LatticeAudioProcessorEditor::LatticeAudioProcessorEditor (LatticeAudioProcessor& p)
    : AudioProcessorEditor (p), processorRef (p)
{
    attackAttachment = std::make_unique<juce::WebSliderParameterAttachment>(*processorRef.apvts.getParameter("attack"), attackRelay, processorRef.apvts.undoManager);
    decayAttachment = std::make_unique<juce::WebSliderParameterAttachment>(*processorRef.apvts.getParameter("decay"), decayRelay, processorRef.apvts.undoManager);
    sustainAttachment = std::make_unique<juce::WebSliderParameterAttachment>(*processorRef.apvts.getParameter("sustain"), sustainRelay, processorRef.apvts.undoManager);
    releaseAttachment = std::make_unique<juce::WebSliderParameterAttachment>(*processorRef.apvts.getParameter("release"), releaseRelay, processorRef.apvts.undoManager);
    cutoffAttachment = std::make_unique<juce::WebSliderParameterAttachment>(*processorRef.apvts.getParameter("cutoff"), cutoffRelay, processorRef.apvts.undoManager);
    resonanceAttachment = std::make_unique<juce::WebSliderParameterAttachment>(*processorRef.apvts.getParameter("resonance"), resonanceRelay, processorRef.apvts.undoManager);
    osc1ActiveAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(*processorRef.apvts.getParameter("osc1Active"), osc1ActiveRelay, processorRef.apvts.undoManager);
    osc1TypeAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(*processorRef.apvts.getParameter("osc1Type"), osc1TypeRelay, processorRef.apvts.undoManager);
    osc1LevelAttachment = std::make_unique<juce::WebSliderParameterAttachment>(*processorRef.apvts.getParameter("osc1Level"), osc1LevelRelay, processorRef.apvts.undoManager);
    osc2ActiveAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(*processorRef.apvts.getParameter("osc2Active"), osc2ActiveRelay, processorRef.apvts.undoManager);
    osc2TypeAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(*processorRef.apvts.getParameter("osc2Type"), osc2TypeRelay, processorRef.apvts.undoManager);
    osc2LevelAttachment = std::make_unique<juce::WebSliderParameterAttachment>(*processorRef.apvts.getParameter("osc2Level"), osc2LevelRelay, processorRef.apvts.undoManager);
    osc3ActiveAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(*processorRef.apvts.getParameter("osc3Active"), osc3ActiveRelay, processorRef.apvts.undoManager);
    osc3TypeAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(*processorRef.apvts.getParameter("osc3Type"), osc3TypeRelay, processorRef.apvts.undoManager);
    osc3LevelAttachment = std::make_unique<juce::WebSliderParameterAttachment>(*processorRef.apvts.getParameter("osc3Level"), osc3LevelRelay, processorRef.apvts.undoManager);
    legatoAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(*processorRef.apvts.getParameter("legato"), legatoRelay, processorRef.apvts.undoManager);
    glideTimeAttachment = std::make_unique<juce::WebSliderParameterAttachment>(*processorRef.apvts.getParameter("glideTime"), glideTimeRelay, processorRef.apvts.undoManager);

    modEnabledAttachment    = std::make_unique<juce::WebToggleButtonParameterAttachment>(*processorRef.apvts.getParameter("modEnabled"),    modEnabledRelay,    processorRef.apvts.undoManager);
    modRateAttachment       = std::make_unique<juce::WebSliderParameterAttachment>(*processorRef.apvts.getParameter("modRate"),       modRateRelay,       processorRef.apvts.undoManager);
    modDepthAttachment      = std::make_unique<juce::WebSliderParameterAttachment>(*processorRef.apvts.getParameter("modDepth"),      modDepthRelay,      processorRef.apvts.undoManager);
    modCenterAttachment     = std::make_unique<juce::WebSliderParameterAttachment>(*processorRef.apvts.getParameter("modCenter"),     modCenterRelay,     processorRef.apvts.undoManager);
    modResonanceAttachment  = std::make_unique<juce::WebSliderParameterAttachment>(*processorRef.apvts.getParameter("modResonance"),  modResonanceRelay,  processorRef.apvts.undoManager);
    modFilterTypeAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(*processorRef.apvts.getParameter("modFilterType"), modFilterTypeRelay, processorRef.apvts.undoManager);
    modDepthModeAttachment  = std::make_unique<juce::WebComboBoxParameterAttachment>(*processorRef.apvts.getParameter("modDepthMode"), modDepthModeRelay, processorRef.apvts.undoManager);
    modPolarityAttachment   = std::make_unique<juce::WebComboBoxParameterAttachment>(*processorRef.apvts.getParameter("modPolarity"),  modPolarityRelay,  processorRef.apvts.undoManager);
    modSlopeAttachment      = std::make_unique<juce::WebComboBoxParameterAttachment>(*processorRef.apvts.getParameter("modSlope"),     modSlopeRelay,     processorRef.apvts.undoManager);

    arpEnabledAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(*processorRef.apvts.getParameter("arpEnabled"), arpEnabledRelay, processorRef.apvts.undoManager);
    arpRateAttachment    = std::make_unique<juce::WebComboBoxParameterAttachment>(*processorRef.apvts.getParameter("arpRate"),    arpRateRelay,    processorRef.apvts.undoManager);
    arpSwingAttachment   = std::make_unique<juce::WebSliderParameterAttachment>(*processorRef.apvts.getParameter("arpSwing"),   arpSwingRelay,   processorRef.apvts.undoManager);
    arpModeAttachment    = std::make_unique<juce::WebComboBoxParameterAttachment>(*processorRef.apvts.getParameter("arpMode"),    arpModeRelay,    processorRef.apvts.undoManager);
    midiChannelAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(*processorRef.apvts.getParameter("midiChannel"), midiChannelRelay, processorRef.apvts.undoManager);
    postFxOrderAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(*processorRef.apvts.getParameter("postFxOrder"), postFxOrderRelay, processorRef.apvts.undoManager);

#ifdef LATTICE_HAS_MODULES
    nebuliEnableAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(*processorRef.apvts.getParameter("nebuli_enable"), nebuliEnableRelay, processorRef.apvts.undoManager);
    nebuliMixAttachment = std::make_unique<juce::WebSliderParameterAttachment>(*processorRef.apvts.getParameter("nebuli_mix"), nebuliMixRelay, processorRef.apvts.undoManager);
    nebuliFreezeAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(*processorRef.apvts.getParameter("nebuli_freeze"), nebuliFreezeRelay, processorRef.apvts.undoManager);
    nebuliGrainCountAttachment = std::make_unique<juce::WebSliderParameterAttachment>(*processorRef.apvts.getParameter("nebuli_grain_count"), nebuliGrainCountRelay, processorRef.apvts.undoManager);
    nebuliGrainSizeAttachment = std::make_unique<juce::WebSliderParameterAttachment>(*processorRef.apvts.getParameter("nebuli_grain_size"), nebuliGrainSizeRelay, processorRef.apvts.undoManager);
    nebuliGrainSizeRandAttachment = std::make_unique<juce::WebSliderParameterAttachment>(*processorRef.apvts.getParameter("nebuli_grain_size_rand"), nebuliGrainSizeRandRelay, processorRef.apvts.undoManager);
    nebuliGrainPositionRandAttachment = std::make_unique<juce::WebSliderParameterAttachment>(*processorRef.apvts.getParameter("nebuli_grain_position_rand"), nebuliGrainPositionRandRelay, processorRef.apvts.undoManager);
    nebuliSpeedAttachment = std::make_unique<juce::WebSliderParameterAttachment>(*processorRef.apvts.getParameter("nebuli_speed"), nebuliSpeedRelay, processorRef.apvts.undoManager);
    nebuliDetuneAttachment = std::make_unique<juce::WebSliderParameterAttachment>(*processorRef.apvts.getParameter("nebuli_detune"), nebuliDetuneRelay, processorRef.apvts.undoManager);
    nebuliFilterTypeAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(*processorRef.apvts.getParameter("nebuli_filter_type"), nebuliFilterTypeRelay, processorRef.apvts.undoManager);
    nebuliFilterCutoffAttachment = std::make_unique<juce::WebSliderParameterAttachment>(*processorRef.apvts.getParameter("nebuli_filter_cutoff"), nebuliFilterCutoffRelay, processorRef.apvts.undoManager);
    nebuliFilterResonanceAttachment = std::make_unique<juce::WebSliderParameterAttachment>(*processorRef.apvts.getParameter("nebuli_filter_resonance"), nebuliFilterResonanceRelay, processorRef.apvts.undoManager);
    nebuliStereoWidthAttachment = std::make_unique<juce::WebSliderParameterAttachment>(*processorRef.apvts.getParameter("nebuli_stereo_width"), nebuliStereoWidthRelay, processorRef.apvts.undoManager);
    nebuliTailAttachment = std::make_unique<juce::WebSliderParameterAttachment>(*processorRef.apvts.getParameter("nebuli_tail"), nebuliTailRelay, processorRef.apvts.undoManager);
    monoizerEnableAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(*processorRef.apvts.getParameter("monoizer_enable"), monoizerEnableRelay, processorRef.apvts.undoManager);
    monoizerCutoffAttachment = std::make_unique<juce::WebSliderParameterAttachment>(*processorRef.apvts.getParameter("monoizer_cutoff"), monoizerCutoffRelay, processorRef.apvts.undoManager);
#endif

    setResizable (true, true);
    setResizeLimits (800, 500, 2000, 1600);
    setSize (1200, 1000);
}

void LatticeAudioProcessorEditor::ensureWebViewCreated()
{
    if (webViewCreated)
        return;

    webViewCreated = true;

    if (! shouldUseDevServer())
    {
        const auto uiRoot = getBundledUiRoot();
        const auto index = uiRoot.getChildFile ("index.html");

        if (! index.existsAsFile())
        {
            uiLoadError = "Lattice UI bundle not found.\n\n"
                          "Executable:\n"
                          + juce::File::getSpecialLocation (juce::File::currentExecutableFile).getFullPathName()
                          + "\n\nExpected UI:\n"
                          + index.getFullPathName()
                          + "\n\nRun: npm run build:vst3";
            repaint();
            return;
        }
    }

    auto options = juce::WebBrowserComponent::Options{}
                       .withKeepPageLoadedWhenBrowserIsHidden()
                       .withAppleWkWebViewOptions (
                           juce::WebBrowserComponent::Options::AppleWkWebView{}.withAllowAccessToEnclosingDirectory (true))
                       .withNativeIntegrationEnabled()
                       .withNativeFunction ("sendMidiNote", [this] (const juce::Array<juce::var>& args, juce::WebBrowserComponent::NativeFunctionCompletion completion)
                       {
                           if (args.size() == 3)
                               processorRef.handleWebMidiEvent ((int) args[0], (int) args[1], (bool) args[2]);

                           completion (juce::var());
                       })
            .withNativeFunction ("sendModDrawState", [this] (const juce::Array<juce::var>& args, juce::WebBrowserComponent::NativeFunctionCompletion completion)
            {
                if (args.size() == 2)
                {
                    processorRef.isModDrawMode.store ((bool) args[0]);
                    processorRef.modDrawValue.store ((float) args[1]);
                }

                completion (juce::var());
            })
            .withNativeFunction ("getActiveMidiNotes", [this] (const juce::Array<juce::var>&, juce::WebBrowserComponent::NativeFunctionCompletion completion)
            {
                completion (processorRef.getActiveMidiNotesForUi());
            })
                       .withResourceProvider ([] (const juce::String& path)
                       {
                           return loadBundledResource (path);
                       });

    if (shouldUseDevServer())
        options = options.withResourceProvider ([] (const juce::String& path)
        {
            return loadBundledResource (path);
        }, kDevServerURL);

    webView = std::make_unique<juce::WebBrowserComponent> (
        options.withOptionsFrom (attackRelay)
               .withOptionsFrom (decayRelay)
               .withOptionsFrom (sustainRelay)
               .withOptionsFrom (releaseRelay)
               .withOptionsFrom (cutoffRelay)
               .withOptionsFrom (resonanceRelay)
               .withOptionsFrom (osc1ActiveRelay)
               .withOptionsFrom (osc1TypeRelay)
               .withOptionsFrom (osc1LevelRelay)
               .withOptionsFrom (osc2ActiveRelay)
               .withOptionsFrom (osc2TypeRelay)
               .withOptionsFrom (osc2LevelRelay)
               .withOptionsFrom (osc3ActiveRelay)
               .withOptionsFrom (osc3TypeRelay)
               .withOptionsFrom (osc3LevelRelay)
               .withOptionsFrom (legatoRelay)
               .withOptionsFrom (glideTimeRelay)
               .withOptionsFrom (modEnabledRelay)
               .withOptionsFrom (modRateRelay)
               .withOptionsFrom (modDepthRelay)
               .withOptionsFrom (modCenterRelay)
               .withOptionsFrom (modResonanceRelay)
               .withOptionsFrom (modFilterTypeRelay)
               .withOptionsFrom (modDepthModeRelay)
               .withOptionsFrom (modPolarityRelay)
               .withOptionsFrom (modSlopeRelay)
               .withOptionsFrom (arpEnabledRelay)
               .withOptionsFrom (arpRateRelay)
               .withOptionsFrom (arpSwingRelay)
               .withOptionsFrom (arpModeRelay)
               .withOptionsFrom (midiChannelRelay)
               .withOptionsFrom (postFxOrderRelay)
#ifdef LATTICE_HAS_MODULES
               .withOptionsFrom (nebuliEnableRelay)
               .withOptionsFrom (nebuliMixRelay)
               .withOptionsFrom (nebuliFreezeRelay)
               .withOptionsFrom (nebuliGrainCountRelay)
               .withOptionsFrom (nebuliGrainSizeRelay)
               .withOptionsFrom (nebuliGrainSizeRandRelay)
               .withOptionsFrom (nebuliGrainPositionRandRelay)
               .withOptionsFrom (nebuliSpeedRelay)
               .withOptionsFrom (nebuliDetuneRelay)
               .withOptionsFrom (nebuliFilterTypeRelay)
               .withOptionsFrom (nebuliFilterCutoffRelay)
               .withOptionsFrom (nebuliFilterResonanceRelay)
               .withOptionsFrom (nebuliStereoWidthRelay)
               .withOptionsFrom (nebuliTailRelay)
               .withOptionsFrom (monoizerEnableRelay)
               .withOptionsFrom (monoizerCutoffRelay)
#endif
               );

    addAndMakeVisible (*webView);

    juce::MessageManager::callAsync ([this]
    {
        if (webView == nullptr)
            return;

        if (shouldUseDevServer())
        {
            webView->goToURL (kDevServerURL);
            return;
        }

        webView->goToURL (juce::WebBrowserComponent::getResourceProviderRoot());
    });
}

LatticeAudioProcessorEditor::~LatticeAudioProcessorEditor()
{
}

void LatticeAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff02050a));

    if (uiLoadError.isNotEmpty())
    {
        g.setColour (juce::Colours::white);
        g.setFont (14.0f);
        g.drawMultiLineText (uiLoadError, 12, 24, getWidth() - 24);
    }
}

void LatticeAudioProcessorEditor::resized()
{
    ensureWebViewCreated();

    if (webView != nullptr)
        webView->setBounds (getLocalBounds());
}
