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

    auto delayBufferSize = sampleRate * 10.0;
    delayBuffer.setSize(getTotalNumOutputChannels(), (int)delayBufferSize);
    tempBuffer.setSize(getTotalNumInputChannels(), samplesPerBlock);
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

    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    auto mainBufferSize = buffer.getNumSamples();
    auto delayBufferSize = delayBuffer.getNumSamples();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    // Get parameter values
    auto chainSettings = getChainSettings(apvts);
    float gain = chainSettings.gain;
    int delayTimeInSamples = static_cast<int>((chainSettings.delayTime / 1000.0) * getSampleRate());
    float feedback = chainSettings.feedback;
    float mix = chainSettings.mix;

    readPosition  = (writePosition - delayTimeInSamples + delayBufferSize) % delayBufferSize;

    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {        
        // Copy the input signal to a temporary buffer
        tempBuffer.copyFrom(channel, 0, buffer.getWritePointer(channel), mainBufferSize);

        // Add delayed signal (from readPosition) to temporary buffer
        readFromDelayBuffer(tempBuffer, channel, 1.f);

        // Mix echoes with dry signal 
        buffer.applyGain(channel, 0, mainBufferSize, 1.f - mix);
        readFromDelayBuffer(buffer, channel, mix);

        // Copy the summed signal back to delayBuffer
        fillDelayBuffer(tempBuffer, channel, feedback);
    }

    updateWritePosition(buffer);

    buffer.applyGain(gain);
}

void GranularDelayAudioProcessor::fillDelayBuffer(juce::AudioBuffer<float>& buffer, int channel, float gain)
{   
    auto bufferSize = buffer.getNumSamples();
    auto delayBufferSize = delayBuffer.getNumSamples();

    // Check if there is enough room for full buffer in delayBuffer
    if (delayBufferSize > bufferSize + writePosition)
    {
        // If so, copy entire buffer over
        delayBuffer.copyFrom(channel, writePosition, buffer.getWritePointer(channel), bufferSize, gain);
    }
    else
    {
        // Determine how much space is left at the end / how much to put at start
        auto numSamplesToEnd = delayBufferSize - writePosition;
        auto numSamplesLeft = bufferSize - numSamplesToEnd;

        // Copy samples to end / start
        delayBuffer.copyFrom(channel, writePosition, buffer.getWritePointer(channel), numSamplesToEnd, gain);
        delayBuffer.copyFrom(channel, 0, buffer.getWritePointer(channel, numSamplesToEnd), numSamplesLeft, gain);
    }    
}

void GranularDelayAudioProcessor::readFromDelayBuffer(juce::AudioBuffer<float>& buffer, int channel, float gain)
{
    auto mainBufferSize = buffer.getNumSamples();
    auto delayBufferSize = delayBuffer.getNumSamples();

    // Check if there are enough samples in delayBuffer left to fill the main buffer
    if (delayBufferSize > mainBufferSize + readPosition)
    {
        // If so, add mainBufferSize samples
        buffer.addFrom(channel, 0, delayBuffer.getReadPointer(channel, readPosition), mainBufferSize, gain);
    }
    else
    {
        // Determine how many samples are left at the end / how much to get from the start
        auto numSamplesToEnd = delayBufferSize - readPosition;
        auto numSamplesLeft = mainBufferSize - numSamplesToEnd;

        // Add samples from end / start of delayBuffer
        buffer.addFrom(channel, 0, delayBuffer.getReadPointer(channel, readPosition), numSamplesToEnd, gain);
        buffer.addFrom(channel, numSamplesToEnd, delayBuffer.getReadPointer(channel, 0), numSamplesLeft, gain);
    }
}

void GranularDelayAudioProcessor::updateWritePosition(juce::AudioBuffer<float>& buffer)
{
    writePosition += buffer.getNumSamples();
    writePosition %= delayBuffer.getNumSamples();
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
    settings.mix = apvts.getRawParameterValue("mix")->load();

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
    layout.add(std::make_unique<juce::AudioParameterFloat>("mix", "Mix", 0.f, 1.f, 0.5f));
    return layout;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new GranularDelayAudioProcessor();
}

