#include "osc.h"
#include <cstdint>
#include <math.h>
#include <algorithm>
#include "waveforms.h"

float Osc::_wavetableA[Osc::TABLE_SIZE];
float Osc::_wavetableB[Osc::TABLE_SIZE];
float Osc::_midiTable[Osc::MIDI_TABLE_SIZE];

uint8_t Osc::_currentIdx[2] = { 0, 1 };

//static constexpr float PI = 3.1415926535f;

void Osc::init(uint16_t sr) noexcept {
    _sr = sr;
    _filter.init(static_cast<float>(sr));
    _adsr.init(static_cast<float>(sr));
    
    static bool tablesInitialized = false;
    
    if (!tablesInitialized) {
        loadWaveform(0, 0); 
        loadWaveform(1, 1);
        
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

void Osc::getMorphedPreview(float* targetBuffer, uint16_t size, float morph) noexcept {
    // Iterate across the 'size' of the screen (e.g., 128 pixels)
    float step = static_cast<float>(TABLE_SIZE) / static_cast<float>(size);
    
    for (uint16_t i = 0; i < size; ++i) {
        uint32_t idx = static_cast<uint32_t>(i * step) & (TABLE_SIZE - 1);
        
        float s1 = _wavetableA[idx];
        float s2 = _wavetableB[idx];
        
        // Simple linear morph for the preview
        targetBuffer[i] = s1 + morph * (s2 - s1);
    }
}

void Osc::loadWaveform(uint8_t libraryIdx, uint8_t slot) noexcept {
    if (libraryIdx >= WAVE_COUNT || slot > 1) return;

    const float* source = waveLibrary[libraryIdx];
    float* target = (slot == 0) ? _wavetableA : _wavetableB;

    std::copy(source, source + TABLE_SIZE, target);
    _currentIdx[slot] = libraryIdx;
}
