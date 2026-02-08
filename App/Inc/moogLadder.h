#pragma once

#include <cmath>
#include <array>

class MoogLadder {
private:
    alignas(4) float stage[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    float delayStage3 = 0.0f;
    float feedbackHistory = 0.0f; 

    float k2vg = 0.0f; 
    float kacr = 0.0f; 
    float resGain = 0.0f;
    
    float i2v = 1.22f;
    float invI2v = 1.0f / i2v;
    
    float sampleRate;
    static constexpr int oversampling = 2;
    static constexpr int LUT_SIZE = 512;
    static constexpr float LUT_MAX_INPUT = 3.0f; 

    [[nodiscard]] inline float fast_tanh(float x) const noexcept {
        float x2 = x * x;
        // Pade approximation
        return x * (27.0f + x2) / (27.0f + 9.0f * x2);
    }

    // [[nodiscard]] inline float soft_clip(float x) const noexcept {
    //     if (x > 1.0f) return 0.666f;
    //     if (x < -1.0f) return -0.666f;
    //     return x - (x * x * x * 0.333333f);
    // }

public:
    MoogLadder() = default;

    void init(float sr) noexcept;

    void setCutoff(float cutoffHz);
    void setResonance(float resonance);

    [[nodiscard]] __attribute__((always_inline)) inline float process(float input) {
        // Copy class members to local stack variables once.
        const float local_k2vg = k2vg;
        const float local_invI2v = invI2v;
        const float local_resGain = resGain;
        const float local_kacr = kacr;
    
        for (int i = 0; i < oversampling; ++i) {
            float saturatedFeedback = fast_tanh(feedbackHistory * local_invI2v);
            // Scaled down because whistling was a problem with resonance
            float feedback = 3.2f * local_resGain * saturatedFeedback * local_kacr;
            // reduce whistling with soft clip
            // feedback = soft_clip(feedback);
            // float stageInput = input - feedback;

            // Replaced full digital moog ladder implementation with a simpler version which only uses fast_tanh on the input and feedback, not each stage
            float stageInput = fast_tanh((input - feedback) * local_invI2v);

            // Stage 1
            // float t0 = fast_tanh(stageInput * local_invI2v);
            // float t1 = fast_tanh(stage[0] * local_invI2v);
            // stage[0] += local_k2vg * (t0 - t1);
    
            // // Stage 2
            // float t2 = fast_tanh(stage[1] * local_invI2v);
            // stage[1] += local_k2vg * (t1 - t2);
    
            // // Stage 3
            // float t3 = fast_tanh(stage[2] * local_invI2v);
            // stage[2] += local_k2vg * (t2 - t3);
    
            // // Stage 4
            // float t4 = fast_tanh(stage[3] * local_invI2v);
            // stage[3] += local_k2vg * (t3 - t4);

            stage[0] += local_k2vg * (stageInput - stage[0]);
            stage[1] += local_k2vg * (stage[0] - stage[1]);
            stage[2] += local_k2vg * (stage[1] - stage[2]);
            stage[3] += local_k2vg * (stage[2] - stage[3]);
    
            feedbackHistory = (stage[3] + delayStage3) * 0.5f;
            delayStage3 = stage[3];
        }
        
        return stage[3];
    }
    
};
