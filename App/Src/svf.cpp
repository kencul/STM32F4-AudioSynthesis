#include "SVF.h"
#include "constants.h"

//static constexpr float PI = 3.1415926535f;

void SVF::init() noexcept {
    sampleRate = Constants::SAMPLE_RATE;
    s1 = 0.0f;
    s2 = 0.0f;
    setCutoff(1000.0f);
    setResonance(0.0f);
}

void SVF::setCutoff(float cutoffHz) noexcept {
    cutoffHz = std::clamp(cutoffHz, 20.0f, sampleRate * 0.49f);
    g = std::tan(Constants::PI * cutoffHz / sampleRate);
    updateCoefficients();
}

void SVF::setResonance(float resonance) noexcept {
    // Damping k: 2.0 (no res) to 0.01 (self-oscillation)
    resonance = std::clamp(resonance, 0.0f, 1.0f);
    k = 2.0f * (1.0f - resonance);
    
    if (k < 0.01f) k = 0.01f;

    updateCoefficients();
}

void SVF::updateCoefficients() noexcept {
    // Solve algebraic loop: D = 1 + g*k + g^2
    float den = 1.0f / (1.0f + g * (g + k));
    a1 = den;
    a2 = g * a1;
    a3 = g * a2;
}

void SVF::reset() noexcept {
    s1 = 0.0f; 
    s2 = 0.0f;
}
