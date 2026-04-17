#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class MrgeeJSFXBridgeAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit MrgeeJSFXBridgeAudioProcessorEditor(MrgeeJSFXBridgeAudioProcessor&);
    ~MrgeeJSFXBridgeAudioProcessorEditor() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    MrgeeJSFXBridgeAudioProcessor& audioProcessor;
    juce::OwnedArray<juce::Slider> sliders;
    juce::OwnedArray<juce::Label> labels;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> attachments;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MrgeeJSFXBridgeAudioProcessorEditor)
};
