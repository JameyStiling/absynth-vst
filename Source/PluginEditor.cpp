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

juce::File getBundledUiRoot()
{
    auto resourcesUiFromBundleRoot = [] (const juce::File& bundleRoot) -> juce::File
    {
        const auto direct = bundleRoot.getChildFile ("Resources").getChildFile ("ui");
        if (direct.isDirectory())
            return direct;

        return bundleRoot.getChildFile ("Contents")
                         .getChildFile ("Resources")
                         .getChildFile ("ui");
    };

#if JUCE_MAC
    if (CFBundleRef pluginBundle = CFBundleGetBundleWithIdentifier (CFSTR ("com.sherdaudio.lattice")))
    {
        if (CFURLRef bundleUrl = CFBundleCopyBundleURL (pluginBundle))
        {
            char path[PATH_MAX] = {};
            if (CFURLGetFileSystemRepresentation (bundleUrl, true, reinterpret_cast<UInt8*> (path), PATH_MAX))
            {
                const auto uiDir = resourcesUiFromBundleRoot (juce::File (path));
                CFRelease (bundleUrl);

                if (uiDir.isDirectory())
                    return uiDir;
            }
            else
            {
                CFRelease (bundleUrl);
            }
        }
    }

    // CFBundleGetMainBundle() is the host in a DAW — use dladdr() on this binary instead.
    {
        Dl_info info {};
        if (dladdr (reinterpret_cast<const void*> (&getBundledUiRoot), &info) != 0 && info.dli_fname != nullptr)
        {
            auto binary = juce::File { juce::String::fromUTF8 (info.dli_fname) };

            if (binary.getParentDirectory().getFileName() == "MacOS")
            {
                const auto uiDir = binary.getParentDirectory().getParentDirectory()
                                      .getChildFile ("Resources")
                                      .getChildFile ("ui");

                if (uiDir.isDirectory())
                    return uiDir;
            }

            auto dir = binary;

            for (int depth = 0; depth < 12 && dir.exists(); ++depth)
            {
                if (dir.getFileExtension() == ".vst3" || dir.getFileExtension() == ".app")
                {
                    const auto uiDir = resourcesUiFromBundleRoot (dir);
                    if (uiDir.isDirectory())
                        return uiDir;
                }

                dir = dir.getParentDirectory();
            }
        }
    }
#endif

    auto dir = juce::File::getSpecialLocation (juce::File::currentExecutableFile);

    for (int depth = 0; depth < 8 && dir.exists(); ++depth)
    {
        const auto uiDir = resourcesUiFromBundleRoot (dir);

        if (uiDir.isDirectory())
            return uiDir;

        if (dir.getFileExtension() == ".vst3")
        {
            const auto nested = resourcesUiFromBundleRoot (dir);
            if (nested.isDirectory())
                return nested;
        }

        dir = dir.getParentDirectory();
    }

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
    return "application/octet-stream";
}

static juce::String normaliseResourcePath (juce::String path)
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

    auto relativePath = normaliseResourcePath (path);
    auto file = uiRoot.getChildFile (relativePath);

    if (! file.existsAsFile())
    {
        const auto fallback = uiRoot.getChildFile ("index.html");
        if (relativePath == "index.html" && fallback.existsAsFile())
            file = fallback;
        else
            return std::nullopt;
    }

    juce::MemoryBlock data;
    if (! file.loadFileAsData (data))
        return std::nullopt;

    juce::WebBrowserComponent::Resource resource;
    resource.mimeType = mimeTypeForExtension (file.getFileExtension());
    resource.data.resize (data.getSize());
    std::memcpy (resource.data.data(), data.getData(), data.getSize());
    return resource;
}

juce::String getUiLoadURL()
{
#if defined(JucePlugin_Build_Standalone) && JucePlugin_Build_Standalone
    return kDevServerURL;
#else
    const auto index = getBundledUiRoot().getChildFile ("index.html");

    if (index.existsAsFile())
        return juce::URL (index).toString (true);

    return juce::WebBrowserComponent::getResourceProviderRoot();
#endif
}
}

//==============================================================================
LatticeAudioProcessorEditor::LatticeAudioProcessorEditor (LatticeAudioProcessor& p)
    : AudioProcessorEditor (p), processorRef (p)
{
    // Initialize parameter attachments
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

    setResizable (true, true);
    setResizeLimits (800, 500, 2000, 1600);
    setSize (1200, 1000);
}

void LatticeAudioProcessorEditor::ensureWebViewCreated()
{
    if (webViewCreated)
        return;

    webViewCreated = true;

    webView = std::make_unique<juce::WebBrowserComponent> (
        juce::WebBrowserComponent::Options{}
            .withKeepPageLoadedWhenBrowserIsHidden()
            .withAppleWkWebViewOptions (
                juce::WebBrowserComponent::Options::AppleWkWebView{}.withAllowAccessToEnclosingDirectory (true))
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
            .withNativeFunction("sendModDrawState", [this] (const juce::Array<juce::var>& args, juce::WebBrowserComponent::NativeFunctionCompletion completion)
            {
                if (args.size() == 2)
                {
                    bool isDraw = args[0];
                    float val = (float)args[1];
                    processorRef.isModDrawMode.store (isDraw);
                    processorRef.modDrawValue.store (val);
                }
                completion (juce::var());
            })
            .withResourceProvider ([] (const juce::String& path)
            {
                return loadBundledResource (path);
            }
#if defined(JucePlugin_Build_Standalone) && JucePlugin_Build_Standalone
            , kDevServerURL
#endif
            )
            .withOptionsFrom(attackRelay)
            .withOptionsFrom(decayRelay)
            .withOptionsFrom(sustainRelay)
            .withOptionsFrom(releaseRelay)
            .withOptionsFrom(cutoffRelay)
            .withOptionsFrom(resonanceRelay)
            .withOptionsFrom(osc1ActiveRelay)
            .withOptionsFrom(osc1TypeRelay)
            .withOptionsFrom(osc1LevelRelay)
            .withOptionsFrom(osc2ActiveRelay)
            .withOptionsFrom(osc2TypeRelay)
            .withOptionsFrom(osc2LevelRelay)
            .withOptionsFrom(osc3ActiveRelay)
            .withOptionsFrom(osc3TypeRelay)
            .withOptionsFrom(osc3LevelRelay)
            .withOptionsFrom(legatoRelay)
            .withOptionsFrom(glideTimeRelay)
            .withOptionsFrom(modEnabledRelay)
            .withOptionsFrom(modRateRelay)
            .withOptionsFrom(modDepthRelay)
            .withOptionsFrom(modCenterRelay)
            .withOptionsFrom(modResonanceRelay)
            .withOptionsFrom(modFilterTypeRelay)
            .withOptionsFrom(modDepthModeRelay)
            .withOptionsFrom(modPolarityRelay)
            .withOptionsFrom(modSlopeRelay)
            .withOptionsFrom(arpEnabledRelay)
            .withOptionsFrom(arpRateRelay)
            .withOptionsFrom(arpSwingRelay)
            .withOptionsFrom(arpModeRelay)
    );
    addAndMakeVisible (*webView);

    juce::MessageManager::callAsync ([this]
    {
        if (webView != nullptr)
            webView->goToURL (getUiLoadURL());
    });
}

LatticeAudioProcessorEditor::~LatticeAudioProcessorEditor()
{
}

//==============================================================================
void LatticeAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff02050a));

#if ! (defined(JucePlugin_Build_Standalone) && JucePlugin_Build_Standalone)
    const auto uiRoot = getBundledUiRoot();
    if (! uiRoot.getChildFile ("index.html").existsAsFile())
    {
        g.setColour (juce::Colours::white);
        g.setFont (14.0f);
        g.drawMultiLineText ("Lattice UI bundle missing.\n"
                             "Run: npm run build:vst3\n\n"
                             "Expected:\n" + uiRoot.getFullPathName(),
                             12, 24, getWidth() - 24);
    }
#endif
}

void LatticeAudioProcessorEditor::resized()
{
    ensureWebViewCreated();

    if (webView != nullptr)
        webView->setBounds (getLocalBounds());
}