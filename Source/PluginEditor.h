#pragma once
#include <JuceHeader.h>
#include <array>
#include <vector>
#include "PluginProcessor.h"

class VoxLnF : public juce::LookAndFeel_V4
{
public:
    VoxLnF();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h,
                           float pos, float startAngle, float endAngle, juce::Slider&) override;
};

class VoxForgeEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit VoxForgeEditor (VoxForgeProcessor&);
    ~VoxForgeEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void layoutRow (juce::Rectangle<int> row, const std::vector<int>& groupIdx);

    VoxForgeProcessor& proc;
    VoxLnF lnf;

    struct Knob
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
    };
    std::array<Knob, (size_t) kNumParams> knobs;

    struct Group
    {
        juce::String title;
        std::vector<int> idx;
        juce::Rectangle<int> bounds;
    };
    std::vector<Group> groups;

    juce::ComboBox presetBox;
    juce::Rectangle<float> meterIn, meterOut;
    float dispIn = 0.f, dispOut = 0.f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VoxForgeEditor)
};
