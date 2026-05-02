#include "../src/PluginProcessor.h"

#include <juce_audio_utils/juce_audio_utils.h>

#include <iostream>
#include <stdexcept>

namespace
{
struct CheckFailure : public std::runtime_error
{
    using std::runtime_error::runtime_error;
};

void require(bool condition, const juce::String& message)
{
    if (! condition)
        throw CheckFailure(message.toStdString());
}

void requireFileText(const juce::File& file, const juce::String& expected)
{
    require(file.existsAsFile(), "Missing materialized asset: " + file.getFullPathName());
    require(file.loadFileAsString().trim() == expected,
            "Unexpected materialized asset content: " + file.getFullPathName());
}
}

int main()
{
    juce::ScopedJuceInitialiser_GUI juceInit;

    try
    {
        MrgeeJsfxAudioProcessor processor;
        const auto root = processor.getJsfxHost().getMaterializedBundleRoot();

        requireFileText(root.getChildFile("Effects").getChildFile("AssetSmoke.jsfx"), "desc:Mrgee Asset Smoke\nauthor:OpenAI Codex\ntags:test utility\n\nslider1:0<0,1,1>Asset Mode {A,B}\n\n@sample\nspl0 = spl0;\nspl1 = spl1;");
        requireFileText(root.getChildFile("Effects").getChildFile("lib").getChildFile("marker.txt"), "asset-smoke-lib");
        requireFileText(root.getChildFile("Data").getChildFile("sample.txt"), "asset-smoke-data");

        std::cout << "jsfx_asset_smoke: OK" << std::endl;
        return 0;
    }
    catch (const CheckFailure& failure)
    {
        std::cerr << "jsfx_asset_smoke: FAIL: " << failure.what() << std::endl;
    }
    catch (const std::exception& failure)
    {
        std::cerr << "jsfx_asset_smoke: ERROR: " << failure.what() << std::endl;
    }

    return 1;
}

