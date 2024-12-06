#include "PluginProcessor.h"
#include "PluginEditor.h"


//==============================================================================
GranularDelayAudioProcessor::GranularDelayAudioProcessor()
    : AudioProcessor (BusesProperties()
                      .withInput  ("Input",  juce::AudioChannelSet::stereo())
                      .withOutput ("Output", juce::AudioChannelSet::stereo())),
                      apvts(*this, nullptr, "Parameters", createParameterLayout()),
                      waveViewer(1), timer([this]() { timerCallback(); })
{
    waveViewer.setRepaintRate(60);
    waveViewer.setBufferSize(1024);
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

    auto delayBufferSize = static_cast<int>(sampleRate * 10);
    delayBuffer.setSize(getTotalNumOutputChannels(), delayBufferSize);
    wetBuffer.setSize(getTotalNumInputChannels(), samplesPerBlock);

    waveViewer.setSamplesPerBlock(delayBufferSize / 1024);
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

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    // Get parameter values
    auto chainSettings = getChainSettings(apvts);
    float inputGain = chainSettings.inputGain;
    float mix = chainSettings.mix;

    if (grainRequested.load())
    {
        addGrain();
        grainRequested.store(false);
    }

    wetBuffer.clear();
    buffer.applyGain(inputGain);

    // Give the buffer to the wave viewer 
    waveViewer.pushBuffer(buffer);

    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {        
        // Copy the input buffer into the delayBuffer
        fillDelayBuffer(buffer, channel, 1.f);

        // Read from the grains into the wetBuffer
        if (!grainVector.empty())
        {
            for (size_t i = grainVector.size() - 1; i >= 0; --i)
            {
                int grainBufferSize = grainVector[i].buffer.getNumSamples();
                int readPosition = grainVector[i].readPosition;

                int samplesToCopy = juce::jmin(grainBufferSize - readPosition, mainBufferSize);
                wetBuffer.addFrom(channel, 0, grainVector[i].buffer, channel, readPosition, samplesToCopy, 0.7f);

                if (grainBufferSize - readPosition > mainBufferSize)
                {
                    grainVector[i].readPosition += mainBufferSize;
                }    
                else
                {
                    grainVector.erase(grainVector.begin() + i);
                }
            }
        }

        // Mix echoes with dry signal 
        buffer.applyGain(channel, 0, mainBufferSize, 1.f - mix);
        buffer.addFrom(channel, 0, wetBuffer, channel, 0, mainBufferSize, mix);
    }

    updateWritePosition(mainBufferSize);
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

void GranularDelayAudioProcessor::readFromDelayBuffer(juce::AudioBuffer<float>& buffer, int channel, int readPosition, float gain)
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

void GranularDelayAudioProcessor::updateWritePosition(int numSamples)
{
    writePosition += numSamples;
    writePosition %= delayBuffer.getNumSamples();
}

// Adds a new grain to the grainVector, taking samples from the delayBuffer
void GranularDelayAudioProcessor::addGrain()
{
    int sampleRate = static_cast<int>(getSampleRate());
    auto chainSettings = getChainSettings(apvts);
    float grainSize = chainSettings.grainSize;
    float rangeStart = chainSettings.rangeStart;
    float rangeEnd = chainSettings.rangeEnd;

    int startSamples = static_cast<int>(rangeStart * sampleRate / 1000);
    int endSamples = static_cast<int>(rangeEnd * sampleRate / 1000);
    int grainSizeSamples = static_cast<int>(grainSize * sampleRate / 1000);

    int minStartSample = (writePosition - startSamples) % delayBuffer.getNumSamples();
	int maxStartSample = (writePosition - endSamples) % delayBuffer.getNumSamples();

    // Get a random start point within the set range
    juce::Random random;
	int startSample = static_cast<int>(juce::jmap(random.nextFloat(), 0.0f, 1.0f, 
                                                  (float)minStartSample, (float)maxStartSample));
	
    // Copy audio from delayBuffer to new AudioBuffer
	auto grainBuffer = juce::AudioBuffer<float>(2, grainSizeSamples);
    grainBuffer.clear();

    for (int channel = 0; channel < getTotalNumInputChannels(); ++channel)
    {
        readFromDelayBuffer(grainBuffer, channel, startSample, 1.f);
    }
    
    // Apply fade envelope to the grain buffer
    int fadeLengthSamples = sampleRate / 2000;
    grainBuffer.applyGainRamp(0, fadeLengthSamples, 0, 1);
    grainBuffer.applyGainRamp(grainSizeSamples - fadeLengthSamples, fadeLengthSamples, 1, 0);

    // Initialize Grain object and add it to the vector
	Grain grain;
    grain.buffer = grainBuffer;
    grain.readPosition = 0;

    grainVector.push_back(grain);
}

void GranularDelayAudioProcessor::timerCallback()
{
    grainRequested.store(true);
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

    settings.inputGain = apvts.getRawParameterValue("inputGain")->load();
    settings.mix = apvts.getRawParameterValue("mix")->load();
    settings.grainSize = apvts.getRawParameterValue("grainSize")->load();
    settings.frequency = apvts.getRawParameterValue("frequency")->load();
    settings.rangeStart = apvts.getRawParameterValue("rangeStart")->load();
    settings.rangeEnd = apvts.getRawParameterValue("rangeEnd")->load();
    settings.dummyParameter1 = apvts.getRawParameterValue("dummyParameter1")->load();
    settings.dummyParameter2 = apvts.getRawParameterValue("dummyParameter2")->load();
    settings.dummyParameter3 = apvts.getRawParameterValue("dummyParameter3")->load();
    settings.dummyParameter4 = apvts.getRawParameterValue("dummyParameter4")->load();

    return settings;
}

juce::AudioProcessorValueTreeState::ParameterLayout
    GranularDelayAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // Add parameters to the APVTS layout
    layout.add(std::make_unique<juce::AudioParameterFloat>("inputGain", "Input Gain", 0.f, 2.f, 1.f));

    layout.add(std::make_unique<juce::AudioParameterFloat>("mix", "Mix", 0.f, 1.f, 0.5f));

    layout.add(std::make_unique<juce::AudioParameterFloat>("grainSize", "Grain Size", 
                                juce::NormalisableRange<float>(1.f, 100.f, 0.f, 0.5f), 10.f));

    layout.add(std::make_unique<juce::AudioParameterFloat>("frequency", "Frequency", 
                                juce::NormalisableRange<float>(1.f, 100.f, 0.f, 0.5f), 10.f));

    layout.add(std::make_unique<juce::AudioParameterFloat>("rangeStart", "Range Start",
                                juce::NormalisableRange<float>(0.f, 9000.f, 0.f, 0.5f), 100.f));

    layout.add(std::make_unique<juce::AudioParameterFloat>("rangeEnd", "Range End",
                                juce::NormalisableRange<float>(10.f, 10000.f, 0.f, 0.5f), 1000.f));

    layout.add(std::make_unique<juce::AudioParameterFloat>("dummyParameter1", "dummyParameter1", 0.f, 1.f, 0.5f));

    layout.add(std::make_unique<juce::AudioParameterFloat>("dummyParameter2", "dummyParameter2", 0.f, 1.f, 0.5f));

    layout.add(std::make_unique<juce::AudioParameterFloat>("dummyParameter3", "dummyParameter3", 0.f, 1.f, 0.5f));

    layout.add(std::make_unique<juce::AudioParameterFloat>("dummyParameter4", "dummyParameter4", 0.f, 1.f, 0.5f));

    return layout;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new GranularDelayAudioProcessor();
}

