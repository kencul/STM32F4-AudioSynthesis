#pragma once

#include <cstdint>
#include <algorithm>
#include "adsr.h"
#include "moogLadder.h"

class Osc {
public:
    static constexpr uint32_t TABLE_SIZE = 4096;
    static constexpr uint32_t MIDI_TABLE_SIZE = 128;

    Osc() noexcept = default; 
    ~Osc() = default;

    void init(uint16_t sr) noexcept;
    
    __attribute__((always_inline)) inline void process(float* __restrict__ buffer, uint16_t numFrames) noexcept {
        if (!_adsr.isActive()) return;

        // Pinning variables to registers and converting division to multiplication
        uint32_t ph = _ph;
        const uint32_t inc = _phaseInc;
        const float morph = _morph;
        const float amp = _amp;
        static constexpr float invFraction = 1.0f / 1048576.0f; 

        for (uint32_t i = 0; i < numFrames; ++i) {
            const uint32_t idx1 = ph >> 20;
            const uint32_t idx2 = (idx1 + 1) & (TABLE_SIZE - 1);
            const float fraction = static_cast<float>(ph & 0xFFFFF) * invFraction;

            float sample;
            // Skip morph if possible
            if (morph <= 0.0f) {
                sample = _wavetableA[idx1] + (_wavetableA[idx2] - _wavetableA[idx1]) * fraction;
            } else if (morph >= 1.0f) {
                sample = _wavetableB[idx1] + (_wavetableB[idx2] - _wavetableB[idx1]) * fraction;
            } else {
                const float s1 = _wavetableA[idx1] + (_wavetableA[idx2] - _wavetableA[idx1]) * fraction;
                const float s2 = _wavetableB[idx1] + (_wavetableB[idx2] - _wavetableB[idx1]) * fraction;
                sample = s1 + morph * (s2 - s1);
            }

            // ADSR & Filter
            sample *= amp * _adsr.getNextSample();
            sample = _filter.process(sample);

            buffer[i << 1] += sample;
            buffer[(i << 1) + 1] += sample;

            ph += inc;
        }
        _ph = ph;
    }
    
    void setAmplitude(float amp) noexcept {_amp = std::clamp(amp, 0.0f, 1.0f);}
    void setFreq(float freq) noexcept;
    void setFreq(uint32_t midiNote) noexcept;
    void setMorph(float morph) noexcept {_morph = std::clamp(morph, 0.0f, 1.0f);}

    void noteOn() noexcept;
    void noteOn(uint32_t midiNote, float amp) noexcept;
    void noteOff() noexcept;

    void setAttack(float seconds) noexcept { _adsr.setAttack(seconds); }
    void setDecay(float seconds) noexcept { _adsr.setDecay(seconds); }
    void setSustain(float value) noexcept { _adsr.setSustain(value); }
    void setRelease(float seconds) noexcept { _adsr.setRelease(seconds); }
    void setCutoff(float freq) noexcept { _filter.setCutoff(freq); }
    void setResonance(float res) noexcept { _filter.setResonance(res); }

    [[nodiscard]] bool isActive() const noexcept { return _adsr.isActive(); }
    [[nodiscard]] float getAdsrLevel() const noexcept { return _adsr.getLevel(); }

    // OLED interface
    static void getMorphedPreview(float* targetBuffer, uint16_t size, float morph) noexcept;

    static void loadWaveform(uint8_t libraryIdx, uint8_t slot) noexcept;
    [[nodiscard]] static uint8_t getActiveIdx(uint8_t slot) noexcept { return _currentIdx[slot]; }
    
private:
    float _freq{440.0f};
    float _amp{0.5f};
    float _morph{0.0f};

    uint32_t _ph{0};
    uint16_t _sr{48000};
    uint32_t _phaseInc{0};

    static float _wavetableA[TABLE_SIZE] __attribute__((section(".ccmram")));
    static float _wavetableB[TABLE_SIZE] __attribute__((section(".ccmram")));
    static float _midiTable[MIDI_TABLE_SIZE] __attribute__((section(".ccmram")));

    Adsr _adsr;
    MoogLadder _filter;

    void calcPhaseInc() noexcept;

    static uint8_t _currentIdx[2];
};
