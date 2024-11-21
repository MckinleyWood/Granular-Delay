#pragma once

#include "PluginProcessor.h"

// Look and feel method here... (2:17:20 in video)
struct LookAndFeel : juce::LookAndFeel_V4
{
    void drawRotarySlider (juce::Graphics&,
                           int x, int y, int width, int height,
                           float sliderPosProportional,
                           float rotaryStartAngle,
                           float rotaryEndAngle,
                           juce::Slider&) override;
};

struct CustomRotarySlider : juce::Slider
{
    CustomRotarySlider(juce::RangedAudioParameter& rap, const juce::String& unitSuffix) 
        : juce::Slider(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag,
                       juce::Slider::TextEntryBoxPosition::NoTextBox),
          param(&rap),
          parameterID(rap.paramID),
          suffix(unitSuffix)
    {
        setLookAndFeel(&lnf);
    }

    ~CustomRotarySlider() override
    {
        setLookAndFeel(nullptr);
    }

    struct LabelPos
    {
        float pos;
        juce::String label;
    };

    juce::Array<LabelPos> labels;

    void paint(juce::Graphics& g) override;
    juce::Rectangle<int> getSliderBounds() const;
    int getTextHeight() const { return 14; }
    juce::String getDisplayString() const;
    const juce::String& getParameterID() const { return parameterID; }

private:
    LookAndFeel lnf;
    juce::RangedAudioParameter* param;
    juce::String suffix;
    juce::String parameterID;
};

//==============================================================================
class GranularDelayAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit GranularDelayAudioProcessorEditor (GranularDelayAudioProcessor&);
    ~GranularDelayAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    GranularDelayAudioProcessor& processorRef;

    // Create control components
    CustomRotarySlider gainSlider,
                       delayTimeSlider,
                       feedbackSlider;

    // Function to get a vector of all components
    std::vector<juce::Component*> getComps();

    // Type aliasing for readability
    using APVTS = juce::AudioProcessorValueTreeState;
    using Attachment = APVTS::SliderAttachment;

    // Create control Attachments 
    Attachment gainSliderAttachment,
               delayTimeSliderAttachment,
               feedbackSliderAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GranularDelayAudioProcessorEditor)
};
