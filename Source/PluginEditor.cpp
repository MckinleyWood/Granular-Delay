#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
// Editor constructor!
GranularDelayAudioProcessorEditor::GranularDelayAudioProcessorEditor (GranularDelayAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p), 
    gainSliderAttachment(processorRef.apvts, "gain", gainSlider)
{
    juce::ignoreUnused (processorRef);
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.

    for(auto* comp : getComps())
    {
        addAndMakeVisible(comp);
    }

    setSize (400, 300);

    // these define the parameters of the gain slider
    //gainSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    //gainSlider.setRange (0.0, 1.0);
    //gainSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 90, 0);
    //gainSlider.setPopupDisplayEnabled (true, false, this);
    //gain.setTextValueSuffix (" Volume");
    //gainSlider.setValue (*processorRef.gain);
    //gainSlider.onValueChange = [this] { *processorRef.gain = gainSlider.getValue(); };
    //addAndMakeVisible (&gainSlider);

    // Create the attachment linking the slider to the "gain" parameter
    // gainAttachment = std::make_unique<juce::SliderParameterAttachment>(processorRef.apvts, 
    //                                                                    "gain", gainSlider);

}

GranularDelayAudioProcessorEditor::~GranularDelayAudioProcessorEditor()
{
}

//==============================================================================
void GranularDelayAudioProcessorEditor::paint (juce::Graphics& g)
{
    // fill the whole window black
    g.fillAll (juce::Colours::black);
 
    // set the current drawing colour to white
    g.setColour (juce::Colours::white);
 
    // set the font size and draw text to the screen
    g.setFont (15.0f);
    g.drawFittedText ("Gain", 0, 0, getWidth(), 30, juce::Justification::centred, 1);
}

void GranularDelayAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..

    gainSlider.setBounds(100, 75, 200, 150);
}

std::vector<juce::Component*> GranularDelayAudioProcessorEditor::getComps()
{
    return {&gainSlider};
}