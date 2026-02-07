#include "MoogLadder.h"
#include <algorithm>

static constexpr float PI = 3.1415926535f;

void MoogLadder::init(float sr) noexcept {
    sampleRate = sr;
    setCutoff(1000.0f); 
}

void MoogLadder::setCutoff(float cutoffHz) {
    cutoffHz = std::clamp(cutoffHz, 20.0f, 12000.0f);
    float kw = cutoffHz / sampleRate;
    float kw2 = kw * kw;
    float kw3 = kw2 * kw;
    
    float fcr = 1.8730f * kw3 + 0.4955f * kw2 - 0.6490f * kw + 0.9988f;
    kacr = -3.9364f * kw2 + 1.8409f * kw + 0.9968f;

    float omega = (2.0f * PI * fcr * cutoffHz) / (oversampling * sampleRate);
    k2vg = i2v * (1.0f - std::exp(-omega)); 
}

void MoogLadder::setResonance(float resonance) {
    resGain = resonance; 
}
