#pragma once
#include <JuceHeader.h>

struct ParamInfo
{
    const char* id;
    const char* name;
    float minV, maxV, defV;
    const char* suffix;
    int decimals;
    float skew;
};

enum ParamIndex
{
    pInGain, pHpf, pLowShelf, pPresence, pAir,
    pThresh, pRatio, pAttack, pRelease, pMakeup,
    pDeess, pDeessFreq, pSat, pDoubler,
    pDelayMix, pDelayTime, pDelayFb, pReverbMix, pReverbSize,
    pOutGain, pMix,
    kNumParams
};

inline constexpr ParamInfo kParams[kNumParams] =
{
    { "inGain",     "Entrada",    -12.f,  12.f,   0.f,  " dB", 1, 1.0f },
    { "hpf",        "HPF",         20.f, 300.f,  80.f,  " Hz", 0, 0.5f },
    { "lowShelf",   "Graves",      -6.f,   6.f,   0.f,  " dB", 1, 1.0f },
    { "presence",   "Presencia",   -6.f,   8.f,   0.f,  " dB", 1, 1.0f },
    { "air",        "Aire",        -6.f,   8.f,   0.f,  " dB", 1, 1.0f },
    { "thresh",     "Umbral",     -40.f,   0.f, -18.f,  " dB", 1, 1.0f },
    { "ratio",      "Ratio",        1.f,  10.f,   3.f,  ":1",  1, 1.0f },
    { "attack",     "Ataque",       1.f,  80.f,  10.f,  " ms", 0, 0.5f },
    { "release",    "Release",     20.f, 400.f, 120.f,  " ms", 0, 0.5f },
    { "makeup",     "Makeup",       0.f,  12.f,   3.f,  " dB", 1, 1.0f },
    { "deess",      "De-Esser",     0.f, 100.f,  30.f,  " %",  0, 1.0f },
    { "deessFreq",  "Freq S",    4000.f,9000.f,6500.f,  " Hz", 0, 1.0f },
    { "sat",        "Saturación",   0.f, 100.f,  10.f,  " %",  0, 1.0f },
    { "doubler",    "Doble/Coro",   0.f, 100.f,   0.f,  " %",  0, 1.0f },
    { "delayMix",   "Delay",        0.f, 100.f,   0.f,  " %",  0, 1.0f },
    { "delayTime",  "Tiempo",      60.f, 600.f, 250.f,  " ms", 0, 1.0f },
    { "delayFb",    "Feedback",     0.f,  70.f,  25.f,  " %",  0, 1.0f },
    { "reverbMix",  "Reverb",       0.f, 100.f,   0.f,  " %",  0, 1.0f },
    { "reverbSize", "Sala",         0.f, 100.f,  50.f,  " %",  0, 1.0f },
    { "outGain",    "Salida",     -12.f,  12.f,   0.f,  " dB", 1, 1.0f },
    { "mix",        "Mezcla",       0.f, 100.f, 100.f,  " %",  0, 1.0f },
};
