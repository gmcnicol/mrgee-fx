#include "../src/PluginProcessor.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>

#include <cmath>
#include <iostream>
#include <memory>
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

void require(bool condition, const juce::String& message)
{
    if (! condition)
        throw CheckFailure(message.toStdString());
}

juce::StringArray expectedParameterNames()
{
    return { "Mode", "Cutoff", "Q", "Slope" };
}

struct ComponentSummary
{
    int sliderCount = 0;
    int comboCount = 0;
    int buttonCount = 0;
    int viewportCount = 0;
    juce::StringArray labelTexts;
};

void collectComponentSummary(juce::Component& component, ComponentSummary& summary)
{
    if (dynamic_cast<juce::Slider*>(&component) != nullptr)
        ++summary.sliderCount;

    if (dynamic_cast<juce::ComboBox*>(&component) != nullptr)
        ++summary.comboCount;

    if (dynamic_cast<juce::Button*>(&component) != nullptr)
        ++summary.buttonCount;

    if (dynamic_cast<juce::Viewport*>(&component) != nullptr)
        ++summary.viewportCount;

    if (auto* label = dynamic_cast<juce::Label*>(&component))
        summary.labelTexts.add(label->getText());

    for (auto* child : component.getChildren())
        collectComponentSummary(*child, summary);
}

juce::String componentTypeName(juce::Component& component)
{
    if (dynamic_cast<juce::Viewport*>(&component) != nullptr)
        return "Viewport";

    if (dynamic_cast<juce::Slider*>(&component) != nullptr)
        return "Slider";

    if (dynamic_cast<juce::ComboBox*>(&component) != nullptr)
        return "ComboBox";

    if (dynamic_cast<juce::Button*>(&component) != nullptr)
        return "Button";

    if (dynamic_cast<juce::ScrollBar*>(&component) != nullptr)
        return "ScrollBar";

    if (dynamic_cast<juce::Label*>(&component) != nullptr)
        return "Label";

    return "Component";
}

void lintComponentBounds(juce::Component& component, const juce::String& path)
{
    const auto childCount = component.getNumChildComponents();

    for (int childIndex = 0; childIndex < childCount; ++childIndex)
    {
        auto* child = component.getChildComponent(childIndex);
        require(child != nullptr, "Null child component at " + path);

        const auto bounds = child->getBounds();
        auto childName = child->getName();
        if (childName.isEmpty())
            childName = componentTypeName(*child);
        const auto childPath = path + "/" + childName;

        if (dynamic_cast<juce::ScrollBar*>(child) != nullptr && ! child->isVisible())
            continue;

        const bool isLintedControl = dynamic_cast<juce::Label*>(child) != nullptr
            || dynamic_cast<juce::Slider*>(child) != nullptr
            || dynamic_cast<juce::ComboBox*>(child) != nullptr
            || dynamic_cast<juce::Viewport*>(child) != nullptr;

        require(child->isVisible(), "Hidden UI component: " + childPath);
        require(bounds.getWidth() > 0 && bounds.getHeight() > 0,
                "Collapsed UI component: " + childPath + " bounds=" + bounds.toString());
        require(bounds.getX() >= 0 && bounds.getY() >= 0,
                "UI component starts outside parent: " + childPath + " bounds=" + bounds.toString());

        if (auto* viewport = dynamic_cast<juce::Viewport*>(&component);
            viewport != nullptr && viewport->getViewedComponent() == child)
        {
            require(bounds.getWidth() <= component.getWidth(),
                    "Viewport content is horizontally clipped: " + childPath + " bounds=" + bounds.toString());
        }
        else
        {
            if (isLintedControl)
                require(bounds.getRight() <= component.getWidth() && bounds.getBottom() <= component.getHeight(),
                        "UI component is clipped by parent: " + childPath + " bounds=" + bounds.toString()
                            + " parent=" + component.getLocalBounds().toString());
        }

        if (dynamic_cast<juce::Slider*>(child) != nullptr)
            require(bounds.getWidth() >= 96 && bounds.getHeight() >= 72,
                    "Slider is too small to use: " + childPath + " bounds=" + bounds.toString());

        if (dynamic_cast<juce::ComboBox*>(child) != nullptr)
            require(bounds.getWidth() >= 96 && bounds.getHeight() >= 24,
                    "ComboBox is too small to use: " + childPath + " bounds=" + bounds.toString());

        if (auto* label = dynamic_cast<juce::Label*>(child))
            require(label->getText().trim().isNotEmpty(),
                    "Visible label has no text: " + childPath);

        lintComponentBounds(*child, childPath);
    }
}

void lintEditorLayout(juce::AudioProcessorEditor& editor, int width, int height)
{
    editor.setSize(width, height);
    editor.resized();

    require(editor.getWidth() == width && editor.getHeight() == height,
            "Editor did not accept lint size "
                + juce::String(width) + "x" + juce::String(height));
    lintComponentBounds(editor, "editor");
}

juce::AudioProcessorParameter* findParameterByName(juce::AudioProcessor& processor, const juce::String& name)
{
    for (auto* parameter : processor.getParameters())
        if (parameter->getName(128) == name)
            return parameter;

    return nullptr;
}

void setParameterValue(juce::AudioProcessor& processor, const juce::String& name, float rawValue)
{
    auto* parameter = findParameterByName(processor, name);
    require(parameter != nullptr, "Missing parameter: " + name);
    auto* ranged = dynamic_cast<juce::RangedAudioParameter*>(parameter);
    require(ranged != nullptr, "Parameter is not ranged: " + name);
    parameter->setValueNotifyingHost(ranged->convertTo0to1(rawValue));
}

float getParameterValue(juce::AudioProcessor& processor, const juce::String& name)
{
    auto* parameter = findParameterByName(processor, name);
    require(parameter != nullptr, "Missing parameter: " + name);
    auto* ranged = dynamic_cast<juce::RangedAudioParameter*>(parameter);
    require(ranged != nullptr, "Parameter is not ranged: " + name);
    return ranged->convertFrom0to1(parameter->getValue());
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
    const auto numFrames = buffer.getNumSamples();
    const auto framesToInspect = numFrames - kAnalysisSkipFrames;
    require(framesToInspect > 0, "Render too short for analysis");

    for (int sample = kAnalysisSkipFrames; sample < numFrames; ++sample)
    {
        const auto t = static_cast<double>(sample) / kSampleRate;
        const auto phase = juce::MathConstants<double>::twoPi * frequencyHz * t;
        const auto mono = 0.5 * (buffer.getSample(0, sample) + buffer.getSample(1, sample));
        real += mono * std::cos(phase);
        imag -= mono * std::sin(phase);
    }

    return std::sqrt(real * real + imag * imag) / static_cast<double>(framesToInspect);
}

juce::AudioBuffer<float> renderDirectProcessor(float mode, float cutoff, float q, float slope)
{
    MrgeeJsfxAudioProcessor processor;
    processor.prepareToPlay(kSampleRate, kBlockSize);

    setParameterValue(processor, "Mode", mode);
    setParameterValue(processor, "Cutoff", cutoff);
    setParameterValue(processor, "Q", q);
    setParameterValue(processor, "Slope", slope);

    auto rendered = makeInputBuffer(kRenderFrames);
    juce::MidiBuffer midi;

    for (int offset = 0; offset < kRenderFrames; offset += kBlockSize)
    {
        const auto blockFrames = juce::jmin(kBlockSize, kRenderFrames - offset);
        juce::AudioBuffer<float> block(rendered.getArrayOfWritePointers(),
                                       rendered.getNumChannels(),
                                       offset,
                                       blockFrames);
        processor.processBlock(block, midi);
    }

    return rendered;
}

void verifyDirectRuntimeAndUi()
{
    MrgeeJsfxAudioProcessor processor;
    auto& host = processor.getJsfxHost();
    const auto& descriptors = processor.getSliderDescriptors();

    require(descriptors.size() == 4, "Expected 4 JSFX descriptors");
    require(host.hasRuntime(), "ysfx runtime was not loaded: " + host.getStatusMessage());

    const auto names = expectedParameterNames();
    for (size_t i = 0; i < descriptors.size(); ++i)
    {
        require(descriptors[i].name == names[static_cast<int>(i)],
                "Unexpected descriptor order/name at index "
                    + juce::String(static_cast<int>(i))
                    + ": got [" + descriptors[i].name + "] expected [" + names[static_cast<int>(i)] + "]");
        require(descriptors[i].paramId == "slider" + juce::String(static_cast<int>(i + 1)), "Unexpected parameter id");
    }

    require(descriptors[0].isEnum && descriptors[0].enumNames == juce::StringArray({ "Lowpass", "Highpass" }),
            "Mode enum metadata mismatch: isEnum="
                + juce::String(descriptors[0].isEnum ? "true" : "false")
                + " got [" + descriptors[0].enumNames.joinIntoString(", ") + "]");
    require(! descriptors[1].isEnum && descriptors[1].minValue <= 20.0f && descriptors[1].maxValue >= 20000.0f,
            "Cutoff range mismatch");
    require(! descriptors[2].isEnum && descriptors[2].minValue <= 0.1f && descriptors[2].maxValue >= 10.0f,
            "Q range mismatch");
    require(descriptors[3].isEnum && descriptors[3].enumNames == juce::StringArray({ "12 dB", "24 dB", "48 dB" }),
            "Slope enum metadata mismatch: got [" + descriptors[3].enumNames.joinIntoString(", ") + "]");

    require(host.getStatusMessage().containsIgnoreCase("ysfx runtime loaded"), "Status did not report active ysfx runtime");

    auto directIdsA = juce::StringArray();
    auto directIdsB = juce::StringArray();

    for (auto* parameter : processor.getParameters())
        if (auto* withId = dynamic_cast<juce::AudioProcessorParameterWithID*>(parameter))
            directIdsA.add(withId->paramID);

    MrgeeJsfxAudioProcessor secondProcessor;
    for (auto* parameter : secondProcessor.getParameters())
        if (auto* withId = dynamic_cast<juce::AudioProcessorParameterWithID*>(parameter))
            directIdsB.add(withId->paramID);

    require(directIdsA == directIdsB, "Parameter IDs changed across runs");

    processor.prepareToPlay(kSampleRate, kBlockSize);
    setParameterValue(processor, "Mode", 1.0f);
    setParameterValue(processor, "Cutoff", 1800.0f);
    setParameterValue(processor, "Q", 1.2f);
    setParameterValue(processor, "Slope", 2.0f);

    juce::MemoryBlock state;
    processor.getStateInformation(state);

    MrgeeJsfxAudioProcessor restored;
    restored.setStateInformation(state.getData(), static_cast<int>(state.getSize()));

    require(std::abs(getParameterValue(restored, "Mode") - 1.0f) < 0.01f, "Mode state round-trip failed");
    require(std::abs(getParameterValue(restored, "Cutoff") - 1800.0f) < 1.0f, "Cutoff state round-trip failed");
    require(std::abs(getParameterValue(restored, "Q") - 1.2f) < 0.01f, "Q state round-trip failed");
    require(std::abs(getParameterValue(restored, "Slope") - 2.0f) < 0.01f, "Slope state round-trip failed");

    auto editor = std::unique_ptr<juce::AudioProcessorEditor>(processor.createEditor());
    require(editor != nullptr, "Failed to create editor");
    lintComponentBounds(*editor, "editor.initial");
    lintEditorLayout(*editor, 960, 620);
    lintEditorLayout(*editor, 560, 420);

    ComponentSummary summary;
    collectComponentSummary(*editor, summary);

    require(summary.viewportCount >= 1, "Expected scrollable editor viewport");
    require(summary.sliderCount >= 2, "Expected at least 2 sliders in editor");
    require(summary.comboCount + summary.buttonCount >= 2, "Expected enum controls in editor");

    for (const auto& name : names)
        require(summary.labelTexts.contains(name), "Editor missing control label: " + name);

    for (const auto& label : summary.labelTexts)
        require(! label.startsWithIgnoreCase("JSFX Slider"), "Editor still shows generic JSFX slider labels");

    auto rendered = renderDirectProcessor(0.0f, 1000.0f, 0.707f, 0.0f);
    const auto lowpassLow = analyseFrequencyMagnitude(rendered, 200.0);
    const auto lowpassHigh = analyseFrequencyMagnitude(rendered, 5000.0);

    require(lowpassLow > lowpassHigh * 3.0, "Direct render did not behave like active ysfx lowpass");
}

}

int main(int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI juceInit;

    try
    {
        juce::ignoreUnused(argc, argv);
        verifyDirectRuntimeAndUi();

        std::cout << "jsfx_smoke: OK" << std::endl;
        return 0;
    }
    catch (const CheckFailure& failure)
    {
        std::cerr << "jsfx_smoke: FAIL: " << failure.what() << std::endl;
    }
    catch (const std::exception& failure)
    {
        std::cerr << "jsfx_smoke: ERROR: " << failure.what() << std::endl;
    }

    return 1;
}
