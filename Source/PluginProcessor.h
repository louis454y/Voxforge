#pragma once
#include <JuceHeader.h>
#include <array>
#include <atomic>
#include "Params.h"

class VoxForgeProcessor : public juce::AudioProcessor
{
public:
    VoxForgeProcessor();
    ~VoxForgeProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "VoxForge"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 3.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;
    std::atomic<float> inLevel { 0.f }, outLevel { 0.f };

private:
    using Filter = juce::dsp::IIR::Filter<float>;
    using Coeffs = juce::dsp::IIR::Coefficients<float>;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    float P (int i) const { return raw[(size_t) i]->load(); }

    void updateFilters();
    void runDeesser (juce::AudioBuffer<float>&, int numCh, int n, float amount);
    void runSaturation (juce::AudioBuffer<float>&, int numCh, int n, float amount);
    void runDelay (juce::AudioBuffer<float>&, int numCh, int n, float mix, float timeMs, float fb);

    std::array<std::atomic<float>*, (size_t) kNumParams> raw {};
    int currentProgram = 0;
    double sr = 44100.0;

    juce::dsp::ProcessorDuplicator<Filter, Coeffs> hpf, lowShelf, presence, air;
    juce::dsp::Compressor<float> comp;
    juce::dsp::Chorus<float> chorus;
    juce::dsp::Reverb reverb;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> delay;

    Filter deessFilter[2];
    float deessEnv[2] { 0.f, 0.f };

    juce::AudioBuffer<float> dryBuffer;
    float cached[5] {};
    bool cacheValid = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VoxForgeProcessor)
};
