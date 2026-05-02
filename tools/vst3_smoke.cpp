#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>

#include <cmath>
#include <iostream>
#include <stdexcept>

namespace
{
constexpr double kSampleRate = 48000.0;
constexpr int kBlockSize = 256;
constexpr int kRenderFrames = 48000;
constexpr int kAnalysisSkipFrames = 4096;

struct CheckFailure : public std::runtime_error
{
    using std::runtime_error::runtime_error;
};

const juce::StringArray kParameterNames { "Mode", "Cutoff", "Q", "Slope" };

void require(bool condition, const juce::String& message)
{
    if (! condition)
        throw CheckFailure(message.toStdString());
}

juce::AudioProcessorParameter* findParameterByName(juce::AudioProcessor& processor, const juce::String& name)
{
    for (auto* parameter : processor.getParameters())
        if (parameter->getName(128) == name)
            return parameter;

    return nullptr;
}

juce::String normalisedTextForValue(const juce::String& name, float rawValue)
{
    if (name == "Mode")
        return rawValue >= 0.5f ? "Highpass" : "Lowpass";

    if (name == "Slope")
    {
        const auto index = juce::jlimit(0, 2, juce::roundToInt(rawValue));
        return juce::StringArray { "12 dB", "24 dB", "48 dB" }[index];
    }

    if (name == "Cutoff")
        return juce::String(rawValue, 0);

    if (name == "Q")
        return juce::String(rawValue, 3);

    return juce::String(rawValue);
}

float extractLeadingFloat(const juce::String& text)
{
    juce::String filtered;

    for (auto character : text)
    {
        if (juce::CharacterFunctions::isDigit(character) || character == '.' || character == '-')
            filtered += juce::String::charToString(character);
        else if (! filtered.isEmpty())
            break;
    }

    return filtered.getFloatValue();
}

void setParameterValue(juce::AudioProcessor& processor, const juce::String& name, float rawValue)
{
    auto* parameter = findParameterByName(processor, name);
    require(parameter != nullptr, "Missing parameter: " + name);
    parameter->setValueNotifyingHost(parameter->getValueForText(normalisedTextForValue(name, rawValue)));
}

juce::String getParameterValueText(juce::AudioProcessor& processor, const juce::String& name)
{
    auto* parameter = findParameterByName(processor, name);
    require(parameter != nullptr, "Missing parameter: " + name);
    return parameter->getCurrentValueAsText();
}

juce::AudioBuffer<float> makeInputBuffer(int numFrames)
{
    juce::AudioBuffer<float> buffer(2, numFrames);

    for (int sample = 0; sample < numFrames; ++sample)
    {
        const auto t = static_cast<double>(sample) / kSampleRate;
        const auto value = static_cast<float>(0.55 * std::sin(juce::MathConstants<double>::twoPi * 200.0 * t)
                                             + 0.25 * std::sin(juce::MathConstants<double>::twoPi * 5000.0 * t));
        buffer.setSample(0, sample, value);
        buffer.setSample(1, sample, value);
    }

    return buffer;
}

double analyseFrequencyMagnitude(const juce::AudioBuffer<float>& buffer, double frequencyHz)
{
    double real = 0.0;
    double imag = 0.0;
    const auto framesToInspect = buffer.getNumSamples() - kAnalysisSkipFrames;
    require(framesToInspect > 0, "Render too short for analysis");

    for (int sample = kAnalysisSkipFrames; sample < buffer.getNumSamples(); ++sample)
    {
        const auto t = static_cast<double>(sample) / kSampleRate;
        const auto phase = juce::MathConstants<double>::twoPi * frequencyHz * t;
        const auto mono = 0.5 * (buffer.getSample(0, sample) + buffer.getSample(1, sample));
        real += mono * std::cos(phase);
        imag -= mono * std::sin(phase);
    }

    return std::sqrt(real * real + imag * imag) / static_cast<double>(framesToInspect);
}

std::unique_ptr<juce::AudioPluginInstance> createHostedInstance(juce::VST3PluginFormat& format,
                                                                const juce::PluginDescription& description)
{
    juce::String error;
    auto instance = format.createInstanceFromDescription(description, kSampleRate, kBlockSize, error);
    require(instance != nullptr, "Failed to instantiate hosted VST3: " + error);
    instance->prepareToPlay(kSampleRate, kBlockSize);
    return instance;
}

juce::AudioBuffer<float> renderHostedInstance(juce::AudioPluginInstance& instance, float mode, float cutoff, float q, float slope)
{
    setParameterValue(instance, "Mode", mode);
    setParameterValue(instance, "Cutoff", cutoff);
    setParameterValue(instance, "Q", q);
    setParameterValue(instance, "Slope", slope);

    auto rendered = makeInputBuffer(kRenderFrames);
    juce::MidiBuffer midi;

    for (int offset = 0; offset < kRenderFrames; offset += kBlockSize)
    {
        const auto blockFrames = juce::jmin(kBlockSize, kRenderFrames - offset);
        juce::AudioBuffer<float> block(rendered.getArrayOfWritePointers(),
                                       rendered.getNumChannels(),
                                       offset,
                                       blockFrames);
        instance.processBlock(block, midi);
    }

    return rendered;
}

void verifyHostedVst3(const juce::File& pluginFile)
{
    require(pluginFile.exists(), "VST3 artifact not found: " + pluginFile.getFullPathName());

    juce::VST3PluginFormat format;
    juce::OwnedArray<juce::PluginDescription> descriptions;
    format.findAllTypesForFile(descriptions, pluginFile.getFullPathName());
    require(descriptions.size() == 1, "Expected exactly one VST3 type in artifact");

    auto instance = createHostedInstance(format, *descriptions[0]);
    for (const auto& name : kParameterNames)
        require(findParameterByName(*instance, name) != nullptr, "Hosted VST3 missing parameter: " + name);

    auto lowpass12 = renderHostedInstance(*instance, 0.0f, 1000.0f, 0.707f, 0.0f);
    const auto lowpass12Low = analyseFrequencyMagnitude(lowpass12, 200.0);
    const auto lowpass12High = analyseFrequencyMagnitude(lowpass12, 5000.0);
    require(lowpass12Low > lowpass12High * 3.0, "Hosted VST3 lowpass render check failed");

    instance = createHostedInstance(format, *descriptions[0]);
    auto highpass12 = renderHostedInstance(*instance, 1.0f, 1000.0f, 0.707f, 0.0f);
    const auto highpass12Low = analyseFrequencyMagnitude(highpass12, 200.0);
    const auto highpass12High = analyseFrequencyMagnitude(highpass12, 5000.0);
    require(highpass12High > highpass12Low * 3.0, "Hosted VST3 highpass render check failed");

    instance = createHostedInstance(format, *descriptions[0]);
    auto lowpass48 = renderHostedInstance(*instance, 0.0f, 1000.0f, 0.707f, 2.0f);
    const auto lowpass48High = analyseFrequencyMagnitude(lowpass48, 5000.0);
    require(lowpass48High < lowpass12High * 0.75, "Hosted VST3 slope render check failed");

    instance = createHostedInstance(format, *descriptions[0]);
    setParameterValue(*instance, "Mode", 1.0f);
    setParameterValue(*instance, "Cutoff", 1500.0f);
    setParameterValue(*instance, "Q", 1.4f);
    setParameterValue(*instance, "Slope", 2.0f);

    juce::MemoryBlock state;
    instance->getStateInformation(state);

    auto restored = createHostedInstance(format, *descriptions[0]);
    restored->setStateInformation(state.getData(), static_cast<int>(state.getSize()));

    require(getParameterValueText(*restored, "Mode").containsIgnoreCase("Highpass"), "Hosted Mode state round-trip failed");
    require(std::abs(extractLeadingFloat(getParameterValueText(*restored, "Cutoff")) - 1500.0f) < 10.0f, "Hosted Cutoff state round-trip failed");
    require(std::abs(extractLeadingFloat(getParameterValueText(*restored, "Q")) - 1.4f) < 0.05f, "Hosted Q state round-trip failed");
    require(getParameterValueText(*restored, "Slope").containsIgnoreCase("48"), "Hosted Slope state round-trip failed");
}
}

int main(int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI juceInit;

    try
    {
        require(argc >= 2, "Usage: mrgee_vst3_smoke <path-to-vst3>");
        verifyHostedVst3(juce::File(argv[1]));
        std::cout << "mrgee_vst3_smoke: OK" << std::endl;
        return 0;
    }
    catch (const CheckFailure& failure)
    {
        std::cerr << "mrgee_vst3_smoke: FAIL: " << failure.what() << std::endl;
    }
    catch (const std::exception& failure)
    {
        std::cerr << "mrgee_vst3_smoke: ERROR: " << failure.what() << std::endl;
    }

    return 1;
}
