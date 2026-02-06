#pragma once

#include <cmath>
#include <array>

class MoogLadder {
private:
    float stage[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    float delayStage3 = 0.0f;
    float feedbackHistory = 0.0f; 

    float k2vg = 0.0f; 
    float kacr = 0.0f; 
    float resGain = 0.0f;
    
    const float sampleRate;
    const int oversampling = 4;
    const float i2v = 1.22f;

    static constexpr int LUT_SIZE = 512;
    static constexpr float LUT_MAX_INPUT = 3.0f; 
    std::array<float, LUT_SIZE> tanhLUT;

    void initLUT();

    inline float lookupTanh(float x) {
        float absX = std::abs(x);
        float indexFloat = (absX / LUT_MAX_INPUT) * (LUT_SIZE - 1);
        int index = static_cast<int>(indexFloat);

        if (index >= LUT_SIZE - 1) return (x > 0) ? 1.0f : -1.0f;

        float fraction = indexFloat - index;
        float result = tanhLUT[index] + fraction * (tanhLUT[index + 1] - tanhLUT[index]);
        return (x > 0) ? result : -result;
    }

public:
    MoogLadder(float sr);

    void setCutoff(float cutoffHz);
    void setResonance(float resonance);

    inline float process(float input) {
        for (int i = 0; i < oversampling; ++i) {
            float feedback = 4.0f * resGain * feedbackHistory * kacr;
            float stageInput = input - feedback;

            for (int k = 0; k < 4; ++k) {
                float x = (k == 0) ? stageInput : stage[k - 1];
                stage[k] += k2vg * (lookupTanh(x / i2v) - lookupTanh(stage[k] / i2v));
            }

            feedbackHistory = (stage[3] + delayStage3) * 0.5f; // 
            delayStage3 = stage[3];
        }
        return stage[3];
    }
};
