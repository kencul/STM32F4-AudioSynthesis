#pragma once

namespace Constants {
    static constexpr float PI      = 3.14159265358979323846f;
    static constexpr float TWO_PI  = 6.28318530717958647692f;
    static constexpr float HALF_PI = 1.57079632679489661923f;

    // Audio Engine 
    static constexpr int SAMPLE_RATE = 48000;
    static constexpr int BUFFER_SIZE  = 64;
    static constexpr int NUM_FRAMES = BUFFER_SIZE/2;
    static constexpr int CIRCULAR_BUFFER_SIZE = BUFFER_SIZE * 2;
}

