#pragma once

#include <cmath>
#include <algorithm>

class SVF {
public:
    SVF() = default;

    void init(float sr) noexcept;
    void setCutoff(float cutoffHz) noexcept;
    void setResonance(float resonance) noexcept;

    [[nodiscard]] __attribute__((always_inline)) inline float process(float input) noexcept {
        float v3 = input - s2;
        float v1 = a1 * s1 + a2 * v3;
        float v2 = s2 + a2 * s1 + a3 * v3;

        // Trapezoidal state update (2*v - s)
        s1 = fast_tanh(2.0f * v1 - s1);
        s2 = 2.0f * v2 - s2;

        // Nonlinear saturation for analog warmth
        return v2;
    }

private:
    void updateCoefficients() noexcept;

    [[nodiscard]] inline float fast_tanh(float x) const noexcept {
        if (x < -3.0f) return -1.0f;
        if (x > 3.0f) return 1.0f;
        float x2 = x * x;
        return x * (27.0f + x2) / (27.0f + 9.0f * x2);
    }

    float sampleRate{48000.0f};
    float g{0.0f}, k{2.0f};
    float a1{0.0f}, a2{0.0f}, a3{0.0f};
    float s1{0.0f}, s2{0.0f};
};
