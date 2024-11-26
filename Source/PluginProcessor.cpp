#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
GranularDelayAudioProcessor::GranularDelayAudioProcessor()
    : AudioProcessor (BusesProperties()
                       .withInput  ("Input",  juce::AudioChannelSet::stereo())
                       .withOutput ("Output", juce::AudioChannelSet::stereo())),
        apvts(*this, nullptr, "Parameters", createParameterLayout())
{
    chains.add(MonoChain());
    chains.add(MonoChain());
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

    chains[0].prepare(spec);
    chains[1].prepare(spec);

    auto chainSettings = getChainSettings(apvts);

    float gain = chainSettings.gain;
    float delayTimeInSamples = static_cast<int>((chainSettings.delayTime / 1000.0) * sampleRate);
    float feedback = chainSettings.feedback;
    
    chains[0].get<chainPositions::delay>().setMaximumDelayInSamples(sampleRate * 10);
    chains[0].get<chainPositions::delay>().setDelay(delayTimeInSamples);
    chains[0].get<chainPositions::gain>().setGainLinear(gain);
    
    chains[1].get<chainPositions::delay>().setMaximumDelayInSamples(sampleRate * 10);
    chains[1].get<chainPositions::delay>().setDelay(delayTimeInSamples);
    chains[1].get<chainPositions::gain>().setGainLinear(gain);

    // maxDelayInSamples = sampleRate * 10;

    // // Set the size of the delay buffer
    // delayBuffer.setSize(getTotalNumInputChannels(), static_cast<int>(maxDelayInSamples)); // 10 seconds buffer

    // // Initialize delay buffer with zeros
    // delayBuffer.clear();

    // // Initialize delayBufferWriteIndex for each channel
    // delayBufferWriteIndexArray.resize(getTotalNumInputChannels());
    // delayBufferWriteIndexArray.clear();

    // // Set delay time in samples
    // float delayTimeMs = apvts.getRawParameterValue("delayTime")->load();
    // delayTimeInSamples = static_cast<int>((delayTimeMs / 1000.0) * sampleRate); // Convert ms to samples
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
    DBG("Processing block with " << buffer.getNumSamples() << " samples, sample rate: " << getSampleRate());
    for (int i = 0; i < 10 && i < buffer.getNumSamples(); ++i)
    {
        DBG("Input sample " << i << ": " << buffer.getSample(0, i));
    }
    
    juce::ignoreUnused(midiMessages);

    juce::ScopedNoDenormals noDenormals;

    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels  = getTotalNumOutputChannels();
    auto numSamplesInBuffer = buffer.getNumSamples();


    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    auto chainSettings = getChainSettings(apvts);

    float gain = chainSettings.gain;
    float delayTimeInSamples = static_cast<int>((chainSettings.delayTime / 1000.0) * getSampleRate());
    float feedback = chainSettings.feedback;

    juce::dsp::AudioBlock<float> block(buffer);

    // Not using these right now 
    // auto leftBlock = block.getSingleChannelBlock(0);
    // auto rightBlock = block.getSingleChannelBlock(1);
    // juce::dsp::ProcessContextReplacing<float> leftContext(leftBlock);
    // juce::dsp::ProcessContextReplacing<float> rightContext(rightBlock);

    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto chain = chains[channel]; // The ProcessorChain for this channel

        chain.get<chainPositions::delay>().setDelay(delayTimeInSamples);
        chain.get<chainPositions::gain>().setGainLinear(gain);

        // Applies delay effect to each sample in the buffer
        for (int sample = 0; sample < numSamplesInBuffer; ++sample)
        {
            // Get input samples from the AudioBlock and DelayLine
            float inputSample = block.getSample(channel, sample);
            float delayedSample = chain.get<chainPositions::delay>().popSample(channel);

            // Add feedback
            float outputSample = inputSample + delayedSample * feedback;

            // Write back to delay line
            chain.get<chainPositions::delay>().pushSample(channel, outputSample);

            // Output the processed sample
            block.setSample(channel, sample, outputSample);
        }

        // Process entire block output gain
        block.multiplyBy(chain.get<chainPositions::gain>().getGainLinear());
    }

    // Log output data after processing
    for (int i = 0; i < 10 && i < buffer.getNumSamples(); ++i)
    {
        DBG("Output sample " << i << ": " << buffer.getSample(0, i));
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

