#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <juce_audio_utils/gui/juce_AudioVisualiserComponent.h>

struct ChainSettings
{
    float inputGain;
    float mix;
    float grainSize;
    float frequency;
    float rangeStart;
    float rangeEnd;
    float grainPitch;
    float detune;
    float dummy2;
    float dummy4;
};

ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& apvts);

//==============================================================================
struct Grain
{
    float readPosition;
    float endSample;
    float playbackSpeed;
    std::vector<bool> needsToWrapAround;
    bool finished = false;

    Grain(float initialReadPosition, float grainSize, float delayBufferSize, float initialPlaybackSpeed, int numChannels)
        : readPosition(initialReadPosition),
          endSample(initialReadPosition + grainSize),
          playbackSpeed(initialPlaybackSpeed),
          needsToWrapAround(static_cast<std::vector<bool>::size_type>(numChannels))
    {
        if (endSample >= delayBufferSize)
        {
            endSample -= delayBufferSize;
            std::fill(needsToWrapAround.begin(), needsToWrapAround.end(), true);
        }
        else
        {
            std::fill(needsToWrapAround.begin(), needsToWrapAround.end(), false);
        }
    }
};

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

    juce::AudioVisualiserComponent waveViewer;

private:
    //==============================================================================
    void fillDelayBuffer(juce::AudioBuffer<float>& buffer, float gain);
    // void readFromDelayBuffer(juce::AudioBuffer<float>& buffer, int channel, int readPosition, float gain);
    void readGrains(juce::AudioBuffer<float>& buffer);
    void readOneGrain(juce::AudioBuffer<float>& buffer, Grain& grain);
    void updateWritePosition(int numSamples);
    void addGrain();
    float getGrainStartSample();
    float getGrainPlaybackSpeed();

    void timerCallback();

    //==============================================================================
    juce::TimedCallback timer;
    std::vector<Grain> grainVector;

    juce::AudioBuffer<float> delayBuffer;
    juce::AudioBuffer<float> wetBuffer;
    std::atomic<bool> grainRequested { false };
    std::atomic<int> timerInterval { 10 };
    int writePosition { 0 };

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GranularDelayAudioProcessor)
};
