#include "PluginEditor.h"

namespace
{
constexpr int kCardWidth = 210;
constexpr int kCardHeight = 160;
}

class MrgeeJsfxAudioProcessorEditor::ControlsComponent final : public juce::Component
{
public:
    explicit ControlsComponent(MrgeeJsfxAudioProcessor& processor)
    {
        auto& apvts = processor.getAPVTS();
        const auto& descriptors = processor.getSliderDescriptors();
        const bool anyVisibleDescriptor = std::any_of(descriptors.begin(),
                                                      descriptors.end(),
                                                      [](const auto& descriptor)
                                                      {
                                                          return descriptor.isVisible;
                                                      });

        for (const auto& descriptor : descriptors)
        {
            if (anyVisibleDescriptor && ! descriptor.isVisible)
                continue;

            auto label = std::make_unique<juce::Label>();
            label->setText(descriptor.name, juce::dontSendNotification);
            label->setJustificationType(juce::Justification::centred);
            label->setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.92f));
            addAndMakeVisible(*label);
            labels.push_back(std::move(label));

            if (descriptor.isEnum)
            {
                auto combo = std::make_unique<juce::ComboBox>();
                combo->setColour(juce::ComboBox::backgroundColourId, juce::Colour::fromRGBA(17, 20, 24, 200));
                combo->setColour(juce::ComboBox::outlineColourId, juce::Colours::transparentBlack);
                combo->setColour(juce::ComboBox::textColourId, juce::Colours::white);
                combo->setColour(juce::ComboBox::arrowColourId, juce::Colours::orange.withBrightness(0.95f));
                combo->addItemList(descriptor.enumNames, 1);
                addAndMakeVisible(*combo);
                comboAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
                    apvts,
                    descriptor.paramId,
                    *combo));
                comboBoxes.push_back(std::move(combo));
                sliders.push_back(nullptr);
            }
            else
            {
                auto slider = std::make_unique<juce::Slider>();
                slider->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
                slider->setTextBoxStyle(juce::Slider::TextBoxBelow, false, 92, 24);
                slider->setColour(juce::Slider::rotarySliderFillColourId, juce::Colours::orange.withBrightness(0.92f));
                slider->setColour(juce::Slider::trackColourId, juce::Colours::black.withAlpha(0.4f));
                slider->setColour(juce::Slider::thumbColourId, juce::Colours::white);
                addAndMakeVisible(*slider);
                sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                    apvts,
                    descriptor.paramId,
                    *slider));
                sliders.push_back(std::move(slider));
                comboBoxes.push_back(nullptr);
            }
        }
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(12);
        const auto totalCards = static_cast<int>(labels.size());
        const auto columns = juce::jmax(1, area.getWidth() / (kCardWidth + 12));

        for (int index = 0; index < totalCards; ++index)
        {
            const int row = index / columns;
            const int column = index % columns;
            auto bounds = juce::Rectangle<int>(
                area.getX() + column * (kCardWidth + 12),
                area.getY() + row * (kCardHeight + 12),
                kCardWidth,
                kCardHeight);

            labels[static_cast<size_t>(index)]->setBounds(bounds.removeFromTop(26));

            if (sliders[static_cast<size_t>(index)] != nullptr)
            {
                sliders[static_cast<size_t>(index)]->setBounds(bounds.reduced(6));
            }
            else if (comboBoxes[static_cast<size_t>(index)] != nullptr)
            {
                comboBoxes[static_cast<size_t>(index)]->setBounds(bounds.removeFromTop(34).reduced(10, 0));
            }
        }
    }

private:
    std::vector<std::unique_ptr<juce::Label>> labels;
    std::vector<std::unique_ptr<juce::Slider>> sliders;
    std::vector<std::unique_ptr<juce::ComboBox>> comboBoxes;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> sliderAttachments;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> comboAttachments;
};

MrgeeJsfxAudioProcessorEditor::MrgeeJsfxAudioProcessorEditor(MrgeeJsfxAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    setSize(960, 620);

    titleLabel.setText(JucePlugin_Name, juce::dontSendNotification);
    titleLabel.setJustificationType(juce::Justification::centredLeft);
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.96f));
    titleLabel.setFont(juce::FontOptions(28.0f, juce::Font::bold));
    addAndMakeVisible(titleLabel);

    subtitleLabel.setText("JSFX-backed audio processor", juce::dontSendNotification);
    subtitleLabel.setJustificationType(juce::Justification::centredLeft);
    subtitleLabel.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.70f));
    addAndMakeVisible(subtitleLabel);

    statusLabel.setJustificationType(juce::Justification::centredLeft);
    statusLabel.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.82f));
    addAndMakeVisible(statusLabel);

    controlsComponent = std::make_unique<ControlsComponent>(audioProcessor);
    addAndMakeVisible(*controlsComponent);

    refreshStatus();
    startTimerHz(2);
}

void MrgeeJsfxAudioProcessorEditor::paint(juce::Graphics& g)
{
    juce::ColourGradient gradient(
        juce::Colour::fromRGB(18, 20, 28), 0.0f, 0.0f,
        juce::Colour::fromRGB(21, 55, 64), 0.0f, static_cast<float>(getHeight()), false);
    g.setGradientFill(gradient);
    g.fillAll();

    g.setColour(juce::Colours::black.withAlpha(0.18f));
    if (controlsComponent != nullptr)
        g.fillRoundedRectangle(controlsComponent->getBounds().toFloat(), 18.0f);
}

void MrgeeJsfxAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(24);

    titleLabel.setBounds(area.removeFromTop(34));
    subtitleLabel.setBounds(area.removeFromTop(24));
    statusLabel.setBounds(area.removeFromTop(24));
    area.removeFromTop(12);

    if (controlsComponent != nullptr)
    {
        controlsComponent->setBounds(area);
    }
}

void MrgeeJsfxAudioProcessorEditor::timerCallback()
{
    refreshStatus();
}

void MrgeeJsfxAudioProcessorEditor::refreshStatus()
{
    statusLabel.setText(audioProcessor.getStatusMessage(), juce::dontSendNotification);
}
