#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
void LookAndFeel::drawRotarySlider(juce::Graphics &g, int x, int y, int width, int height,
                                   float sliderPosProportional, float rotaryStartAngle,
                                   float rotaryEndAngle, juce::Slider &slider)
{
    using namespace juce;

    auto bounds = Rectangle<float>(x, y, width, height);

    // Colour the circle and border
    g.setColour(Colour(70u, 75u, 80u));
    g.drawEllipse(bounds, width * 0.05f);

    if (auto* crs = dynamic_cast<CustomRotarySlider*>(&slider))
    {
        auto centre = bounds.getCentre();

        // Create the bar
        Path p;
        Rectangle<float> r = bounds.withTrimmedBottom(bounds.getHeight() * 0.75f)
                                   .withTrimmedLeft(bounds.getWidth() * 0.475f)
                                   .withTrimmedRight(bounds.getWidth() * 0.475f)
                                   .translated(0.f, -(bounds.getHeight() * 0.05f));
                
        p.addRoundedRectangle(r, 2.f);

        // Rotate the bar
        jassert(rotaryStartAngle < rotaryEndAngle);
        auto sliderAngleRadians = jmap(sliderPosProportional, 0.f, 1.f, rotaryStartAngle, rotaryEndAngle);
        p.applyTransform(AffineTransform().rotated(sliderAngleRadians, centre.getX(), centre.getY()));

        // Fill the bar
        g.fillPath(p);
    }
}

void CustomRotarySlider::paint(juce::Graphics &g)
{
    using namespace juce;

    auto startAngle = degreesToRadians(180.f + 45.f);
    auto endAngle = degreesToRadians(180.f - 45.f) + MathConstants<float>::twoPi;
    auto range = getRange();
    auto sliderBounds = getSliderBounds();
    auto compBounds = getLocalBounds();
    auto centre = sliderBounds.getCentre();
    Rectangle<int> r;

    g.setFont(compBounds.getHeight() * 0.125f);

    // Paint the circle and bar
    getLookAndFeel().drawRotarySlider(g, sliderBounds.getX(), sliderBounds.getY(),
                                      sliderBounds.getWidth(), sliderBounds.getHeight(),
                                      (float)jmap(getValue(), range.getStart(), range.getEnd(), 0., 1.), 
                                      startAngle, endAngle, *this);

    // Paint the centre text box
    g.setFont(getTextHeight());
    auto text = getDisplayString();
    auto strWidth = g.getCurrentFont().getStringWidth(text);

    r.setSize(strWidth + 2, getTextHeight() + 1);
    r.setCentre(centre);

    g.setColour(Colours::white);
    g.drawFittedText(text, r.toNearestInt(), Justification::centred, 1);

    // Paint the slider title
    g.setFont(getTextHeight());

    auto str = parameterName;
    r.setSize(g.getCurrentFont().getStringWidth(str), getTextHeight());
    r.setCentre(centre);
    r.setY(sliderBounds.getY() - (getTextHeight() + 5));
    g.drawFittedText(str, r.toNearestInt(), Justification::centred, 1);
}

// Returns the bounds of the visible slider within the slider component
juce::Rectangle<int> CustomRotarySlider::getSliderBounds() const
{
    // Small sliders
    // return getLocalBounds().withTrimmedTop(static_cast<int>(getHeight() * 0.375f))
    //                        .withTrimmedBottom(static_cast<int>(getHeight() * 0.125f))
    //                        .withTrimmedLeft(static_cast<int>(getWidth() * 0.25f))
    //                        .withTrimmedRight(static_cast<int>(getWidth() * 0.25f));

    // Big sliders
    return getLocalBounds().withTrimmedTop(getHeight() / 4)
                           .withTrimmedBottom(getHeight() / 12)
                           .withTrimmedLeft(getWidth() / 6)
                           .withTrimmedRight(getWidth() / 6);
}

juce::String CustomRotarySlider::getDisplayString() const
{
    juce::String str;

    if (auto* floatParam = dynamic_cast<juce::AudioParameterFloat*>(param))
    {
        float val = (float)getValue();
        juce::String id = getParameterID();

        if (suffix == "ms")
        {
            if (val >= 1000.f)
            {
                val /= 1000.f;
                str = juce::String(val, 2) + " s";
            } 
            else
            {
                str = juce::String(val, 2) + " ms";
            }
        } 
        else if (suffix == "%")
        {
            val *= 100.f;
            str = juce::String(val, 0) + " " + suffix;
        }
        else
        {
            str = juce::String(val, 2);
            if (suffix.isNotEmpty())
                str += " " + suffix;
        }
    }
    else 
    {
        jassertfalse;
    }

    return str;
}


//==============================================================================
// void DelayBar::paint(juce::Graphics &g)
// {
//     // Calculate delay position in samples
//     auto mixMs = processorRef.apvts.getRawParameterValue("mix")->load();
//     auto grainSize = processorRef.apvts.getRawParameterValue("grainSize")->load();

//     auto widthSamples = static_cast<float>(10 * processorRef.getSampleRate());
//     auto delaySamples = static_cast<float>(mixMs / 1000.f * processorRef.getSampleRate());

//     auto widthPixels = static_cast<float>(getWidth());
//     auto delayPixels = delaySamples / widthSamples * widthPixels;

//     auto barPosition = widthPixels - delayPixels;
//     auto opacity = grainSize;

//     g.setColour(juce::Colours::white);
//     auto bounds = getLocalBounds();
//     auto r = juce::Rectangle<float>(2.f, bounds.getHeight());

//     while (barPosition > 0 && opacity > 0.01f)
//     {
//         g.setOpacity(opacity);
//         r.setCentre(barPosition, bounds.getCentreY());
//         g.fillRoundedRectangle(r, 1.f);

//         barPosition -= delayPixels;
//         opacity *= grainSize;
//     }
// }


//==============================================================================
void RangeVisualiser::paint(juce::Graphics &g)
{
    jassert(rangeStartMs <= rangeEndMs);
    g.fillAll(juce::Colours::transparentBlack);

    auto bounds = getLocalBounds();
    float startXProportional = 1.f - juce::jmap(rangeStartMs, 0.f, 10000.f, 0.f, 1.f);
    float endXProportional = 1.f - juce::jmap(rangeEndMs, 0.f, 10000.f, 0.f, 1.f);
    float startX = startXProportional * bounds.getWidth();
    float endX = endXProportional * bounds.getWidth();
    juce::Rectangle<float> range(endX, 0.f, startX - endX, bounds.getHeight());
    if (range.getWidth() < 4.f)
        range.setWidth(4.f);
    
    g.setColour(juce::Colours::white.withAlpha(0.5f));
    g.fillRoundedRectangle(range, 2.f);
}


//==============================================================================
// Editor constructor!
GranularDelayAudioProcessorEditor::GranularDelayAudioProcessorEditor (GranularDelayAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p), 
    rangeVisualizer(*processorRef.apvts.getRawParameterValue("rangeStart"),
                    *processorRef.apvts.getRawParameterValue("rangeEnd")),

    inputGainSlider(*processorRef.apvts.getParameter("inputGain"), "%"),
    mixSlider(*processorRef.apvts.getParameter("mix"), "%"),
    grainSizeSlider(*processorRef.apvts.getParameter("grainSize"), "ms"),
    frequencySlider(*processorRef.apvts.getParameter("frequency"), "/s"),
    rangeStartSlider(*processorRef.apvts.getParameter("rangeStart"), "ms"),
    rangeEndSlider(*processorRef.apvts.getParameter("rangeEnd"), "ms"),
    pitchSlider(*processorRef.apvts.getParameter("grainPitch"), "x"),
    dummy2Slider(*processorRef.apvts.getParameter("dummy2"), ""),
    detuneSlider(*processorRef.apvts.getParameter("detune"), "c"),
    dummy4Slider(*processorRef.apvts.getParameter("dummy4"), ""),

    inputGainSliderAttachment(processorRef.apvts, "inputGain", inputGainSlider),
    mixSliderAttachment(processorRef.apvts, "mix", mixSlider),
    grainSizeSliderAttachment(processorRef.apvts, "grainSize", grainSizeSlider),
    frequencySliderAttachment(processorRef.apvts, "frequency", frequencySlider),
    rangeStartSliderAttachment(processorRef.apvts, "rangeStart", rangeStartSlider),
    rangeEndSliderAttachment(processorRef.apvts, "rangeEnd", rangeEndSlider),
    pitchSliderAttachment(processorRef.apvts, "grainPitch", pitchSlider),
    dummy2SliderAttachment(processorRef.apvts, "dummy2", dummy2Slider),
    detuneSliderAttachment(processorRef.apvts, "detune", detuneSlider),
    dummy4SliderAttachment(processorRef.apvts, "dummy4", dummy4Slider)
{
    processorRef.apvts.addParameterListener("rangeStart", this);
    processorRef.apvts.addParameterListener("rangeEnd", this);

    grainSizeSlider.onValueChange = [this]()
    {
        auto grainSize = grainSizeSlider.getValue();
        auto rangeStart = rangeStartSlider.getValue();

        if (grainSize > rangeStart) // Enforce constraints
            grainSizeSlider.setValue(rangeStart);
    };

    rangeStartSlider.onValueChange = [this]()
    {
        auto grainSize = grainSizeSlider.getValue();
        auto rangeStart = rangeStartSlider.getValue();
        auto rangeEnd = rangeEndSlider.getValue();

        if (rangeStart < grainSize)
            rangeStartSlider.setValue(grainSize); 

        if (rangeStart > rangeEnd)
            rangeStartSlider.setValue(rangeEnd);
    };

    rangeEndSlider.onValueChange = [this]()
    {
        auto rangeStart = rangeStartSlider.getValue();
        auto rangeEnd = rangeEndSlider.getValue();

        if (rangeEnd < rangeStart)
            rangeEndSlider.setValue(rangeStart);
    };


    // Make all components visible
    for(auto* comp : getComps())
    {
        addAndMakeVisible(comp);
    }

    // Set the size of the editor
    setSize (620, 420);
}

GranularDelayAudioProcessorEditor::~GranularDelayAudioProcessorEditor()
{
    processorRef.apvts.removeParameterListener("rangeStart", this);
    processorRef.apvts.removeParameterListener("rangeEnd", this);
}

//==============================================================================
void GranularDelayAudioProcessorEditor::parameterChanged(const juce::String& parameterID, float newValue)
{
    if (parameterID == "rangeStart")
    {
        rangeVisualizer.setStart(newValue);
        rangeVisualizer.repaint();  // Repaint to show the new range
    }
    else if (parameterID == "rangeEnd")
    {
        rangeVisualizer.setEnd(newValue);
        rangeVisualizer.repaint();
    }
}

void GranularDelayAudioProcessorEditor::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().reduced(10, 10);

    // Paint the background
    g.fillAll (juce::Colours::black);

    // Paint the title
    g.setFont(bounds.getHeight() * 0.06f);
    title.setText("Granular Delay", juce::dontSendNotification);
    title.setFont(g.getCurrentFont());
    title.setJustificationType(juce::Justification::centred);
    title.setColour(juce::Label::textColourId, juce::Colours::white);

    // Paint the waveViewer
    processorRef.waveViewer.setColours(juce::Colours::black, juce::Colour(70u, 75u, 80u));

    // Paint boxes around all components (for testing)
    // g.setColour(juce::Colours::yellow);
    // for (const auto& comp : getComps())
    // {
    //     g.drawRect(comp->getBounds());
    // }
}

// This is where we set the boundaries of the components
void GranularDelayAudioProcessorEditor::resized()
{
    // Partition the editor into zones
    auto bounds = getLocalBounds();

    auto titleZone = bounds.withTrimmedBottom(static_cast<int>(bounds.getHeight() * 0.9f));

    bounds.reduce(10, 10);
    
    auto waveViewerZone = bounds.withTrimmedTop(static_cast<int>(bounds.getHeight() * 0.1f))
                                .withTrimmedBottom(static_cast<int>(bounds.getHeight() * 0.6f));

    auto sliderZone = bounds.withTrimmedTop(static_cast<int>(bounds.getHeight() * 0.4f));

    std::vector<juce::Rectangle<int>> sliderBoxes;
    
    auto topRow = sliderZone.withTrimmedBottom(static_cast<int>(sliderZone.getHeight() * 0.5f));
    auto bottomRow = sliderZone.withTrimmedTop(static_cast<int>(sliderZone.getHeight() * 0.5f));
    
    for (int i = 0; i < 5; ++i)
    {
        sliderBoxes.push_back(topRow.withTrimmedLeft(static_cast<int>(topRow.getWidth() * 0.2f * i))
                                    .withTrimmedRight(static_cast<int>(topRow.getWidth() * 0.2f * (4 - i))));
    }

    for (int i = 0; i < 5; ++i)
    {
        sliderBoxes.push_back(bottomRow.withTrimmedLeft(static_cast<int>(bottomRow.getWidth() * 0.2f * i))
                                       .withTrimmedRight(static_cast<int>(bottomRow.getWidth() * 0.2f * (4 - i))));
    }

    // Set the bounds of the components
    title.setBounds(titleZone);
    processorRef.waveViewer.setBounds(waveViewerZone);
    rangeVisualizer.setBounds(waveViewerZone);

    auto sliders = getSliders();
    for(size_t i = 0; i < sliders.size(); ++i)
    {
        sliders[i]->setBounds(sliderBoxes[i]);
    }
}

// Returns a vector containing all of our components
std::vector<juce::Component*> GranularDelayAudioProcessorEditor::getComps()
{
    return {&title,
            &processorRef.waveViewer,
            &rangeVisualizer,
            &inputGainSlider,
            &frequencySlider,
            &grainSizeSlider,
            &mixSlider,
            &rangeStartSlider,
            &rangeEndSlider,
            &pitchSlider,
            &dummy2Slider,
            &detuneSlider,
            &dummy4Slider
            };
}

// Returns a vector containing just the slider components (in the correct order for drawing)
std::vector<juce::Component*> GranularDelayAudioProcessorEditor::getSliders()
{
    return {&inputGainSlider,
            &rangeStartSlider,
            &grainSizeSlider,
            &pitchSlider,
            &dummy2Slider,
            &mixSlider,
            &rangeEndSlider,
            &frequencySlider,
            &detuneSlider,
            &dummy4Slider
            };
}