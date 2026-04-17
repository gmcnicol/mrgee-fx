#pragma once

#include <JuceHeader.h>
#include <cmath>

class JsfxHost
{
public:
    bool loadScript(const juce::File& scriptFile)
    {
        loadedScript = scriptFile;
       #if MRGEE_HAS_YSFX
        // TODO: initialize ysfx VM + compile script.
       #endif
        return scriptFile.existsAsFile();
    }

    void prepare(double sampleRate, int samplesPerBlock, int channels)
    {
        juce::ignoreUnused(sampleRate, samplesPerBlock, channels);
    }

    void setSlider(int sliderIndex, float value)
    {
        sliderValues.set(sliderIndex, value);
        #if MRGEE_HAS_YSFX
        // TODO: ysfx_slider_set_value(runtime, sliderIndex, value);
        #endif
    }

    float getSlider(int sliderIndex) const
    {
        return sliderValues[sliderIndex];
    }

    void process(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
    {
        juce::ignoreUnused(midi);
        #if MRGEE_HAS_YSFX
        // TODO: pass audio + MIDI to ysfx runtime process callback.
        #else
        // Mock behavior for bootstrap: soft clip to verify DSP path + parameter mapping.
        auto gain = juce::jlimit(0.0f, 2.0f, sliderValues[0]);
        for (auto ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            auto* data = buffer.getWritePointer(ch);
            for (auto i = 0; i < buffer.getNumSamples(); ++i)
                data[i] = std::tanh(data[i] * gain);
        }
        #endif
    }

private:
    juce::File loadedScript;
    juce::Array<float> sliderValues { 1.0f, 0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
};
