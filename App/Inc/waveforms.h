#pragma once
#include <cstdint>

extern const float waveform_Sine[4096];
extern const float waveform_Saw[4096];
extern const float waveform_Square[4096];
extern const float waveform_Rhodes[4096];
extern const float waveform_Clav[4096];
extern const float waveform_Choir[4096];
extern const float waveform_Acid[4096];
extern const float waveform_Glass[4096];

static const float* const waveLibrary[] = {
    waveform_Sine,
    waveform_Saw,
    waveform_Square,
    waveform_Rhodes,
    waveform_Clav,
    waveform_Choir,
    waveform_Acid,
    waveform_Glass
};

static constexpr uint8_t WAVE_COUNT = sizeof(waveLibrary) / sizeof(waveLibrary[0]);

extern const char* const waveNames[];

