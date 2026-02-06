#include "MoogLadder.h"
#include <algorithm>

MoogLadder::MoogLadder(float sr) : sampleRate(sr) {
    initLUT();
}

void MoogLadder::initLUT() {
    for (int i = 0; i < LUT_SIZE; ++i) {
        float x = (static_cast<float>(i) / (LUT_SIZE - 1)) * LUT_MAX_INPUT;
        tanhLUT[i] = std::tanh(x);
    }
}

void MoogLadder::setCutoff(float cutoffHz) {
    cutoffHz = std::clamp(cutoffHz, 20.0f, 12000.0f);
    float kw = cutoffHz / sampleRate;
    
    float fcr = 1.8730f * (std::pow(kw, 3)) + 0.4955f * (std::pow(kw, 2)) - 0.6490f * kw + 0.9988f;
    kacr = -3.9364f * (std::pow(kw, 2)) + 1.8409f * kw + 0.9968f;

    float omega = (2.0f * M_PI * fcr * cutoffHz) / (oversampling * sampleRate);
    k2vg = i2v * (1.0f - std::exp(-omega)); 
}

void MoogLadder::setResonance(float resonance) {
    resGain = resonance; 
}
