#ifndef SRC_OSC_H_
#define SRC_OSC_H_

#include <stdio.h>
#include <cstdint>
#define twopi 6.283185307179586

class Osc {
    float _freq;
    float _amp;
    float _ph;
    uint16_t _sr;
public:
    Osc(float freq, float amp, uint16_t bufferSize, uint16_t sr = 48000) : _freq(freq), _amp(amp), _sr(sr){};
    virtual ~Osc();

    uint16_t process(int16_t * buffer, uint16_t numFrames);

    void setAmplitude(float amp){_amp = amp;}
    
private:
    
};
#endif
