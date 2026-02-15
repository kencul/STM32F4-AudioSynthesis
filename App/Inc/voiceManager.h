#pragma once
#include "constants.h"
#include "osc.h"
#include <array>
#include <cstdint>
#include "app.h"
#include "constants.h"

class VoiceManager {
private:
    std::array<Osc, Constants::NUM_VOICES> _voices; 
    uint8_t _noteMap[Constants::NUM_VOICES];
    uint32_t _lastUsed[Constants::NUM_VOICES];
    uint32_t _tickCount = 0;
    //float _sampleRate = Constants::SAMPLE_RATE;
    //uint16_t _bufferSize;
    float mixBus[Constants::BUFFER_SIZE];

    std::array<float, Constants::NUM_VOICES> _voiceLevels;

public:
    VoiceManager(){
        for(int i = 0; i < Constants::NUM_VOICES; i++) {
            _voices[i].init();
            _noteMap[i] = 255; // 255 = Idle
        }
    }

    void noteOn(uint8_t note, uint8_t velocity);
    void noteOff(uint8_t note);
    void process(int16_t* buffer);
    
    // Parameter setters
    void setCutoff(float freq);
    void setResonance(float res);
    void setMorph(float morph);
    void setAttack(float seconds);
    void setDecay(float seconds);
    void setSustain(float level);
    void setRelease(float seconds);

    [[nodiscard]] float getVoiceLevel(uint8_t voiceIdx) const noexcept {
        return (voiceIdx < Constants::NUM_VOICES) ? _voiceLevels[voiceIdx] : 0.0f;
    }

    // MIDI CC Controls
    void setPitchBend(uint8_t lsb, uint8_t msb);
    void setModWheel(uint8_t value);
};
