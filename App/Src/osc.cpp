#include "osc.h"
#include <cstdint>
#include <math.h>
#include <algorithm>
#include "waveforms.h"
#include "constants.h"

float Osc::_wavetableA[Osc::TABLE_SIZE];
float Osc::_wavetableB[Osc::TABLE_SIZE];
float Osc::_midiTable[Osc::MIDI_TABLE_SIZE];

uint8_t Osc::_currentIdx[2] = { 0, 1 };
float Osc::_pitchBendMult = 1.0f;

// Global LFO state initialization
uint32_t Osc::_lfoPhase = 0;
uint32_t Osc::_lfoInc = 0;
float Osc::_lfoValue = 0.0f;

void Osc::init() noexcept {
    _filter.init();
    _adsr.init();
    
    // Set LFO freq
    _lfoInc = static_cast<uint32_t>((Constants::LFO_FREQ * 4294967296.0f) / (static_cast<float>(Constants::SAMPLE_RATE) / Constants::NUM_FRAMES)); // Adjusted for block rate

    static bool tablesInitialized = false;
    if (!tablesInitialized) {
        loadWaveform(3, 0); 
        loadWaveform(7, 1);
        
        for(uint32_t i = 0; i < MIDI_TABLE_SIZE; i++) {
            _midiTable[i] = 440.0f * powf(2.0f, (static_cast<float>(i) - 69.f) / 12.0f);
        }
        tablesInitialized = true;
    }
    calcPhaseInc();
}

void Osc::updateGlobalLFO() noexcept {
    _lfoPhase += _lfoInc;
    // Fast floating point sine approximation
    _lfoValue = sinf(static_cast<float>(_lfoPhase) * 1.462918e-09f); // phase * (2PI / 2^32)
}
    
void Osc::calcPhaseInc() noexcept {
    float finalFreq = _freq * _pitchBendMult;
    _phaseInc = static_cast<uint32_t>((static_cast<double>(finalFreq) * 4294967296.0) / Constants::SAMPLE_RATE);
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
    //if(!_adsr.isActive()) _ph = 0;
    _adsr.gate(true);
}

void Osc::noteOn(uint32_t midiNote, float amp) noexcept {
    if (!_adsr.isActive()) {
        executeNoteOn(midiNote, amp);
    } else {
        _pending.midiNote = midiNote;
        _pending.velocity = amp;
        _pending.waiting = true;
        
        _adsr.kill(); 
    }
}

void Osc::executeNoteOn(uint32_t midiNote, float amp) noexcept {
    _ph = 0;             // Clean phase start
    _filter.reset();     // Clear filter energy
    setFreq(midiNote);
    setAmplitude(amp);
    _adsr.gate(true);    // Standard ADSR start
    _pending.waiting = false;
}

void Osc::noteOff() noexcept {
    _adsr.gate(false);
}

void Osc::getMorphedPreview(float* targetBuffer, uint16_t size, float morph) noexcept {
    float step = static_cast<float>(TABLE_SIZE) / static_cast<float>(size);
    for (uint16_t i = 0; i < size; ++i) {
        uint32_t idx = static_cast<uint32_t>(i * step) & (TABLE_SIZE - 1);
        float s1 = _wavetableA[idx];
        float s2 = _wavetableB[idx];
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

void Osc::setPitchBend(int16_t bendValue) noexcept {
    float normalizedBend = static_cast<float>(bendValue) / 8192.0f;
    _pitchBendMult = powf(2.0f, (normalizedBend * 2.0f) / 12.0f);
}

void Osc::applyPitchBend() noexcept {
    calcPhaseInc();
}

void Osc::forceReset() noexcept {
    //_ph = 0;
    _adsr.reset();
    _filter.reset();
}
