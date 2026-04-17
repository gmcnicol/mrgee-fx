#include "PluginEditor.h"

MrgeeJSFXBridgeAudioProcessorEditor::MrgeeJSFXBridgeAudioProcessorEditor(MrgeeJSFXBridgeAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    setSize(920, 520);

    auto& apvts = audioProcessor.getAPVTS();
    for (int i = 0; i < 8; ++i)
    {
        auto* slider = sliders.add(new juce::Slider());
        slider->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider->setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 24);
        slider->setColour(juce::Slider::rotarySliderFillColourId, juce::Colours::orange.withBrightness(0.9f));
        slider->setColour(juce::Slider::trackColourId, juce::Colours::black.withAlpha(0.4f));
        slider->setColour(juce::Slider::thumbColourId, juce::Colours::white);
        addAndMakeVisible(slider);

        auto* label = labels.add(new juce::Label());
        label->setText("JSFX Slider " + juce::String(i + 1), juce::dontSendNotification);
        label->setJustificationType(juce::Justification::centred);
        label->setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.9f));
        addAndMakeVisible(label);

        attachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            apvts,
            "slider" + juce::String(i + 1),
            *slider));
    }
}

void MrgeeJSFXBridgeAudioProcessorEditor::paint(juce::Graphics& g)
{
    juce::ColourGradient grad(
        juce::Colour::fromRGB(18, 20, 28), 0, 0,
        juce::Colour::fromRGB(37, 20, 66), 0, static_cast<float>(getHeight()), false);
    g.setGradientFill(grad);
    g.fillAll();

    g.setColour(juce::Colours::white.withAlpha(0.92f));
    g.setFont(juce::FontOptions(30.0f, juce::Font::bold));
    g.drawFittedText("Mrgee JSFX Bridge", getLocalBounds().removeFromTop(56), juce::Justification::centred, 1);

    g.setColour(juce::Colours::white.withAlpha(0.66f));
    g.setFont(juce::FontOptions(14.0f));
    g.drawFittedText("Map JUCE controls directly to JSFX sliders → export as VST3/AU", 0, 52, getWidth(), 28, juce::Justification::centred, 1);
}

void MrgeeJSFXBridgeAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(28);
    area.removeFromTop(90);

    juce::Grid grid;
    using Track = juce::Grid::TrackInfo;
    grid.templateRows = { Track(1_fr), Track(28_px), Track(1_fr), Track(28_px) };
    grid.templateColumns = {
        Track(1_fr), Track(1_fr), Track(1_fr), Track(1_fr)
    };
    grid.columnGap = juce::Grid::Px(12.0f);
    grid.rowGap = juce::Grid::Px(10.0f);

    for (int i = 0; i < 8; ++i)
    {
        grid.items.add(juce::GridItem(*sliders[i]));
        grid.items.add(juce::GridItem(*labels[i]));
    }

    grid.performLayout(area);
}
