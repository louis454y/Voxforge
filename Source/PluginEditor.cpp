#include "PluginEditor.h"
#include "Presets.h"

namespace
{
    const juce::Colour kBgTop    { 0xff14161c };
    const juce::Colour kBgBottom { 0xff0b0c10 };
    const juce::Colour kPanel    { 0xff171a21 };
    const juce::Colour kPanelEdge{ 0xff272b35 };
    const juce::Colour kGold     { 0xffd4af6a };
    const juce::Colour kText     { 0xffc9ccd3 };
    const juce::Colour kDim      { 0xff7d828e };

    void drawMeter (juce::Graphics& g, juce::Rectangle<float> r, float level, const juce::String& name)
    {
        g.setColour (kDim);
        g.setFont (juce::Font (juce::FontOptions (11.f)).boldened());
        g.drawText (name, r.withWidth (26.f).toNearestInt(), juce::Justification::centredLeft);

        auto bar = r.withTrimmedLeft (28.f).withSizeKeepingCentre (r.getWidth() - 28.f, 8.f);
        g.setColour (juce::Colour (0xff0e1015));
        g.fillRoundedRectangle (bar, 4.f);

        const float db = juce::Decibels::gainToDecibels (level, -60.f);
        const float prop = juce::jlimit (0.f, 1.f, juce::jmap (db, -60.f, 0.f, 0.f, 1.f));

        g.saveState();
        g.reduceClipRegion (bar.withWidth (bar.getWidth() * prop).toNearestInt());
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2dd4bf), bar.getX(), 0.f,
                                                 juce::Colour (0xffef4444), bar.getRight(), 0.f, false));
        g.fillRoundedRectangle (bar, 4.f);
        g.restoreState();
    }
}

VoxLnF::VoxLnF()
{
    setColour (juce::Slider::textBoxTextColourId, kText);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff1c1f27));
    setColour (juce::ComboBox::textColourId, kText);
    setColour (juce::ComboBox::outlineColourId, kPanelEdge);
    setColour (juce::ComboBox::arrowColourId, kGold);
    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff1c1f27));
    setColour (juce::PopupMenu::textColourId, kText);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, kGold.withAlpha (0.25f));
    setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
}

void VoxLnF::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h,
                               float pos, float startAngle, float endAngle, juce::Slider&)
{
    auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (4.f);
    const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto c = bounds.getCentre();
    const float angle = startAngle + pos * (endAngle - startAngle);
    const float lw = 3.5f;
    const float arcR = radius - lw;

    juce::Path track;
    track.addCentredArc (c.x, c.y, arcR, arcR, 0.f, startAngle, endAngle, true);
    g.setColour (juce::Colour (0xff2a2e38));
    g.strokePath (track, juce::PathStrokeType (lw, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::Path val;
    val.addCentredArc (c.x, c.y, arcR, arcR, 0.f, startAngle, angle, true);
    g.setColour (kGold);
    g.strokePath (val, juce::PathStrokeType (lw, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    const float kr = radius - lw - 7.f;
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff3b404c), c.x, c.y - kr,
                                             juce::Colour (0xff181b22), c.x, c.y + kr, false));
    g.fillEllipse (c.x - kr, c.y - kr, kr * 2.f, kr * 2.f);
    g.setColour (juce::Colour (0xff08090c));
    g.drawEllipse (c.x - kr, c.y - kr, kr * 2.f, kr * 2.f, 1.f);

    juce::Path p;
    p.addRoundedRectangle (-1.6f, -kr + 2.f, 3.2f, kr * 0.45f, 1.5f);
    p.applyTransform (juce::AffineTransform::rotation (angle).translated (c.x, c.y));
    g.setColour (kGold);
    g.fillPath (p);
}

VoxForgeEditor::VoxForgeEditor (VoxForgeProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&lnf);

    for (int i = 0; i < kNumParams; ++i)
    {
        auto& k = knobs[(size_t) i];
        const auto& info = kParams[i];

        k.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        k.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 64, 16);
        k.slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f,
                                      juce::MathConstants<float>::pi * 2.8f, true);
        addAndMakeVisible (k.slider);

        k.label.setText (juce::String::fromUTF8 (info.name), juce::dontSendNotification);
        k.label.setJustificationType (juce::Justification::centred);
        k.label.setColour (juce::Label::textColourId, kDim);
        k.label.setFont (juce::Font (juce::FontOptions (12.f)).boldened());
        addAndMakeVisible (k.label);

        k.att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, info.id, k.slider);
    }

    groups = {
        { "EQ Y TONO",          { pHpf, pLowShelf, pPresence, pAir },                         {} },
        { "COMPRESOR",          { pThresh, pRatio, pAttack, pRelease, pMakeup },              {} },
        { "DE-ESSER / SAT",     { pDeess, pDeessFreq, pSat },                                 {} },
        { "ESPACIO Y COROS",    { pDoubler, pDelayMix, pDelayTime, pDelayFb, pReverbMix, pReverbSize }, {} },
        { "SALIDA",             { pInGain, pOutGain, pMix },                                  {} },
    };

    for (int i = 0; i < kNumPresets; ++i)
        presetBox.addItem (juce::String::fromUTF8 (kPresets[i].name), i + 1);
    presetBox.setSelectedId (proc.getCurrentProgram() + 1, juce::dontSendNotification);
    presetBox.onChange = [this] { proc.setCurrentProgram (presetBox.getSelectedId() - 1); };
    addAndMakeVisible (presetBox);

    setSize (920, 500);
    startTimerHz (30);
}

VoxForgeEditor::~VoxForgeEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void VoxForgeEditor::timerCallback()
{
    dispIn  = juce::jmax (proc.inLevel.load(),  dispIn  * 0.85f);
    dispOut = juce::jmax (proc.outLevel.load(), dispOut * 0.85f);
    repaint (meterIn.getUnion (meterOut).toNearestInt().expanded (4));
}

void VoxForgeEditor::paint (juce::Graphics& g)
{
    g.setGradientFill (juce::ColourGradient (kBgTop, 0.f, 0.f, kBgBottom, 0.f, (float) getHeight(), false));
    g.fillAll();

    auto title = juce::Font (juce::FontOptions (28.f)).boldened();
    title.setExtraKerningFactor (0.18f);
    g.setFont (title);
    g.setColour (kGold);
    g.drawText ("VOXFORGE", 20, 12, 260, 34, juce::Justification::centredLeft);

    g.setFont (juce::Font (juce::FontOptions (11.f)));
    g.setColour (kDim);
    g.drawText ("VOCAL SUITE  ·  REGGAETÓN  ·  DEEP HOUSE  ·  TRAP  ·  AFROBEATS", 22, 42, 420, 14,
                juce::Justification::centredLeft);

    for (auto& grp : groups)
    {
        auto b = grp.bounds.toFloat();
        g.setColour (kPanel);
        g.fillRoundedRectangle (b, 10.f);
        g.setColour (kPanelEdge);
        g.drawRoundedRectangle (b.reduced (0.5f), 10.f, 1.f);

        g.setColour (kGold.withAlpha (0.9f));
        g.setFont (juce::Font (juce::FontOptions (12.f)).boldened());
        g.drawText (grp.title, grp.bounds.withHeight (28).withTrimmedLeft (14), juce::Justification::centredLeft);
    }

    drawMeter (g, meterIn,  dispIn,  "IN");
    drawMeter (g, meterOut, dispOut, "OUT");
}

void VoxForgeEditor::layoutRow (juce::Rectangle<int> row, const std::vector<int>& gi)
{
    const int gap = 10, pad = 10;
    int cells = 0;
    for (int g : gi)
        cells += (int) groups[(size_t) g].idx.size();

    const int cellW = (row.getWidth() - gap * ((int) gi.size() - 1) - 2 * pad * (int) gi.size()) / cells;
    int x = row.getX();

    for (int g : gi)
    {
        auto& grp = groups[(size_t) g];
        const int w = (int) grp.idx.size() * cellW + 2 * pad;
        grp.bounds = { x, row.getY(), w, row.getHeight() };

        int kx = x + pad;
        for (int idx : grp.idx)
        {
            auto& k = knobs[(size_t) idx];
            const int top = row.getY() + 30;
            k.label.setBounds (kx, top, cellW, 16);
            const int sh = juce::jmin (row.getHeight() - 30 - 16 - 8, cellW + 18);
            k.slider.setBounds (kx, top + 16, cellW, sh);
            kx += cellW;
        }
        x += w + gap;
    }
}

void VoxForgeEditor::resized()
{
    auto area = getLocalBounds().reduced (16);
    auto header = area.removeFromTop (56);

    presetBox.setBounds (header.removeFromRight (230).withSizeKeepingCentre (230, 30));

    auto meters = header.removeFromRight (360).withSizeKeepingCentre (340, 20);
    meterIn  = meters.removeFromLeft (165).toFloat();
    meters.removeFromLeft (10);
    meterOut = meters.toFloat();

    area.removeFromTop (6);
    const int rowH = (area.getHeight() - 10) / 2;
    layoutRow (area.removeFromTop (rowH), { 0, 1, 2 });
    area.removeFromTop (10);
    layoutRow (area, { 3, 4 });
}
