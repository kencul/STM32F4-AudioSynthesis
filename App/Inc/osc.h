#ifndef SRC_OSC_H_
#define SRC_OSC_H_

#include <stdio.h>
#include <cstdint>
#include "adsr.h"
#include "moogLadder.h"

class Osc {
public:
    static constexpr uint32_t TABLE_SIZE = 4096;
    static constexpr uint32_t MIDI_TABLE_SIZE = 128;

    Osc(float freq, float amp, uint16_t bufferSize, uint16_t sr = 48000);
    virtual ~Osc();
    
    uint16_t process(int16_t * buffer, uint16_t numFrames);
    
    void setAmplitude(float amp){_amp = amp;}
    
    void setFreq(float freq);
    
    void setFreq(uint32_t midiNote);
    
    void setMorph(float morph){_morph = morph;}

    void noteOn();
    void noteOn(uint32_t midiNote);
    void noteOff();

    void setAttack(float seconds);
    void setDecay(float seconds);
    void setSustain(float value);
    void setRelease(float seconds);
    
private:
    float _freq;
    float _amp;
    float _morph;

    uint32_t _ph = 0;
    uint16_t _sr;
    uint32_t _phaseInc = 0;

    static float _wavetableSin[TABLE_SIZE];
    static float _wavetableSquare[TABLE_SIZE];
    static float _midiTable[MIDI_TABLE_SIZE];

    Adsr _adsr = Adsr();
    MoogLadder _filter;

    void calcPhaseInc();
};
#endif
