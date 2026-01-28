#include "osc.h"

Osc::~Osc(){}

uint16_t Osc::process(int16_t * buffer, uint16_t numFrames){
    float phaseInc = _freq / (float)_sr;

    for(int i = 0; i < numFrames; i++){
        float sample = (_ph * 2.0f) - 1.0f;
        sample *= _amp;

        if (sample > 1.0f) sample = 1.0f;
        else if (sample < -1.0f) sample = -1.0f;
        
        int16_t out = (int16_t)(sample * 32767);
        
        buffer[i*2] = out;
        buffer[i*2+1] = out*0.1f;

        _ph += phaseInc;
        if (_ph > 1.0f) _ph -= 1.f;
    }
    return 0;
}
    
