#include "../src/YsfxMidiBridge.h"

#include <juce_audio_basics/juce_audio_basics.h>

#include <array>
#include <cstring>
#include <stdexcept>

#include <ysfx.h>

#include <iostream>

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

class ScopedYsfxRuntime
{
public:
    ScopedYsfxRuntime()
    {
        config = ysfx_config_new();
        require(config != nullptr, "Failed to create ysfx config");
        ysfx_register_builtin_audio_formats(config);

        auto root = juce::File::getSpecialLocation(juce::File::tempDirectory)
                        .getChildFile("mrgee-midi-bridge-smoke");
        auto effectsRoot = root.getChildFile("Effects");
        auto dataRoot = root.getChildFile("Data");
        effectsRoot.createDirectory();
        dataRoot.createDirectory();

        scriptFile = effectsRoot.getChildFile("MidiPassthrough.jsfx");
        scriptFile.replaceWithText(
            "desc:Mrgee MIDI Passthrough\n"
            "@block\n"
            "while (midirecv(offset, msg1, msg23)) (\n"
            "  midisend(offset, msg1, msg23);\n"
            ");\n");

        ysfx_set_import_root(config, effectsRoot.getFullPathName().toRawUTF8());
        ysfx_set_data_root(config, dataRoot.getFullPathName().toRawUTF8());

        effect = ysfx_new(config);
        require(effect != nullptr, "Failed to create ysfx effect");
        require(ysfx_load_file(effect, scriptFile.getFullPathName().toRawUTF8(), 0), "Failed to load MIDI smoke script");
        require(ysfx_compile(effect, ysfx_compile_no_gfx), "Failed to compile MIDI smoke script");

        ysfx_set_sample_rate(effect, 48000.0);
        ysfx_set_block_size(effect, 256);
        ysfx_set_midi_capacity(effect, 256, true);
        ysfx_init(effect);
    }

    ~ScopedYsfxRuntime()
    {
        if (effect != nullptr)
            ysfx_free(effect);
        if (config != nullptr)
            ysfx_config_free(config);
    }

    ysfx_t& getEffect() const
    {
        return *effect;
    }

private:
    ysfx_config_t* config = nullptr;
    ysfx_t* effect = nullptr;
    juce::File scriptFile;
};

void verifyMidiRoundTrip()
{
    ScopedYsfxRuntime runtime;
    auto& effect = runtime.getEffect();

    juce::MidiBuffer midiIn;
    midiIn.addEvent(juce::MidiMessage::noteOn(1, 60, (juce::uint8) 100), 12);
    midiIn.addEvent(juce::MidiMessage::controllerEvent(1, 74, 96), 128);
    midiIn.addEvent(juce::MidiMessage::noteOff(1, 60), 255);

    mrgee::sendMidiBufferToYsfx(effect, midiIn, 256);

    juce::AudioBuffer<float> audio(2, 256);
    audio.clear();
    std::array<const float*, 2> ins { audio.getReadPointer(0), audio.getReadPointer(1) };
    std::array<float*, 2> outs { audio.getWritePointer(0), audio.getWritePointer(1) };

    ysfx_process_float(&effect, ins.data(), outs.data(), 2, 2, 256);

    juce::MidiBuffer midiOut;
    mrgee::receiveMidiBufferFromYsfx(effect, midiOut, 256);

    auto inIt = midiIn.begin();
    auto outIt = midiOut.begin();
    for (; inIt != midiIn.end() && outIt != midiOut.end(); ++inIt, ++outIt)
    {
        const auto inEvent = *inIt;
        const auto outEvent = *outIt;

        require(inEvent.samplePosition == outEvent.samplePosition, "MIDI offset mismatch after ysfx round-trip");
        require(inEvent.numBytes == outEvent.numBytes, "MIDI message size mismatch after ysfx round-trip");
        require(std::memcmp(inEvent.data, outEvent.data, static_cast<size_t>(inEvent.numBytes)) == 0,
                "MIDI payload mismatch after ysfx round-trip");
    }

    require(inIt == midiIn.end() && outIt == midiOut.end(), "MIDI event count mismatch after ysfx round-trip");
}
} // namespace

int main()
{
    try
    {
        verifyMidiRoundTrip();
        std::cout << "mrgee_midi_bridge_smoke: OK" << std::endl;
        return 0;
    }
    catch (const CheckFailure& failure)
    {
        std::cerr << "mrgee_midi_bridge_smoke: FAIL: " << failure.what() << std::endl;
    }
    catch (const std::exception& failure)
    {
        std::cerr << "mrgee_midi_bridge_smoke: ERROR: " << failure.what() << std::endl;
    }

    return 1;
}
