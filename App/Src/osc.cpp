#include "osc.h"
#include <cstdint>
#include <math.h>
#include <algorithm>

Osc::Osc(float freq, float amp, uint16_t bufferSize, uint16_t sr = 48000) : _freq(freq), _amp(amp), _sr(sr){
    calcPhaseInc();

    // Fill wavetable with sine wave
    for(int i = 0; i < 1024; i++) {
        float angle = ((float)i / 1024.0f) * 2.0f * M_PI;
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
    for(int i = 0; i < 128; i++) {
        _midiTable[i] = 440.0f * powf(2.0f, (float)(i - 69) / 12.0f);
    }
};

Osc::~Osc(){}

uint16_t Osc::process(int16_t * buffer, uint16_t numFrames){
    // value of 1 over int32
    const float invFraction = 1.0f / 4194304.0f;
    for(int i = 0; i < numFrames; i++){
        // Get top 10 bits of phase aka 0-1023
        uint32_t idx1 = _ph >> 22; 
        uint32_t idx2 = (idx1 + 1) & 1023; 
        // Bottom 22 bits are the fractional phase
        float fraction = (_ph & 0x3FFFFF) * invFraction ;

        // Linear interpolation
        float sine1 = _wavetableSin[idx1];
        float sine2 = _wavetableSin[idx2];
        float sineSample = sine1 + (sine2-sine1) * fraction;
        
        float square1 = _wavetableSquare[idx1];
        float square2 = _wavetableSquare[idx2];
        float squareSample = square1 + (square2-square1) * fraction;

        // morph two wavetables
        float sample = squareSample + _morph * (sineSample - squareSample);

        sample *= _amp;

        if (sample > 1.0f) sample = 1.0f;
        else if (sample < -1.0f) sample = -1.0f;
        
        int16_t out = (int16_t)(sample * 32767);
        
        buffer[i*2] = out;
        buffer[i*2+1] = out;

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

void Osc::calcPhaseInc(){
    _phaseInc =  (uint32_t)(((double)_freq* 4294967296.0) / (double)_sr);
    return;
}
