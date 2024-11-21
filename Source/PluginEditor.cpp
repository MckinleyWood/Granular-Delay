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
    // g.setColour(Colour(70u, 35u, 100u));
    // g.fillEllipse(bounds);
    g.setColour(Colour(70u, 75u, 80u));
    g.drawEllipse(bounds, 4.f);

    if (auto* crs = dynamic_cast<CustomRotarySlider*>(&slider))
    {
        auto centre = bounds.getCentre();

        // Create the bar
        Path p;
        Rectangle<float> r;
        r.setLeft(centre.getX() - 2);
        r.setRight(centre.getX() + 2);
        r.setTop(bounds.getY());
        r.setBottom(centre.getY() - crs->getTextHeight() * 3.f);
        p.addRoundedRectangle(r, 2.f);

        // Rotate the bar
        jassert(rotaryStartAngle < rotaryEndAngle);
        auto sliderAngleRadians = jmap(sliderPosProportional, 0.f, 1.f, rotaryStartAngle, rotaryEndAngle);
        p.applyTransform(AffineTransform().rotated(sliderAngleRadians, centre.getX(), centre.getY()));

        // Fill the bar
        g.fillPath(p);

        // Draw the text box
        g.setFont(crs->getTextHeight());
        auto text = crs->getDisplayString();
        auto strWidth = g.getCurrentFont().getStringWidth(text);

        r.setSize(strWidth + 4, crs->getTextHeight() + 2);
        r.setCentre(centre);

        g.setColour(Colours::black);
        g.fillRect(r);

        g.setColour(Colours::white);
        g.drawFittedText(text, r.toNearestInt(), Justification::centred, 1);
    }
}

//==============================================================================

void CustomRotarySlider::paint(juce::Graphics &g)
{
    using namespace juce;

    auto startAngle = degreesToRadians(180.f + 45.f);
    auto endAngle = degreesToRadians(180.f - 45.f) + MathConstants<float>::twoPi;
    auto range = getRange();
    auto sliderBounds = getSliderBounds();

    /* 
    //// Draw boxes around 
    g.setColour(Colours::red);
    g.drawRect(getLocalBounds());
    g.setColour(Colours::yellow);
    g.drawRect(sliderBounds);
    */

    getLookAndFeel().drawRotarySlider(g, sliderBounds.getX(), sliderBounds.getY(),
                                      sliderBounds.getWidth(), sliderBounds.getHeight(),
                                      jmap(getValue(), range.getStart(), range.getEnd(), 0., 1.), 
                                      startAngle, endAngle, *this);

    auto centre = sliderBounds.toFloat().getCentre();
    auto radius = sliderBounds.getWidth() * 0.5f;

    // Paint the min/max labels
    g.setColour(Colours::white);
    g.setFont(getTextHeight());

    auto numLabels = labels.size();
    for (int i = 0; i < numLabels; i++)
    {
        auto pos = labels[i].pos;
        jassert(0.f <= pos && pos <= 1.f);

        auto angle = jmap(pos, 0.f, 1.f, startAngle, endAngle);

        auto c = centre.getPointOnCircumference(radius + getTextHeight() * 0.5f + 1.f, angle);

        Rectangle<float> r;
        auto str = labels[i].label;
        r.setSize(g.getCurrentFont().getStringWidth(str), getTextHeight());
        r.setCentre(c);
        r.setY(r.getY() + getTextHeight());

        g.drawFittedText(str, r.toNearestInt(), Justification::centred, 1);
    }
}

juce::Rectangle<int> CustomRotarySlider::getSliderBounds() const
{
    auto bounds = getLocalBounds();
    auto size = juce::jmin(bounds.getWidth(), bounds.getHeight());
    size -= getTextHeight() * 2;
    juce::Rectangle<int> r;
    r.setSize(size, size);
    r.setCentre(bounds.getCentre());

    return r;
}

juce::String CustomRotarySlider::getDisplayString() const
{
    juce::String str;

    if (auto* floatParam = dynamic_cast<juce::AudioParameterFloat*>(param))
    {
        float val = getValue();
        juce::String id = getParameterID();

        if (id == "delayTime")
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
// Editor constructor!
GranularDelayAudioProcessorEditor::GranularDelayAudioProcessorEditor (GranularDelayAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p), 

    gainSlider(*processorRef.apvts.getParameter("gain"), "%"),
    delayTimeSlider(*processorRef.apvts.getParameter("delayTime"), "ms"),
    feedbackSlider(*processorRef.apvts.getParameter("feedback"), "%"),

    gainSliderAttachment(processorRef.apvts, "gain", gainSlider),
    delayTimeSliderAttachment(processorRef.apvts, "delayTime", delayTimeSlider),
    feedbackSliderAttachment(processorRef.apvts, "feedback", feedbackSlider)
{
    juce::ignoreUnused (processorRef);

    // Add min/max labels
    gainSlider.labels.add({0.f, "0%"});
    gainSlider.labels.add({1.f, "200%"});
    delayTimeSlider.labels.add({0.f, "10ms"});
    delayTimeSlider.labels.add({1.f, "10s"});
    feedbackSlider.labels.add({0.f, "0%"});
    feedbackSlider.labels.add({1.f, "100%"});

    // Make all components visible
    for(auto* comp : getComps())
    {
        addAndMakeVisible(comp);
    }

    // Set the size of the editor
    setSize (600, 400);
}

GranularDelayAudioProcessorEditor::~GranularDelayAudioProcessorEditor()
{
}

//==============================================================================
void GranularDelayAudioProcessorEditor::paint (juce::Graphics& g)
{
    // Make it black
    g.fillAll (juce::Colours::black);
}

// This is where we set the boundaries of the components
void GranularDelayAudioProcessorEditor::resized()
{
    // Partition the editor into zones
    auto bounds = getLocalBounds();
    bounds.reduced(10);
    auto sliderZone = bounds.removeFromTop(bounds.getHeight() * 0.33);
    auto gainZone = bounds.removeFromLeft(bounds.getWidth() * 0.33);
    auto delayTimeZone = bounds.removeFromLeft(bounds.getWidth() * 0.5);
    auto feedbackZone = bounds;

    // Set the bounds of the sliders
    gainSlider.setBounds(gainZone);
    delayTimeSlider.setBounds(delayTimeZone);
    feedbackSlider.setBounds(feedbackZone);
}

// Returns a vector containg all of our components
std::vector<juce::Component*> GranularDelayAudioProcessorEditor::getComps()
{
    return {&gainSlider,
            &delayTimeSlider,
            &feedbackSlider};
}