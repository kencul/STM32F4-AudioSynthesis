/*
 * SVF host-side verification tests.
 *
 * These tests run on desktop (no HAL, no hardware) to verify the filter math
 * before deployment. Drive a sine, measure steady-state RMS,
 * compare to the expected magnitude response..
 */

#include "svf.h"
#include "constants.h"

#include <cmath>
#include <cstdio>

static int passed = 0;
static int failed = 0;

static void check(bool ok, const char* name) {
    if (ok) { printf("PASS  %s\n", name); ++passed; }
    else     { printf("FAIL  %s\n", name); ++failed; }
}

/*
 * Drives the filter with a steady-state sine and returns the RMS of the output
 * after an initial settle period. Small amplitude (0.01) keeps the nonlinear
 * fast_tanh stage in its linear regime so the result matches the linear
 * magnitude response.
 */
static float steadyStateRMS(SVF& svf, float freqHz, float amplitude,
                              int settleSamples, int measureSamples) {
    constexpr float SR = 48000.0f;
    const float phaseInc = Constants::TWO_PI * freqHz / SR;
    float phase = 0.0f;

    for (int i = 0; i < settleSamples; ++i) {
        (void)svf.process(amplitude * std::sin(phase));
        phase += phaseInc;
    }

    float sumSq = 0.0f;
    for (int i = 0; i < measureSamples; ++i) {
        float out = svf.process(amplitude * std::sin(phase));
        sumSq += out * out;
        phase += phaseInc;
    }
    return std::sqrt(sumSq / measureSamples);
}

/*
 * With Butterworth Q (k = √2, Q = 1/√2), a second-order lowpass is exactly
 * -3 dB at the pole frequency. Measured as the ratio of RMS at cutoff to RMS
 * in the passband, the target is 1/√2 ≈ 0.707 (±10% tolerance).
 */
static void test_cutoff_frequency() {
    // k = 2*(1 - resonance) = √2  →  resonance = 1 - √2/2 ≈ 0.2929
    constexpr float BUTTERWORTH_RES = 1.0f - 0.70711f;
    constexpr float CUTOFF_HZ       = 1000.0f;
    constexpr float AMP             = 0.01f;

    SVF svf;
    svf.init();
    svf.setCutoff(CUTOFF_HZ);
    svf.setResonance(BUTTERWORTH_RES);

    // 20 cycles at 100 Hz to reach steady state, then measure 10 cycles
    float passbandRMS = steadyStateRMS(svf, 100.0f, AMP, 9600, 4800);

    svf.reset();
    // 20 cycles at 1000 Hz to reach steady state, then measure 10 cycles
    float cutoffRMS = steadyStateRMS(svf, CUTOFF_HZ, AMP, 960, 480);

    float ratio = cutoffRMS / passbandRMS;
    printf("      ratio at cutoff / passband: %.4f  (target 0.707)\n", ratio);
    check(ratio > 0.636f && ratio < 0.778f,
          "SVF: -3 dB within +-10%% at cutoff (Butterworth Q)");
}

/*
 * At maximum resonance the damping coefficient k is clamped to 0.01, placing
 * the filter at the boundary of self-oscillation. An impulse should ring but
 * stay bounded indefinitely — blow-up here is a stability regression.
 */
static void test_self_oscillation_stability() {
    SVF svf;
    svf.init();
    svf.setCutoff(1000.0f);
    svf.setResonance(1.0f);

    bool ok = true;
    for (int i = 0; i < 10000; ++i) {
        float out = svf.process(i == 0 ? 1.0f : 0.0f);
        if (!std::isfinite(out) || std::abs(out) > 10.0f) {
            printf("      diverged at sample %d: %.4f\n", i, out);
            ok = false;
            break;
        }
    }
    check(ok, "SVF: stable at self-oscillation boundary (res=1.0, impulse, 10k samples)");
}

int main() {
    printf("=== SVF Tests ===\n");
    test_cutoff_frequency();
    test_self_oscillation_stability();
    printf("\n%d / %d passed\n", passed, passed + failed);
    return failed > 0 ? 1 : 0;
}
