#ifndef SRC_ADSR_H_
#define SRC_ADSR_H_

#include <cstdint>

class Adsr {

    enum class EnvState {
        IDLE,    // Output is 0
        ATTACK,  // Rising to 1.0
        DECAY,   // Falling to Sustain level
        SUSTAIN, // Holding steady
        RELEASE  // Falling to 0.0
    };

public:
    Adsr(float sr = 48000, float attack = 0.1f, float decay = 0.1f, float sustain = 0.7f, float release = 0.5f);
    virtual ~Adsr();

    // Interface for the Osc to interact with
    void gate(bool on);
    float getNextSample();
    bool isActive();
    
    // Setters for  parameters
    void setAttack(float seconds);
    void setDecay(float seconds);
    void setSustain(float level); // 0.0 - 1.0
    void setRelease(float seconds);
private:
    EnvState _state = EnvState::IDLE;
    float _sampleRate;

    float _output = 0.0f;     // The current gain value (0.0 to 1.0)

    float _attackTime, _decayTime, _releaseTime;
    float _sustainLevel;
    // Increments calculated based on time and sample rate
    float _attackStep, _decayStep, _releaseStep;

    //void calcSteps();
    void calcAttack(float startValue);
    void calcRelease(float startValue);
    void calcDecay();
};
#endif
