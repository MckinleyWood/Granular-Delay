#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
GranularDelayAudioProcessor::GranularDelayAudioProcessor()
    : AudioProcessor (BusesProperties()
                       .withInput  ("Input",  juce::AudioChannelSet::stereo())
                       .withOutput ("Output", juce::AudioChannelSet::stereo())),
        apvts(*this, nullptr, "Parameters", createParameterLayout())
{
}

GranularDelayAudioProcessor::~GranularDelayAudioProcessor()
{
}

//==============================================================================
const juce::String GranularDelayAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool GranularDelayAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool GranularDelayAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool GranularDelayAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double GranularDelayAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int GranularDelayAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int GranularDelayAudioProcessor::getCurrentProgram()
{
    return 0;
}

void GranularDelayAudioProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String GranularDelayAudioProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void GranularDelayAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

//==============================================================================
void GranularDelayAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    juce::ignoreUnused (sampleRate, samplesPerBlock);
}

void GranularDelayAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

bool GranularDelayAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}

void GranularDelayAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                                juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);

    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // This is the place where you'd normally do the guts of your plugin's
    // audio processing...
    // Make sure to reset the state if your inner loop is processing
    // the samples and the outer loop is handling the channels.
    // Alternatively, you can process the samples with the channels
    // interleaved by keeping the same state.
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer (channel);
        juce::ignoreUnused (channelData);

        // Gets gain parameter and applies it to each sample
        float currentGain = apvts.getRawParameterValue("gain")->load();
        buffer.applyGain(currentGain);
    }
}

//==============================================================================
bool GranularDelayAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* GranularDelayAudioProcessor::createEditor()
{
    return new GranularDelayAudioProcessorEditor (*this); // Use custom editor
    //return new juce::GenericAudioProcessorEditor (*this); // Use default editor
}

//==============================================================================
void GranularDelayAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::ignoreUnused (destData);

    // Write parameters to memory block
    juce::MemoryOutputStream mos(destData, true);
    apvts.state.writeToStream(mos);
}

void GranularDelayAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    juce::ignoreUnused (data, sizeInBytes);

    // Restore parameters from memory block
    auto tree = juce::ValueTree::readFromData(data, sizeInBytes);
    if (tree.isValid()) 
    {
        apvts.replaceState(tree);
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout
    GranularDelayAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // Add parameters to the APVTS layout
    layout.add(std::make_unique<juce::AudioParameterFloat>("gain", "Gain", 0.f, 2.f, 1.f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("delayTime", "Delay Time", 
                                juce::NormalisableRange<float>(10.f, 10000.f, 0.f, 0.5f), 1000.f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("feedback", "Feedback", 0.f, 1.f, 0.5f));

    return layout;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new GranularDelayAudioProcessor();
}

