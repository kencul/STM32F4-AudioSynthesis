/*
 * Copyright (c) 2026 Kencul
 * Licensed under the MIT License
 */

#pragma once
#include "constants.h"
#include "osc.h"
#include <array>
#include <cstdint>
#include "app.h"
#include "constants.h"

/*
 * 8-voice polyphonic engine. Handles MIDI-driven voice allocation, mixing,
 * and global parameter dispatch.
 *
 * Voice allocation priority on noteOn:
 *   1. Idle voice (ADSR in IDLE state)
 *   2. Released voice (ADSR in RELEASE, not yet silent)
 *   3. Oldest active voice (LRU via _tickCount / _lastUsed), triggering
 *      the Adsr KILL ramp to avoid a click on the stolen voice.
 *
 * _noteMap[i] stores the MIDI note number owned by voice i (255 = unassigned),
 * used on noteOff to release the correct voice without a linear search on note number.
 *
 * process() accumulates floating-point output from all active voices into mixBus,
 * then clamps and converts to int16 for the I2S DMA buffer. The global LFO is
 * ticked once per block here via Osc::updateGlobalLFO() to keep vibrato
 * synchronized across all voices without per-sample overhead.
 */
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

    void setPitchBend(uint8_t lsb, uint8_t msb);
    void setModWheel(uint8_t value);
};
