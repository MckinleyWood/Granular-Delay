#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>


struct ChainSettings
{
    float gain;
    float delayTime;
    float feedback;
};

ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& apvts);

//==============================================================================
class GranularDelayAudioProcessor final : public juce::AudioProcessor
{
public:
    //==============================================================================
    GranularDelayAudioProcessor();
    ~GranularDelayAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout 
        createParameterLayout();
    juce::AudioProcessorValueTreeState apvts {*this, nullptr, 
        "Parameters", createParameterLayout()};

private:
    //==============================================================================
    using DelayLine = juce::dsp::DelayLine<float>;
    using GainStage = juce::dsp::Gain<float>;
    using MonoChain = juce::dsp::ProcessorChain<DelayLine, GainStage>;

    juce::Array<MonoChain> chains;

    enum chainPositions
    {
        delay,
        gain,
    };

    // // Array of indexes for writing new samples to delayBuffer (one will be added per channel)
    // juce::Array<int> delayBufferWriteIndexArray;
    // juce::AudioBuffer<float> delayBuffer; // Circular buffer for delayed audio 
    // int delayTimeInSamples = 1000;
    // int maxDelayInSamples;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GranularDelayAudioProcessor)
};
