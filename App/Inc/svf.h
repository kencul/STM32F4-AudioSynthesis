/*
 * Copyright (c) 2026 Kencul
 * Licensed under the MIT License
 */

#pragma once

#include <cmath>
#include <algorithm>

/*
 * Zero-delay feedback State Variable Filter (SVF). Trapezoidal integration
 * eliminates the one-sample delay in traditional SVF feedback paths that causes
 * phase drift at high cutoff frequencies. The algebraic loop is resolved
 * analytically per sample: den = 1 / (1 + g*(g+k)) → a1, a2, a3.
 */
class SVF {
public:
    SVF() = default;

    void init() noexcept;
    void setCutoff(float cutoffHz) noexcept;
    void setResonance(float resonance) noexcept;
    void reset() noexcept;

    [[nodiscard]] inline float process(float input) noexcept {
        float v3 = input - s2;
        float v1 = a1 * s1 + a2 * v3;
        float v2 = s2 + a2 * s1 + a3 * v3;

        s1 = fast_tanh(2.0f * v1 - s1); // trapezoidal update + soft-clip on s1; s2 stays linear
        s2 = 2.0f * v2 - s2;

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

    float sampleRate;
    float g{0.0f}, k{2.0f};
    float a1{0.0f}, a2{0.0f}, a3{0.0f};
    float s1{0.0f}, s2{0.0f};
};
