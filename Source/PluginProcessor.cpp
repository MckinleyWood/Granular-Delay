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
    juce::ignoreUnused (sampleRate, samplesPerBlock);

    juce::dsp::ProcessSpec spec;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = 1;
    spec.sampleRate = sampleRate;

    auto chainSettings = getChainSettings(apvts);

    float gain = chainSettings.gain;
    float delayTimeInSamples = static_cast<int>((chainSettings.delayTime / 1000.0) * sampleRate);
    float feedback = chainSettings.feedback;

    auto delayBufferSize = sampleRate * 10.0;
    delayBuffer.setSize(getTotalNumOutputChannels(), (int)delayBufferSize);
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
    juce::ignoreUnused(midiMessages);

    juce::ScopedNoDenormals noDenormals;

    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels  = getTotalNumOutputChannels();
    auto mainBufferSize = buffer.getNumSamples();
    auto delayBufferSize = delayBuffer.getNumSamples();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    // Get parameter values
    auto chainSettings = getChainSettings(apvts);
    float gain = chainSettings.gain;
    int delayTimeInSamples = static_cast<int>((chainSettings.delayTime / 1000.0) * getSampleRate());
    float feedback = chainSettings.feedback;

    readPosition  = (writePosition - delayTimeInSamples + delayBufferSize) % delayBufferSize;

    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer(channel);

        fillDelayBuffer(channelData, channel, mainBufferSize, delayBufferSize);
        readFromDelayBuffer(buffer, delayBuffer, channel, mainBufferSize, delayBufferSize);
    }

    writePosition += mainBufferSize;
    writePosition %= delayBufferSize;
}

void GranularDelayAudioProcessor::fillDelayBuffer(float* channelData, int channel, 
                                                  int mainBufferSize, int delayBufferSize)
{
    // Check if there is enough room for full buffer in delayBuffer
    if (delayBufferSize > mainBufferSize + writePosition)
    {
        // If so, copy entire buffer over
        delayBuffer.copyFromWithRamp(channel, writePosition, channelData, mainBufferSize, 1.f, 1.f);
    }
    else
    {
        // Determine how much space is left at the end / how much to put at start
        auto numSamplesToEnd = delayBufferSize - writePosition;
        auto numSamplesLeft = mainBufferSize - numSamplesToEnd;

        // Copy samples to end / start
        delayBuffer.copyFromWithRamp(channel, writePosition, channelData, numSamplesToEnd, 1.f, 1.f);
        delayBuffer.copyFromWithRamp(channel, 0, channelData + numSamplesToEnd, numSamplesLeft, 1.f, 1.f);
    }    
}

void GranularDelayAudioProcessor::readFromDelayBuffer(juce::AudioBuffer<float>& buffer,
                                                      juce::AudioBuffer<float>& delayBuffer, int channel, 
                                                      int mainBufferSize, int delayBufferSize)
{
    // Check if there are enough samples in delayBuffer left to fill the main buffer
    if (delayBufferSize > mainBufferSize + readPosition)
    {
        // If so, copy mainBufferSize samples
        buffer.addFromWithRamp(channel, 0, delayBuffer.getReadPointer(channel, readPosition), 
                                mainBufferSize, 1.f, 1.f);
    }
    else
    {
        // Determine how many samples are left at the end / how much to get from the start
        auto numSamplesToEnd = delayBufferSize - readPosition;
        auto numSamplesLeft = mainBufferSize - numSamplesToEnd;

        // Copy samples from end / start or delayBuffer
        buffer.addFromWithRamp(channel, 0, delayBuffer.getReadPointer(channel, readPosition), 
                                numSamplesToEnd, 1.f, 1.f);
        buffer.addFromWithRamp(channel, numSamplesToEnd, 
                                delayBuffer.getReadPointer(channel, 0), 
                                numSamplesLeft, 1.f, 1.f);
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

ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& apvts)
{
    ChainSettings settings;

    settings.gain = apvts.getRawParameterValue("gain")->load();
    settings.delayTime = apvts.getRawParameterValue("delayTime")->load();
    settings.feedback = apvts.getRawParameterValue("feedback")->load();

    return settings;
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

