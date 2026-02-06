#include "midiBuffer.h"

// Global buffer instance
MidiBuffer gMidiBuffer;

extern "C" {
    void Midi_Push_To_Buffer(uint8_t* raw) {
        gMidiBuffer.push(raw);
    }
}
