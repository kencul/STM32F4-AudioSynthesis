#pragma once

#include <math.h>

namespace Constants {
    static constexpr float PI      = 3.14159265358979323846f;
    static constexpr float TWO_PI  = 6.28318530717958647692f;
    static constexpr float HALF_PI = 1.57079632679489661923f;

    static constexpr float LFO_FREQ = 5.0f;

    // Audio Engine 
    static constexpr int SAMPLE_RATE = 48000;
    static constexpr int BUFFER_SIZE  = 64;
    static constexpr int NUM_FRAMES = BUFFER_SIZE/2;
    static constexpr int CIRCULAR_BUFFER_SIZE = BUFFER_SIZE * 2;
    
    static constexpr int NUM_VOICES = 8;
    static constexpr float VOICE_GAIN_SCALAR = 0.13f; // slightly more than 1/8
}

