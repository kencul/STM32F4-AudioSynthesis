#ifndef SRC_ADSR_H_
#define SRC_ADSR_H_

#include <cstdint>
#include <algorithm>

class Adsr {
    public:
    enum class EnvState {
        IDLE,    // Output is 0
        ATTACK,  // Rising to 1.0
        DECAY,   // Falling to Sustain level
        SUSTAIN, // Holding steady
        RELEASE  // Falling to 0.0
    };

    Adsr() noexcept = default;
    ~Adsr() = default;
    
    // Interface for the Osc to interact with
    void init(float sr) noexcept;
    void gate(bool on) noexcept;

    __attribute__((always_inline)) inline float getNextSample() noexcept {
        // Early exit for idle voices
        if (_state == EnvState::IDLE) return 0.0f;

        if (_state == EnvState::SUSTAIN) {
            return _sustainLevel;
        }

        if (_state == EnvState::ATTACK) {
            _output += _attackStep;
            if (_output >= 1.0f) {
                _output = 1.0f;
                _state = EnvState::DECAY;
                calcDecay();
            }
        } 
        else if (_state == EnvState::DECAY) {
            _output = _sustainLevel + (_output - _sustainLevel) * _decayMult;
            if ((_output - _sustainLevel) < 0.0001f) {
                _output = _sustainLevel;
                _state = EnvState::SUSTAIN;
            }
        } 
        else if (_state == EnvState::RELEASE) {
            _output = -0.01f + (_output + 0.01f) * _releaseMult;
            if (_output <= 0.0001f) {
                _output = 0.0f;
                _state = EnvState::IDLE;
            }
        }

        return _output;
    }

    [[nodiscard]] bool isActive() const noexcept { return _state != EnvState::IDLE; }
    [[nodiscard]] float getLevel() const noexcept { return _output; }
    
    // Setters for  parameters
    void setAttack(float seconds) noexcept;
    void setDecay(float seconds) noexcept;
    void setSustain(float level) noexcept;
    void setRelease(float seconds) noexcept;
private:
    EnvState _state = EnvState::IDLE;
    float _sampleRate = 48000.0f;
    float _invSampleRate = 1.0f / 48000.0f;
    float _output = 0.0f;

    float _attackTime{0.01f}, _decayTime{0.1f}, _releaseTime{0.5f};
    float _sustainLevel{0.7f};
    float _attackStep{0.0f}, _decayMult{0.0f}, _releaseMult{0.0f};

    [[nodiscard]] float calcMultiplier(float timeInSeconds) const noexcept;
    void calcDecay() noexcept;
    void calcRelease() noexcept;
};
#endif
