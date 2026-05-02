#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "JsfxHost.h"

class MrgeeJsfxAudioProcessor final : public juce::AudioProcessor
{
public:
    MrgeeJsfxAudioProcessor();
    ~MrgeeJsfxAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #if ! JucePlugin_IsMidiEffect
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
   #endif

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }
    JsfxHost& getJsfxHost() { return jsfxHost; }
    const std::vector<JsfxHost::SliderDescriptor>& getSliderDescriptors() const noexcept { return jsfxHost.getSliderDescriptors(); }
    const juce::String& getStatusMessage() const noexcept { return jsfxHost.getStatusMessage(); }

private:
    JsfxHost jsfxHost;
    juce::AudioProcessorValueTreeState apvts;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout(const std::vector<JsfxHost::SliderDescriptor>& descriptors);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MrgeeJsfxAudioProcessor)
};
