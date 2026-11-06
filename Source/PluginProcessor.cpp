#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Presets.h"

using Coeffs = juce::dsp::IIR::Coefficients<float>;

juce::AudioProcessorValueTreeState::ParameterLayout VoxForgeProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    for (int i = 0; i < kNumParams; ++i)
    {
        const auto& p = kParams[i];
        juce::NormalisableRange<float> range (p.minV, p.maxV, 0.0f, p.skew);
        const juce::String suffix = juce::String::fromUTF8 (p.suffix);
        const int dec = p.decimals;

        auto attrs = juce::AudioParameterFloatAttributes()
            .withStringFromValueFunction ([dec, suffix] (float v, int) { return juce::String (v, dec) + suffix; });

        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { p.id, 1 }, juce::String::fromUTF8 (p.name), range, p.defV, attrs));
    }
    return layout;
}

VoxForgeProcessor::VoxForgeProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "VOXFORGE", createLayout())
{
    for (int i = 0; i < kNumParams; ++i)
        raw[(size_t) i] = apvts.getRawParameterValue (kParams[i].id);
}

bool VoxForgeProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto out = l.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::stereo() && out != juce::AudioChannelSet::mono())
        return false;
    return l.getMainInputChannelSet() == out;
}

void VoxForgeProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) samplesPerBlock,
                                  (juce::uint32) juce::jlimit (1, 2, getTotalNumOutputChannels()) };

    hpf.prepare (spec);
    lowShelf.prepare (spec);
    presence.prepare (spec);
    air.prepare (spec);
    comp.prepare (spec);
    chorus.prepare (spec);
    reverb.prepare (spec);

    delay.setMaximumDelayInSamples ((int) (sampleRate * 1.0) + 8);
    delay.prepare (spec);
    delay.reset();

    for (int ch = 0; ch < 2; ++ch)
    {
        deessFilter[ch].coefficients = Coeffs::makeHighPass (sampleRate, 6500.f);
        deessFilter[ch].reset();
        deessEnv[ch] = 0.f;
    }

    chorus.setRate (0.7f);
    chorus.setDepth (0.35f);
    chorus.setCentreDelay (18.f);
    chorus.setFeedback (0.f);

    dryBuffer.setSize (2, juce::jmax (1, samplesPerBlock));
    cacheValid = false;
}

void VoxForgeProcessor::updateFilters()
{
    const float now[5] = { P (pHpf), P (pLowShelf), P (pPresence), P (pAir), P (pDeessFreq) };

    if (cacheValid && std::equal (now, now + 5, cached))
        return;

    std::copy (now, now + 5, cached);
    cacheValid = true;

    *hpf.state      = *Coeffs::makeHighPass (sr, now[0]);
    *lowShelf.state = *Coeffs::makeLowShelf (sr, 200.f, 0.7f, juce::Decibels::decibelsToGain (now[1]));
    *presence.state = *Coeffs::makePeakFilter (sr, 3500.f, 0.9f, juce::Decibels::decibelsToGain (now[2]));
    *air.state      = *Coeffs::makeHighShelf (sr, 11000.f, 0.7f, juce::Decibels::decibelsToGain (now[3]));

    for (int ch = 0; ch < 2; ++ch)
        *deessFilter[ch].coefficients = *Coeffs::makeHighPass (sr, now[4]);
}

void VoxForgeProcessor::runDeesser (juce::AudioBuffer<float>& buf, int numCh, int n, float amount)
{
    if (amount < 0.001f)
        return;

    const float atk = 1.f - std::exp (-1.f / (0.0005f * (float) sr));
    const float rel = 1.f - std::exp (-1.f / (0.060f * (float) sr));
    constexpr float thr = 0.05f;

    for (int ch = 0; ch < numCh; ++ch)
    {
        auto* d = buf.getWritePointer (ch);
        auto& f = deessFilter[ch];
        float env = deessEnv[ch];

        for (int i = 0; i < n; ++i)
        {
            const float x  = d[i];
            const float hp = f.processSample (x);
            const float lo = x - hp;
            const float lvl = std::abs (hp);
            env += (lvl > env ? atk : rel) * (lvl - env);

            float g = 1.f;
            if (env > thr)
                g = 1.f - amount * (1.f - thr / env);

            d[i] = lo + hp * g;
        }
        deessEnv[ch] = env;
    }
}

void VoxForgeProcessor::runSaturation (juce::AudioBuffer<float>& buf, int numCh, int n, float amount)
{
    if (amount < 0.001f)
        return;

    const float drive = 1.f + amount * 4.f;
    const float inv = 1.f / drive;

    for (int ch = 0; ch < numCh; ++ch)
    {
        auto* d = buf.getWritePointer (ch);
        for (int i = 0; i < n; ++i)
            d[i] = std::tanh (drive * d[i]) * inv;
    }
}

void VoxForgeProcessor::runDelay (juce::AudioBuffer<float>& buf, int numCh, int n, float mix, float timeMs, float fb)
{
    if (mix < 0.001f)
        return;

    for (int ch = 0; ch < numCh; ++ch)
    {
        const float ds = (float) (timeMs * 0.001 * sr) * (ch == 0 ? 1.0f : 1.12f);
        auto* d = buf.getWritePointer (ch);

        for (int i = 0; i < n; ++i)
        {
            const float in = d[i];
            const float del = delay.popSample (ch, ds, true);
            delay.pushSample (ch, in + del * fb);
            d[i] = in + del * mix;
        }
    }
}

void VoxForgeProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int n = buffer.getNumSamples();
    const int numCh = juce::jmin (buffer.getNumChannels(), 2);

    for (int c = getTotalNumInputChannels(); c < buffer.getNumChannels(); ++c)
        buffer.clear (c, 0, n);

    if (numCh < 1 || n < 1)
        return;

    if (dryBuffer.getNumSamples() < n)
        dryBuffer.setSize (2, n, false, false, true);

    updateFilters();

    for (int ch = 0; ch < numCh; ++ch)
        dryBuffer.copyFrom (ch, 0, buffer, ch, 0, n);

    float inPeak = 0.f;
    for (int ch = 0; ch < numCh; ++ch)
        inPeak = juce::jmax (inPeak, buffer.getMagnitude (ch, 0, n));
    inLevel.store (inPeak);

    buffer.applyGain (juce::Decibels::decibelsToGain (P (pInGain)));

    juce::dsp::AudioBlock<float> block (buffer);
    auto sub = block.getSubsetChannelBlock (0, (size_t) numCh);
    juce::dsp::ProcessContextReplacing<float> ctx (sub);

    // 1) Limpieza + compresión
    hpf.process (ctx);

    comp.setThreshold (P (pThresh));
    comp.setRatio (P (pRatio));
    comp.setAttack (P (pAttack));
    comp.setRelease (P (pRelease));
    comp.process (ctx);
    buffer.applyGain (juce::Decibels::decibelsToGain (P (pMakeup)));

    // 2) Tono
    lowShelf.process (ctx);
    presence.process (ctx);
    air.process (ctx);

    // 3) De-esser y saturación
    runDeesser (buffer, numCh, n, P (pDeess) * 0.01f);
    runSaturation (buffer, numCh, n, P (pSat) * 0.01f);

    // 4) Doble / coro
    const float dbl = P (pDoubler) * 0.01f;
    if (dbl > 0.001f)
    {
        chorus.setMix (dbl);
        chorus.process (ctx);
    }

    // 5) Delay y reverb
    runDelay (buffer, numCh, n, P (pDelayMix) * 0.01f, P (pDelayTime), P (pDelayFb) * 0.01f);

    const float rv = P (pReverbMix) * 0.01f;
    if (rv > 0.001f)
    {
        juce::dsp::Reverb::Parameters rp;
        rp.roomSize = P (pReverbSize) * 0.01f;
        rp.damping = 0.5f;
        rp.wetLevel = rv * 0.4f;
        rp.dryLevel = 1.0f;
        rp.width = 1.0f;
        reverb.setParameters (rp);
        reverb.process (ctx);
    }

    // 6) Mezcla seco/húmedo + salida
    const float m = P (pMix) * 0.01f;
    const float og = juce::Decibels::decibelsToGain (P (pOutGain));
    float outPeak = 0.f;

    for (int ch = 0; ch < numCh; ++ch)
    {
        auto* w = buffer.getWritePointer (ch);
        const auto* d = dryBuffer.getReadPointer (ch);
        for (int i = 0; i < n; ++i)
        {
            w[i] = (d[i] * (1.f - m) + w[i] * m) * og;
            outPeak = juce::jmax (outPeak, std::abs (w[i]));
        }
    }
    outLevel.store (outPeak);
}

int VoxForgeProcessor::getNumPrograms() { return kNumPresets; }

const juce::String VoxForgeProcessor::getProgramName (int index)
{
    if (index < 0 || index >= kNumPresets)
        return {};
    return juce::String::fromUTF8 (kPresets[index].name);
}

void VoxForgeProcessor::setCurrentProgram (int index)
{
    if (index < 0 || index >= kNumPresets)
        return;

    currentProgram = index;

    for (int i = 0; i < kNumParams; ++i)
    {
        if (auto* p = apvts.getParameter (kParams[i].id))
        {
            p->beginChangeGesture();
            p->setValueNotifyingHost (p->convertTo0to1 (kPresets[index].v[i]));
            p->endChangeGesture();
        }
    }
}

void VoxForgeProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void VoxForgeProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* VoxForgeProcessor::createEditor()
{
    return new VoxForgeEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VoxForgeProcessor();
}
