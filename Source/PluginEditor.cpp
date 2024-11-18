#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
// Editor constructor!
GranularDelayAudioProcessorEditor::GranularDelayAudioProcessorEditor (GranularDelayAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    juce::ignoreUnused (processorRef);
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (400, 300);

    // these define the parameters of the gain slider
    gain.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    gain.setRange (0.0, 1.0);
    gain.setTextBoxStyle (juce::Slider::NoTextBox, false, 90, 0);
    gain.setPopupDisplayEnabled (true, false, this);
    //gain.setTextValueSuffix (" Volume");
    gain.setValue(0.0);
    addAndMakeVisible (&gain);
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

    gain.setBounds (100, 75, 200, 150);
}
