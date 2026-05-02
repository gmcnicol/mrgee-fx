#include "PluginProcessor.h"
#include "PluginEditor.h"

MrgeeJsfxAudioProcessor::MrgeeJsfxAudioProcessor()
    : AudioProcessor(BusesProperties()
   #if ! JucePlugin_IsMidiEffect
   #if ! JucePlugin_IsSynth
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
   #endif
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)
   #endif
      ),
      apvts(*this, nullptr, "PARAMS", createParameterLayout(JsfxHost::loadBundledSliderDescriptors()))
{
    jsfxHost.loadBundledScript();
}

juce::AudioProcessorValueTreeState::ParameterLayout MrgeeJsfxAudioProcessor::createParameterLayout(
    const std::vector<JsfxHost::SliderDescriptor>& descriptors)
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> parameters;
    parameters.reserve(descriptors.size());

    for (const auto& descriptor : descriptors)
    {
        if (descriptor.isEnum && descriptor.enumNames.size() > 0)
        {
            parameters.push_back(std::make_unique<juce::AudioParameterChoice>(
                descriptor.paramId,
                descriptor.name,
                descriptor.enumNames,
                juce::jlimit(0, descriptor.enumNames.size() - 1, juce::roundToInt(descriptor.defaultValue))));
            continue;
        }

        auto range = juce::NormalisableRange<float>(descriptor.minValue, descriptor.maxValue, descriptor.step);
        if (descriptor.paramId == "slider2")
            range.setSkewForCentre(1000.0f);

        parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
            descriptor.paramId,
            descriptor.name,
            range,
            descriptor.defaultValue));
    }

    return { parameters.begin(), parameters.end() };
}

const juce::String MrgeeJsfxAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool MrgeeJsfxAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool MrgeeJsfxAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool MrgeeJsfxAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double MrgeeJsfxAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int MrgeeJsfxAudioProcessor::getNumPrograms()
{
    return 1;
}

int MrgeeJsfxAudioProcessor::getCurrentProgram()
{
    return 0;
}

void MrgeeJsfxAudioProcessor::setCurrentProgram(int index)
{
    juce::ignoreUnused(index);
}

const juce::String MrgeeJsfxAudioProcessor::getProgramName(int index)
{
    juce::ignoreUnused(index);
    return {};
}

void MrgeeJsfxAudioProcessor::changeProgramName(int index, const juce::String& newName)
{
    juce::ignoreUnused(index, newName);
}

void MrgeeJsfxAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    jsfxHost.prepare(sampleRate, samplesPerBlock, getTotalNumOutputChannels());
}

void MrgeeJsfxAudioProcessor::releaseResources()
{
    jsfxHost.reset();
}

#if ! JucePlugin_IsMidiEffect
bool MrgeeJsfxAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
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

void MrgeeJsfxAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    const auto& descriptors = jsfxHost.getSliderDescriptors();
    for (const auto& descriptor : descriptors)
    {
        if (auto* parameter = apvts.getRawParameterValue(descriptor.paramId))
            jsfxHost.setSlider(descriptor.index, parameter->load());
    }

    jsfxHost.process(buffer, midiMessages);
}

juce::AudioProcessorEditor* MrgeeJsfxAudioProcessor::createEditor()
{
    return new MrgeeJsfxAudioProcessorEditor(*this);
}

void MrgeeJsfxAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    if (auto state = apvts.copyState(); state.isValid())
    {
        std::unique_ptr<juce::XmlElement> xml(state.createXml());
        copyXmlToBinary(*xml, destData);
    }
}

void MrgeeJsfxAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MrgeeJsfxAudioProcessor();
}
