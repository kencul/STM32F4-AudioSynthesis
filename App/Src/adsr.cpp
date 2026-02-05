#include "adsr.h"
#include <algorithm>
#include <cmath>

Adsr::Adsr(float sr, float attack, float decay, float sustain, float release) {
    _sampleRate = sr;
    _output = 0.0f;
    _state = EnvState::IDLE;

    setAttack(attack);
    setDecay(decay);
    setSustain(sustain);
    setRelease(release);
}

Adsr::~Adsr() {}

void Adsr::gate(bool on) {
    if (on) {
        _state = EnvState::ATTACK;
        // Linear attack: calculate step based on current output to peak (1.0)
        _attackStep = (1.0f - _output) / (std::max(0.0001f, _attackTime) * _sampleRate);
    } else {
        _state = EnvState::RELEASE;
        // Pre-calculate multiplier for exponential release
        calcRelease();
    }
}

float Adsr::getNextSample() {
    switch (_state) {
        case EnvState::IDLE:
            _output = 0.0f;
            break;

        case EnvState::ATTACK:
            _output += _attackStep;
            if (_output >= 1.0f) {
                _output = 1.0f;
                _state = EnvState::DECAY;
                calcDecay(); // Prepare multiplier for decay
            }
            break;

        case EnvState::DECAY:
            // Exponential decay toward sustain level
            // We use a small offset so it actually reaches the target
            _output = _sustainLevel + (_output - _sustainLevel) * _decayMult;
            
            // If we are within a tiny margin of sustain, snap to it
            if (std::abs(_output - _sustainLevel) < 0.0001f) {
                _output = _sustainLevel;
                _state = EnvState::SUSTAIN;
            }
            break;

        case EnvState::SUSTAIN:
            _output = _sustainLevel;
            break;

        case EnvState::RELEASE:
            // Exponential release toward a target slightly below 0
            // This ensures the curve crosses 0 in a reasonable time
            _output = -0.01f + (_output - (-0.01f)) * _releaseMult;

            if (_output <= 0.0f) {
                _output = 0.0f;
                _state = EnvState::IDLE;
            }
            break;
    }
    return _output;
}

// Helper to calculate the multiplier coefficient
// rate = how long to reach ~0.01% of the distance
float Adsr::calcMultiplier(float timeInSeconds) {
    if (timeInSeconds <= 0.0001f) return 0.0f;
    // T60: time to drop 60dB (to 0.001 of original value)
    return expf(-6.907755f / (timeInSeconds * _sampleRate)); 
}

void Adsr::setAttack(float seconds) {
    _attackTime = std::max(0.0001f, seconds);
}

void Adsr::setDecay(float seconds) {
    _decayTime = std::max(0.0001f, seconds);
    calcDecay();
}

void Adsr::setSustain(float level) {
    _sustainLevel = std::clamp(level, 0.0f, 1.0f);
    // If we change sustain while decaying, we don't need to re-calc multiplier,
    // just the target changes in getNextSample()
}

void Adsr::setRelease(float seconds) {
    _releaseTime = std::max(0.0001f, seconds);
    calcRelease();
}

void Adsr::calcDecay() {
    _decayMult = calcMultiplier(_decayTime);
}

void Adsr::calcRelease() {
    _releaseMult = calcMultiplier(_releaseTime);
}

bool Adsr::isActive() {
    return _state != EnvState::IDLE;
}
