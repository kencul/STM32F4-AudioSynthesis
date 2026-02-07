#pragma once
#include "osc.h"
#include <array>
#include <cstdint>

class VoiceManager {
private:
    static constexpr int MAX_VOICES = 8;
    std::array<Osc, MAX_VOICES> _voices;
    uint8_t _noteMap[MAX_VOICES];
    uint32_t _lastUsed[MAX_VOICES];
    uint32_t _tickCount = 0;
    float _sampleRate;
    uint16_t _bufferSize;

public:
    VoiceManager(float sr, uint16_t bufferSize) : _sampleRate(sr) {
        for(int i = 0; i < MAX_VOICES; i++) {
            _voices[i].init(sr);
            _noteMap[i] = 255; // 255 = Idle
        }
    }

    void noteOn(uint8_t note);
    void noteOff(uint8_t note);
    void process(int16_t* buffer, uint16_t numFrames);
    
    // Global parameter setters
    void setCutoff(float freq);
    void setResonance(float res);
    void setMorph(float morph);

    void setAttack(float seconds);
    void setDecay(float seconds);
    void setSustain(float level);
    void setRelease(float seconds);
};
