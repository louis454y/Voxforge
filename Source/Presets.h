#pragma once
#include "Params.h"
#include <iterator>

// Orden de valores (igual que ParamIndex):
// inGain, hpf, lowShelf, presence, air, thresh, ratio, attack, release, makeup,
// deess, deessFreq, sat, doubler, delayMix, delayTime, delayFb, reverbMix, reverbSize, outGain, mix
struct Preset
{
    const char* name;
    float v[kNumParams];
};

inline constexpr Preset kPresets[] =
{
    { "Init (Limpio)",           { 0,  80, 0, 0, 0, -18, 2.0f, 15, 150, 0,  0, 6500,  0,  0,  0, 250, 25,  0, 50,  0, 100 } },
    { "Reggaetón Lead",          { 0,  90, 1, 3, 3, -22, 4.0f,  8, 100, 5, 45, 6500, 20, 15, 18, 330, 30, 14, 35,  0, 100 } },
    { "Reggaetón Doble/Ad-libs", { 0, 120, 0, 2, 4, -20, 4.0f,  6,  90, 4, 40, 7000, 30, 45, 30, 250, 40, 20, 40,  0, 100 } },
    { "Deep House Suave",        { 0, 100, 2, 1, 5, -20, 3.0f, 20, 200, 3, 50, 6000, 12, 20, 20, 468, 35, 30, 70,  0, 100 } },
    { "Deep House Aéreo",        { 0, 110, 0, 0, 7, -18, 2.5f, 25, 250, 2, 40, 7500,  5, 35, 28, 468, 45, 42, 85, -1, 100 } },
    { "Trap Lead Punch",         { 0, 100, 1, 4, 2, -26, 6.0f,  3,  70, 7, 55, 6500, 40, 10, 15, 300, 25, 10, 25, -1, 100 } },
    { "Trap Oscuro/Ad-libs",     { 0, 130,-1, 2, 1, -24, 5.0f,  5,  90, 6, 50, 6000, 55, 30, 35, 375, 50, 28, 60, -1, 100 } },
    { "Afrobeats Cálido",        { 0,  85, 3, 2, 4, -20, 3.0f, 15, 140, 4, 40, 6500, 15, 20, 18, 345, 30, 18, 45,  0, 100 } },
    { "Afrobeats Brillante",     { 0,  95, 1, 3, 6, -22, 3.5f, 10, 120, 5, 45, 7000, 10, 30, 22, 345, 35, 22, 50,  0, 100 } },
    { "Voz Rap Hablada",         { 0, 100, 1, 3, 2, -24, 4.0f,  6,  90, 5, 50, 6500, 20,  0,  8, 200, 20,  6, 20,  0, 100 } },
    { "Coro Ancho",              { 0, 150,-1, 2, 5, -20, 3.0f, 20, 200, 3, 50, 6500, 10, 80, 15, 420, 30, 35, 70, -1, 100 } },
    { "Coro Etéreo",             { 0, 160,-2, 0, 7, -18, 2.5f, 25, 250, 2, 40, 7500,  5, 65, 30, 540, 50, 55, 90, -2, 100 } },
    { "Coro Íntimo",             { 0, 140, 0, 1, 3, -22, 3.0f, 18, 180, 4, 45, 6500,  8, 40, 10, 300, 20, 20, 40,  0, 100 } },
};

inline constexpr int kNumPresets = (int) std::size(kPresets);
