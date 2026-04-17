#include "PluginProcessor.h"
#include "PluginEditor.h"

MrgeeJSFXBridgeAudioProcessor::MrgeeJSFXBridgeAudioProcessor()
    : AudioProcessor(BusesProperties()
   #if ! JucePlugin_IsMidiEffect
   #if ! JucePlugin_IsSynth
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
   #endif
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)
   #endif
      ),
      apvts(*this, nullptr, "PARAMS", createParameterLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout MrgeeJSFXBridgeAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> parameters;
    for (int i = 0; i < 8; ++i)
    {
        auto id = juce::String("slider") + juce::String(i + 1);
        auto name = juce::String("Slider ") + juce::String(i + 1);
        parameters.push_back(std::make_unique<juce::AudioParameterFloat>(id, name, 0.0f, 1.0f, i == 0 ? 1.0f : 0.0f));
    }
    return { parameters.begin(), parameters.end() };
}

const juce::String MrgeeJSFXBridgeAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool MrgeeJSFXBridgeAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool MrgeeJSFXBridgeAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool MrgeeJSFXBridgeAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double MrgeeJSFXBridgeAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int MrgeeJSFXBridgeAudioProcessor::getNumPrograms()
{
    return 1;
}

int MrgeeJSFXBridgeAudioProcessor::getCurrentProgram()
{
    return 0;
}

void MrgeeJSFXBridgeAudioProcessor::setCurrentProgram(int index)
{
    juce::ignoreUnused(index);
}

const juce::String MrgeeJSFXBridgeAudioProcessor::getProgramName(int index)
{
    juce::ignoreUnused(index);
    return {};
}

void MrgeeJSFXBridgeAudioProcessor::changeProgramName(int index, const juce::String& newName)
{
    juce::ignoreUnused(index, newName);
}

void MrgeeJSFXBridgeAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    jsfxHost.prepare(sampleRate, samplesPerBlock, getTotalNumOutputChannels());
}

void MrgeeJSFXBridgeAudioProcessor::releaseResources()
{
}

#if ! JucePlugin_IsMidiEffect
bool MrgeeJSFXBridgeAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
   #if JucePlugin_IsSynth
    juce::ignoreUnused(layouts);
    return true;
   #else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
   #endif
}
#endif

void MrgeeJSFXBridgeAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    for (int i = 0; i < 8; ++i)
    {
        const auto id = juce::String("slider") + juce::String(i + 1);
        if (auto* p = apvts.getRawParameterValue(id))
            jsfxHost.setSlider(i, p->load());
    }

    jsfxHost.process(buffer, midiMessages);
}

bool MrgeeJSFXBridgeAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* MrgeeJSFXBridgeAudioProcessor::createEditor()
{
    return new MrgeeJSFXBridgeAudioProcessorEditor(*this);
}

void MrgeeJSFXBridgeAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    if (auto state = apvts.copyState(); state.isValid())
    {
        std::unique_ptr<juce::XmlElement> xml(state.createXml());
        copyXmlToBinary(*xml, destData);
    }
}

void MrgeeJSFXBridgeAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MrgeeJSFXBridgeAudioProcessor();
}
