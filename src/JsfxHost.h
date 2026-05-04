#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include <vector>

class JsfxHost
{
public:
    struct SliderDescriptor
    {
        int index = 0;
        juce::String paramId;
        juce::String name;
        float defaultValue = 0.0f;
        float minValue = 0.0f;
        float maxValue = 1.0f;
        float step = 0.01f;
        bool isEnum = false;
        bool isVisible = true;
        juce::StringArray enumNames;
    };

    JsfxHost();
    ~JsfxHost();

    bool loadBundledScript();
    void prepare(double sampleRate, int samplesPerBlock, int channels);
    void reset();
    void setSlider(int sliderIndex, float value);
    float getSlider(int sliderIndex) const;
    float getRuntimeSlider(int sliderIndex) const;
    void process(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi);

    const std::vector<SliderDescriptor>& getSliderDescriptors() const noexcept { return sliderDescriptors; }
    const juce::String& getStatusMessage() const noexcept { return statusMessage; }
    bool hasRuntime() const noexcept { return runtimeLoaded; }
    juce::File getMaterializedBundleRoot() const;

    static std::vector<SliderDescriptor> loadBundledSliderDescriptors();

private:
    struct BundleEntry
    {
        juce::String logicalPath;
        const char* data = nullptr;
        int size = 0;
    };

    struct JsfxBundle
    {
        juce::String bundleId;
        juce::String mainScriptName;
        std::vector<BundleEntry> entries;

        const BundleEntry* getMainScript() const noexcept;
    };

    void updateStatus(juce::String message, bool runtimeActive);
    void loadFallbackDescriptors();
    void applySliderDefaults();
    juce::File materializeBundledBundle() const;
    static JsfxBundle getBundledJsfxBundle();
    static juce::String getBundledScriptText();
    static std::vector<SliderDescriptor> parseSliderDescriptors(const juce::String& scriptText);

    std::vector<SliderDescriptor> sliderDescriptors;
    juce::Array<float> sliderValues;
    juce::String statusMessage;
    bool runtimeLoaded = false;

    struct Runtime;
    std::unique_ptr<Runtime> runtime;
};
