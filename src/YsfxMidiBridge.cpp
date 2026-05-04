#include "YsfxMidiBridge.h"

namespace mrgee
{
namespace
{
uint32_t clampMidiOffset(int samplePosition, int numSamples)
{
    return static_cast<uint32_t>(juce::jlimit(0, juce::jmax(0, numSamples - 1), samplePosition));
}
} // namespace

void sendMidiBufferToYsfx(ysfx_t& effect, const juce::MidiBuffer& midi, int numSamples)
{
    for (const auto metadata : midi)
    {
        ysfx_midi_event_t event {};
        event.bus = 0;
        event.offset = clampMidiOffset(metadata.samplePosition, numSamples);
        event.size = static_cast<uint32_t>(metadata.numBytes);
        event.data = metadata.data;
        ysfx_send_midi(&effect, &event);
    }
}

void receiveMidiBufferFromYsfx(ysfx_t& effect, juce::MidiBuffer& midi, int numSamples)
{
    midi.clear();

    ysfx_midi_event_t event {};
    while (ysfx_receive_midi(&effect, &event))
    {
        const auto boundedOffset = static_cast<int>(juce::jmin<uint32_t>(event.offset, static_cast<uint32_t>(juce::jmax(0, numSamples - 1))));
        midi.addEvent(event.data, static_cast<int>(event.size), boundedOffset);
    }
}
} // namespace mrgee
