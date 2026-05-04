#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <ysfx.h>

namespace mrgee
{
void sendMidiBufferToYsfx(ysfx_t& effect, const juce::MidiBuffer& midi, int numSamples);
void receiveMidiBufferFromYsfx(ysfx_t& effect, juce::MidiBuffer& midi, int numSamples);
} // namespace mrgee
