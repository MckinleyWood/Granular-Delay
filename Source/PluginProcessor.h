#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>


struct ChainSettings
{
    float gain;
    float delayTime;
    float feedback;
    float mix;
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
    void fillDelayBuffer(juce::AudioBuffer<float>& buffer, int channel, float gain);
    void readFromDelayBuffer(juce::AudioBuffer<float>& buffer, int channel, float gain);
    void updateWritePosition(juce::AudioBuffer<float>& buffer);

    //==============================================================================
    juce::AudioBuffer<float> delayBuffer;
    juce::AudioBuffer<float> tempBuffer;
    int writePosition { 0 };
    int readPosition { 0 };

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GranularDelayAudioProcessor)
};
