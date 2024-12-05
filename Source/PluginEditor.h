#pragma once

#include "PluginProcessor.h"

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
          parameterName(rap.name),
          suffix(unitSuffix)
    {
        setLookAndFeel(&lnf);
    }

    ~CustomRotarySlider() override
    {
        setLookAndFeel(nullptr);
    }

    juce::Array<juce::String> labels;

    void paint(juce::Graphics& g) override;
    juce::Rectangle<int> getSliderBounds() const;
    int getTextHeight() const { return 14; }
    juce::String getDisplayString() const;
    const juce::String& getParameterID() const { return parameterID; }

private:
    LookAndFeel lnf;
    juce::RangedAudioParameter* param;
    juce::String parameterID;
    juce::String parameterName;
    juce::String suffix;
};


//==============================================================================
struct DelayBar : juce::Component
{
    DelayBar(GranularDelayAudioProcessor& processor)
        : processorRef(processor) {}

    void paint(juce::Graphics& g) override;

    private:
        GranularDelayAudioProcessor& processorRef;
};


//==============================================================================
class GranularDelayAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                                public juce::AudioProcessorValueTreeState::Listener
{
public:
    explicit GranularDelayAudioProcessorEditor (GranularDelayAudioProcessor&);
    ~GranularDelayAudioProcessorEditor() override;

    //==============================================================================
    void parameterChanged(const juce::String& parameterID, float newValue) override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    GranularDelayAudioProcessor& processorRef;

    // Create text components
    juce::Label title;

    // Create delay bar
    DelayBar delayBar;

    // Create sliders
    CustomRotarySlider inputGainSlider,
                       delayTimeSlider,
                       feedbackSlider,
                       mixSlider,
                       outputGainSlider;

    // Function to get a vector of all components
    std::vector<juce::Component*> getComps();

    // Function to get a vector of slider components
    std::vector<juce::Component*> getSliders();

    // Type aliasing for readability
    using APVTS = juce::AudioProcessorValueTreeState;
    using Attachment = APVTS::SliderAttachment;

    // Create control Attachments 
    Attachment inputGainSliderAttachment,
               delayTimeSliderAttachment,
               feedbackSliderAttachment,
               mixSliderAttachment,
               outputGainSliderAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GranularDelayAudioProcessorEditor)
};
