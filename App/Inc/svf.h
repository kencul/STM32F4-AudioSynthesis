/*
 * Copyright (c) 2026 Kencul
 * Licensed under the MIT License
 */

#pragma once

#include <cmath>
#include <algorithm>

/*
 * Zero-delay feedback State Variable Filter (SVF) using the Cytomic TPT topology
 * (Andy Simper, DAFx 2013). Trapezoidal integration eliminates the one-sample delay
 * in the feedback path that causes traditional SVF implementations to drift at high
 * cutoff frequencies. The algebraic loop is resolved analytically each sample:
 *   den = 1 / (1 + g*(g + k))  →  a1, a2, a3
 * Nonlinear saturation via fast_tanh on the first integrator state (s1) adds
 * analog warmth without a significant CPU overhead. Resonance is controlled via
 * damping coefficient k (range 2.0 → 0.01); clamping k to 0.01 keeps the filter
 * stable at self-oscillation. Returns lowpass (v2); highpass (v3) and bandpass
 * (v1) are also available within process() if needed.
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

    float sampleRate;
    float g{0.0f}, k{2.0f};
    float a1{0.0f}, a2{0.0f}, a3{0.0f};
    float s1{0.0f}, s2{0.0f};
};
