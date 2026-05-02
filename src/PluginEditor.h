#pragma once

#include <juce_gui_extra/juce_gui_extra.h>
#include "PluginProcessor.h"

class MrgeeJsfxAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                            private juce::Timer
{
public:
    explicit MrgeeJsfxAudioProcessorEditor(MrgeeJsfxAudioProcessor&);
    ~MrgeeJsfxAudioProcessorEditor() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    class ControlsComponent;

    void timerCallback() override;
    void refreshStatus();

    MrgeeJsfxAudioProcessor& audioProcessor;
    juce::Label titleLabel;
    juce::Label subtitleLabel;
    juce::Label statusLabel;
    std::unique_ptr<ControlsComponent> controlsComponent;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MrgeeJsfxAudioProcessorEditor)
};
