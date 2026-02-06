#include "osc.h"
#include <cstdint>
#include <math.h>
#include <algorithm>

float Osc::_wavetableSin[Osc::TABLE_SIZE];
float Osc::_wavetableSquare[Osc::TABLE_SIZE];
float Osc::_midiTable[Osc::MIDI_TABLE_SIZE];

Osc::Osc(float freq, float amp, uint16_t bufferSize, uint16_t sr) : _freq(freq), _amp(amp), _sr(sr), _filter((float)sr){
    
    static bool tablesInitialized = false;
    if (!tablesInitialized) {
        // Fill wavetables
        for(uint32_t i = 0; i < TABLE_SIZE; i++) {
            float angle = ((float)i / (float)TABLE_SIZE) * 2.0f * M_PI;
            _wavetableSin[i] = sinf(angle);
            
            // Band-Limited Square Table
            float squareSum = 0.0f;
            int numHarmonics = 20; // 20 harmonics
            
            for(int n = 1; n <= numHarmonics; n += 2) {
                squareSum += (1.0f / (float)n) * sinf(angle * n);
            }
            _wavetableSquare[i] = squareSum * (4.0f / M_PI);
        }
        
        // compute midi to freq table
        for(uint32_t i = 0; i < MIDI_TABLE_SIZE; i++) {
            _midiTable[i] = 440.0f * powf(2.0f, ((float)i - 69.f) / 12.0f);
        }
        
        tablesInitialized = true;
    }
    
    calcPhaseInc();
};

Osc::~Osc(){}

uint16_t Osc::process(int16_t * buffer, uint16_t numFrames){
    if(!_adsr.isActive()){
        return 0;
    }

    // value of 1 over int32
    const float invFraction = 1.0f / 1048576.0f;
    for(int i = 0; i < numFrames; i++){
        // Get top 12 bits of phase aka 0-4095
        uint32_t idx1 = _ph >> 20; 
        uint32_t idx2 = (idx1 + 1) & (TABLE_SIZE - 1); 
        // Bottom 22 bits are the fractional phase
        float fraction = (_ph & 0xFFFFF) * invFraction;

        // Linear interpolation
        float sine1 = _wavetableSin[idx1];
        float sine2 = _wavetableSin[idx2];
        float sineSample = sine1 + (sine2-sine1) * fraction;
        
        float square1 = _wavetableSquare[idx1];
        float square2 = _wavetableSquare[idx2];
        float squareSample = square1 + (square2-square1) * fraction;

        // morph two wavetables
        float sample = sineSample + _morph * (squareSample - sineSample);

        // Adsr
        sample *= _amp * _adsr.getNextSample(); 

        // if (sample > 1.0f) sample = 1.0f;
        // else if (sample < -1.0f) sample = -1.0f;
        // filtering
        sample = _filter.process(sample);

        if (sample > 1.0f) sample = 1.0f;
        else if (sample < -1.0f) sample = -1.0f;
        
        float finalSample = sample * 32767.0f;
        int16_t out = (int16_t)lrint(finalSample);
        
        buffer[i*2] += out;
        buffer[i*2+1] += out;

        _ph += _phaseInc;
    }
    return 0;
}
    
void Osc::setFreq(float freq){
    _freq = freq;
    calcPhaseInc();
    return;
}

void Osc::setFreq(uint32_t midiNote){
    midiNote = std::clamp<uint32_t>(midiNote, 0, 127);
    _freq = _midiTable[midiNote];
    calcPhaseInc();
    return;
}

void Osc::noteOn(){
    if(!_adsr.isActive()) _ph = 0;
    _adsr.gate(true);
    return;
}

void Osc::noteOn(uint32_t midiNote){
    setFreq(midiNote);
    noteOn();
    return;
}

void Osc::noteOff(){
    _adsr.gate(false);
    return;
}

void Osc::setAttack(float seconds){
    _adsr.setAttack(seconds);
    return;
}

void Osc::setDecay(float seconds){
    _adsr.setDecay(seconds);
    return;
}
void Osc::setSustain(float value){
    _adsr.setSustain(value);
    return;
}
void Osc::setRelease(float seconds){
    _adsr.setRelease(seconds);
    return;
}

void Osc::calcPhaseInc(){
    _phaseInc =  (uint32_t)(((float)_freq* 4294967296.0f) / (float)_sr);
    return;
}

void Osc::setCutOff(float freq){
    _filter.setCutoff(freq);
    return;
}

void Osc::setResonance(float res){

    _filter.setResonance(std::clamp(res, 0.0f, 0.99f));
    return;
}
