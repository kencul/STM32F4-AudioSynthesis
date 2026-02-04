#ifndef SRC_OSC_H_
#define SRC_OSC_H_

#include <stdio.h>
#include <cstdint>
#define twopi 6.283185307179586

class Osc {
    float _freq;
    float _amp;
    float _morph;
    uint32_t _ph = 0;
    uint16_t _sr;
    uint32_t _phaseInc = 0;
    float _wavetableSin[1024];
    float _wavetableSquare[1024];
    float _midiTable[128];
public:
    Osc(float freq, float amp, uint16_t bufferSize, uint16_t sr);
    virtual ~Osc();

    uint16_t process(int16_t * buffer, uint16_t numFrames);

    void setAmplitude(float amp){_amp = amp;}

    void setFreq(float freq);

    void setFreq(uint32_t midiNote);

    void setMorph(float morph){_morph = morph;}
    
private:
    void calcPhaseInc();
};
#endif
