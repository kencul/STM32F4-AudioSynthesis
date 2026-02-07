#include "osc.h"
#include <cstdint>
#include <math.h>
#include <algorithm>

float Osc::_wavetableSin[Osc::TABLE_SIZE];
float Osc::_wavetableSquare[Osc::TABLE_SIZE];
float Osc::_midiTable[Osc::MIDI_TABLE_SIZE];

static constexpr float PI = 3.1415926535f;

void Osc::init(uint16_t sr) {
    _sr = sr;
    _filter.init(static_cast<float>(sr));
    _adsr.init(static_cast<float>(sr));
    
    static bool tablesInitialized = false;
    
    if (!tablesInitialized) {
        // Fill wavetables
        for(uint32_t i = 0; i < TABLE_SIZE; i++) {
            const float angle = (static_cast<float>(i)/ TABLE_SIZE) * 2.0f * PI;
            _wavetableSin[i] = sinf(angle);
            
            // Band-Limited Square Table
            float squareSum = 0.0f;
            // 20 harmonics
            for(int n = 1; n <= 20; n += 2) {
                squareSum += (1.0f / n) * sinf(angle * n);
            }
            _wavetableSquare[i] = squareSum * (4.0f / PI);
        }
        
        // compute midi to freq table
        for(uint32_t i = 0; i < MIDI_TABLE_SIZE; i++) {
            _midiTable[i] = 440.0f * powf(2.0f, (static_cast<float>(i) - 69.f) / 12.0f);
        }
        tablesInitialized = true;
    }

    calcPhaseInc();
}
    
void Osc::calcPhaseInc() noexcept {
    // Use double-precision constant for calculation to maintain accuracy before casting to uint32_t phase increment.
    _phaseInc = static_cast<uint32_t>((static_cast<double>(_freq) * 4294967296.0) / _sr);
}

void Osc::setFreq(float freq) noexcept {
    _freq = freq;
    calcPhaseInc();
}

void Osc::setFreq(uint32_t midiNote) noexcept {
    _freq = _midiTable[midiNote & 0x7F];
    calcPhaseInc();
}

void Osc::noteOn() noexcept {
    if(!_adsr.isActive()) _ph = 0;
    _adsr.gate(true);
}

void Osc::noteOn(uint32_t midiNote, float amp) noexcept {
    _amp = amp;
    setFreq(midiNote);
    noteOn();
}

void Osc::noteOff() noexcept {
    _adsr.gate(false);
}
