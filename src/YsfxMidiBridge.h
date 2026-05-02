#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

#if MRGEE_HAS_YSFX
#include <ysfx.h>
#endif

namespace mrgee
{
#if MRGEE_HAS_YSFX
void sendMidiBufferToYsfx(ysfx_t& effect, const juce::MidiBuffer& midi, int numSamples);
void receiveMidiBufferFromYsfx(ysfx_t& effect, juce::MidiBuffer& midi, int numSamples);
#endif
} // namespace mrgee
