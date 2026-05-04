#include "JsfxHost.h"
#include "YsfxMidiBridge.h"

#include <BinaryData.h>
#include <MrgeeJsfxBundle.h>

#include <algorithm>
#include <cstring>

#include <ysfx.h>

namespace
{
juce::String sanitizeRelativePath(juce::String path)
{
    path = path.replaceCharacter('\\', '/').trim();

    while (path.startsWithChar('/'))
        path = path.substring(1);

    juce::StringArray parts;
    parts.addTokens(path, "/", {});
    parts.removeEmptyStrings();

    juce::StringArray cleanParts;
    for (const auto& part : parts)
    {
        if (part == "." || part == "..")
            continue;

        cleanParts.add(part);
    }

    return cleanParts.joinIntoString("/");
}

bool isDataRelativePath(const juce::String& path)
{
    return path == "Data" || path.startsWith("Data/");
}

juce::File getBundleRoot()
{
    return juce::File::getSpecialLocation(juce::File::tempDirectory)
        .getChildFile("mrgee-jsfx")
        .getChildFile(mrgee::generated::kJsfxBundleId);
}

juce::String sanitizeSliderName(juce::String name)
{
    if (const auto brace = name.indexOfChar('{'); brace >= 0)
        name = name.substring(0, brace).trimEnd();

    return name.trim();
}
}

struct JsfxHost::Runtime
{
    static void logCallback(intptr_t userdata, ysfx_log_level level, const char* message)
    {
        auto* self = reinterpret_cast<JsfxHost*>(userdata);
        if (self == nullptr || message == nullptr)
            return;

        juce::String levelPrefix;
        switch (level)
        {
            case ysfx_log_info: levelPrefix = "info"; break;
            case ysfx_log_warning: levelPrefix = "warning"; break;
            case ysfx_log_error: levelPrefix = "error"; break;
        }

        self->updateStatus("ysfx " + levelPrefix + ": " + juce::String(message), level != ysfx_log_error);
    }

    ysfx_config_t* config = nullptr;
    ysfx_t* effect = nullptr;

    ~Runtime()
    {
        if (effect != nullptr)
            ysfx_free(effect);
        if (config != nullptr)
            ysfx_config_free(config);
    }
};

JsfxHost::JsfxHost()
{
    loadFallbackDescriptors();
    applySliderDefaults();
    updateStatus("ysfx runtime not loaded yet.", false);
}

JsfxHost::~JsfxHost() = default;

const JsfxHost::BundleEntry* JsfxHost::JsfxBundle::getMainScript() const noexcept
{
    if (mrgee::generated::kJsfxMainScriptIndex < 0
        || static_cast<size_t>(mrgee::generated::kJsfxMainScriptIndex) >= entries.size())
        return nullptr;

    return &entries[static_cast<size_t>(mrgee::generated::kJsfxMainScriptIndex)];
}

std::vector<JsfxHost::SliderDescriptor> JsfxHost::loadBundledSliderDescriptors()
{
    return parseSliderDescriptors(getBundledScriptText());
}

bool JsfxHost::loadBundledScript()
{
    const auto fallbackDescriptors = loadBundledSliderDescriptors();
    sliderDescriptors = fallbackDescriptors;
    applySliderDefaults();

    auto scriptFile = materializeBundledBundle();

    runtime = std::make_unique<Runtime>();
    runtime->config = ysfx_config_new();
    if (runtime->config == nullptr)
    {
        updateStatus("Failed to create ysfx configuration.", false);
        return false;
    }

    ysfx_set_user_data(runtime->config, reinterpret_cast<intptr_t>(this));
    ysfx_set_log_reporter(runtime->config, &Runtime::logCallback);
    ysfx_register_builtin_audio_formats(runtime->config);

    auto effectsRoot = scriptFile.getParentDirectory();
    auto dataRoot = effectsRoot.getSiblingFile("Data");
    dataRoot.createDirectory();

    ysfx_set_import_root(runtime->config, effectsRoot.getFullPathName().toRawUTF8());
    ysfx_set_data_root(runtime->config, dataRoot.getFullPathName().toRawUTF8());

    runtime->effect = ysfx_new(runtime->config);
    if (runtime->effect == nullptr)
    {
        runtime.reset();
        updateStatus("Failed to create ysfx effect runtime.", false);
        return false;
    }

    if (! ysfx_load_file(runtime->effect, scriptFile.getFullPathName().toRawUTF8(), 0))
    {
        runtime.reset();
        updateStatus("Failed to load bundled JSFX script.", false);
        return false;
    }

    if (! ysfx_compile(runtime->effect, ysfx_compile_no_gfx))
    {
        runtime.reset();
        updateStatus("Failed to compile bundled JSFX script.", false);
        return false;
    }

    sliderDescriptors.clear();
    for (uint32_t index = 0; index < ysfx_max_sliders; ++index)
    {
        if (! ysfx_slider_exists(runtime->effect, index))
            continue;

        SliderDescriptor descriptor;
        descriptor.index = static_cast<int>(index);
        descriptor.paramId = "slider" + juce::String(static_cast<int>(index + 1));
        descriptor.name = sanitizeSliderName(juce::String(ysfx_slider_get_name(runtime->effect, index)));
        descriptor.isVisible = ysfx_slider_is_initially_visible(runtime->effect, index);
        descriptor.isEnum = ysfx_slider_is_enum(runtime->effect, index);

        ysfx_slider_range_t range {};
        if (ysfx_slider_get_range(runtime->effect, index, &range))
        {
            descriptor.defaultValue = static_cast<float>(range.def);
            descriptor.minValue = static_cast<float>(range.min);
            descriptor.maxValue = static_cast<float>(range.max);
            descriptor.step = static_cast<float>(range.inc > 0.0 ? range.inc : 0.01);
        }

        if (descriptor.isEnum)
        {
            const auto enumCount = ysfx_slider_get_enum_size(runtime->effect, index);
            for (uint32_t enumIndex = 0; enumIndex < enumCount; ++enumIndex)
                descriptor.enumNames.add(juce::String(ysfx_slider_get_enum_name(runtime->effect, index, enumIndex)));
            descriptor.step = 1.0f;
        }

        const auto fallbackIt = std::find_if(fallbackDescriptors.begin(),
                                             fallbackDescriptors.end(),
                                             [index](const SliderDescriptor& fallback)
                                             {
                                                 return fallback.index == static_cast<int>(index);
                                             });

        if (fallbackIt != fallbackDescriptors.end())
        {
            if (descriptor.name.isEmpty())
                descriptor.name = fallbackIt->name;

            if (! descriptor.isEnum && fallbackIt->isEnum)
                descriptor.isEnum = true;

            if (descriptor.isEnum && descriptor.enumNames.isEmpty())
                descriptor.enumNames = fallbackIt->enumNames;
        }

        sliderDescriptors.push_back(std::move(descriptor));
    }

    // Some hosts/builds expose valid slider ranges and names through ysfx while reporting
    // every slider as initially hidden. If we trusted that literally, the custom editor would
    // render as an empty shell even though the script defines a real control surface.
    const bool anyVisibleSlider = std::any_of(sliderDescriptors.begin(),
                                              sliderDescriptors.end(),
                                              [](const SliderDescriptor& descriptor)
                                              {
                                                  return descriptor.isVisible;
                                              });

    if (! anyVisibleSlider)
    {
        for (auto& descriptor : sliderDescriptors)
        {
            const auto fallbackIt = std::find_if(fallbackDescriptors.begin(),
                                                 fallbackDescriptors.end(),
                                                 [&descriptor](const SliderDescriptor& fallback)
                                                 {
                                                     return fallback.index == descriptor.index;
                                                 });

            if (fallbackIt != fallbackDescriptors.end())
                descriptor.isVisible = fallbackIt->isVisible;
        }
    }

    if (sliderDescriptors.empty())
    {
        sliderDescriptors = fallbackDescriptors;
        applySliderDefaults();
        ysfx_set_midi_capacity(runtime->effect, 2048, true);
        ysfx_init(runtime->effect);
        updateStatus("ysfx runtime loaded bundled JSFX script with no exposed sliders.", true);
        return true;
    }

    applySliderDefaults();
    ysfx_set_midi_capacity(runtime->effect, 2048, true);
    ysfx_init(runtime->effect);
    updateStatus("ysfx runtime loaded bundled JSFX script.", true);
    return true;
}

void JsfxHost::prepare(double sampleRate, int samplesPerBlock, int channels)
{
    if (runtime != nullptr && runtime->effect != nullptr)
    {
        ysfx_set_sample_rate(runtime->effect, sampleRate);
        ysfx_set_block_size(runtime->effect, static_cast<uint32_t>(juce::jmax(1, samplesPerBlock)));
        ysfx_set_midi_capacity(runtime->effect, static_cast<uint32_t>(juce::jmax(256, samplesPerBlock * 4)), true);
        ysfx_init(runtime->effect);
    }

    juce::ignoreUnused(channels);
}

void JsfxHost::reset()
{
    if (runtime != nullptr && runtime->effect != nullptr)
        ysfx_init(runtime->effect);
}

void JsfxHost::setSlider(int sliderIndex, float value)
{
    sliderValues.set(sliderIndex, value);

    if (runtime != nullptr && runtime->effect != nullptr)
        ysfx_slider_set_value(runtime->effect, static_cast<uint32_t>(sliderIndex), value);
}

float JsfxHost::getSlider(int sliderIndex) const
{
    return sliderValues[sliderIndex];
}

float JsfxHost::getRuntimeSlider(int sliderIndex) const
{
    if (runtime != nullptr && runtime->effect != nullptr && sliderIndex >= 0)
        return static_cast<float>(ysfx_slider_get_value(runtime->effect, static_cast<uint32_t>(sliderIndex)));

    return getSlider(sliderIndex);
}

void JsfxHost::process(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    if (runtime != nullptr && runtime->effect != nullptr)
    {
        mrgee::sendMidiBufferToYsfx(*runtime->effect, midi, buffer.getNumSamples());

        std::vector<const float*> inputs(static_cast<size_t>(buffer.getNumChannels()));
        std::vector<float*> outputs(static_cast<size_t>(buffer.getNumChannels()));
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        {
            inputs[static_cast<size_t>(channel)] = buffer.getReadPointer(channel);
            outputs[static_cast<size_t>(channel)] = buffer.getWritePointer(channel);
        }

        ysfx_process_float(runtime->effect,
                           inputs.data(),
                           outputs.data(),
                           static_cast<uint32_t>(inputs.size()),
                           static_cast<uint32_t>(outputs.size()),
                           static_cast<uint32_t>(buffer.getNumSamples()));
        mrgee::receiveMidiBufferFromYsfx(*runtime->effect, midi, buffer.getNumSamples());
        return;
    }

    juce::ignoreUnused(buffer, midi);
}

void JsfxHost::updateStatus(juce::String message, bool runtimeActive)
{
    statusMessage = std::move(message);
    runtimeLoaded = runtimeActive;
}

void JsfxHost::loadFallbackDescriptors()
{
    sliderDescriptors = loadBundledSliderDescriptors();
}

void JsfxHost::applySliderDefaults()
{
    sliderValues.clearQuick();
    for (const auto& descriptor : sliderDescriptors)
        sliderValues.add(descriptor.defaultValue);
}

juce::File JsfxHost::getMaterializedBundleRoot() const
{
    return getBundleRoot();
}

juce::File JsfxHost::materializeBundledBundle() const
{
    const auto bundle = getBundledJsfxBundle();
    auto root = getBundleRoot();
    auto effectsRoot = root.getChildFile("Effects");
    auto dataRoot = root.getChildFile("Data");
    effectsRoot.createDirectory();
    dataRoot.createDirectory();

    for (const auto& entry : bundle.entries)
    {
        const auto cleanPath = sanitizeRelativePath(entry.logicalPath);
        if (cleanPath.isEmpty() || entry.data == nullptr || entry.size < 0)
            continue;

        const auto outputFile = isDataRelativePath(cleanPath)
            ? root.getChildFile(cleanPath)
            : effectsRoot.getChildFile(cleanPath);

        outputFile.getParentDirectory().createDirectory();

        juce::MemoryBlock existing;
        const bool existingMatches = outputFile.existsAsFile()
            && outputFile.loadFileAsData(existing)
            && existing.getSize() == static_cast<size_t>(entry.size)
            && std::memcmp(existing.getData(), entry.data, static_cast<size_t>(entry.size)) == 0;

        if (! existingMatches)
            outputFile.replaceWithData(entry.data, static_cast<size_t>(entry.size));
    }

    return effectsRoot.getChildFile(bundle.mainScriptName);
}

JsfxHost::JsfxBundle JsfxHost::getBundledJsfxBundle()
{
    JsfxBundle bundle;
    bundle.bundleId = mrgee::generated::kJsfxBundleId;
    bundle.mainScriptName = mrgee::generated::kJsfxMainScriptName;
    bundle.entries.reserve(mrgee::generated::kJsfxBundleEntryCount);

    const auto entryCount = juce::jmin(static_cast<int>(mrgee::generated::kJsfxBundleEntryCount),
                                      BinaryData::namedResourceListSize);

    for (int index = 0; index < entryCount; ++index)
    {
        int dataSize = 0;
        auto* data = BinaryData::getNamedResource(BinaryData::namedResourceList[index], dataSize);
        bundle.entries.push_back({
            mrgee::generated::kJsfxBundleLogicalPaths[index],
            data,
            dataSize,
        });
    }

    return bundle;
}

juce::String JsfxHost::getBundledScriptText()
{
    const auto bundle = getBundledJsfxBundle();
    if (const auto* mainScript = bundle.getMainScript())
    {
        if (mainScript->data == nullptr || mainScript->size <= 0)
            return {};

        return juce::String::fromUTF8(mainScript->data, mainScript->size);
    }

    return {};
}

std::vector<JsfxHost::SliderDescriptor> JsfxHost::parseSliderDescriptors(const juce::String& scriptText)
{
    std::vector<SliderDescriptor> descriptors;
    auto lines = juce::StringArray::fromLines(scriptText);

    for (const auto& rawLine : lines)
    {
        auto line = rawLine.trim();
        if (! line.startsWithIgnoreCase("slider"))
            continue;

        const auto colon = line.indexOfChar(':');
        const auto lt = line.indexOfChar('<');
        const auto gt = line.indexOfChar('>');
        if (colon < 0 || lt < 0 || gt < 0 || colon > lt || lt > gt)
            continue;

        SliderDescriptor descriptor;
        descriptor.index = line.substring(6, colon).trim().getIntValue() - 1;
        if (descriptor.index < 0)
            continue;

        descriptor.paramId = "slider" + juce::String(descriptor.index + 1);
        descriptor.defaultValue = line.substring(colon + 1, lt).trim().getFloatValue();

        auto rangeTokens = juce::StringArray::fromTokens(line.substring(lt + 1, gt), ",", {});
        rangeTokens.trim();
        rangeTokens.removeEmptyStrings();
        if (rangeTokens.size() < 3)
            continue;

        descriptor.minValue = rangeTokens[0].getFloatValue();
        descriptor.maxValue = rangeTokens[1].getFloatValue();
        descriptor.step = juce::jmax(0.0001f, rangeTokens[2].upToFirstOccurrenceOf(":", false, false).getFloatValue());

        auto suffix = line.substring(gt + 1).trim();
        auto braceStart = suffix.indexOfChar('{');
        auto braceEnd = suffix.lastIndexOfChar('}');
        if (braceStart >= 0 && braceEnd > braceStart)
        {
            descriptor.name = suffix.substring(0, braceStart).trim();
        }
        else
        {
            descriptor.name = suffix.trim();
        }

        auto enumText = (braceStart >= 0 && braceEnd > braceStart)
                            ? suffix.substring(braceStart + 1, braceEnd).trim()
                            : juce::String();
        if (enumText.isNotEmpty())
        {
            descriptor.isEnum = true;
            descriptor.step = 1.0f;
            descriptor.enumNames.addTokens(enumText, ",", {});
            descriptor.enumNames.trim();
            descriptor.enumNames.removeEmptyStrings();
        }

        descriptors.push_back(std::move(descriptor));
    }

    std::sort(descriptors.begin(), descriptors.end(), [](const auto& lhs, const auto& rhs) {
        return lhs.index < rhs.index;
    });

    return descriptors;
}
