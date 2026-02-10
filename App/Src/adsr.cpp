#include "adsr.h"
#include <cmath>

void Adsr::init(float sr) noexcept {
    _sampleRate = sr;
    _invSampleRate = 1.0f / sr;
    _state = EnvState::IDLE;
    _output = 0.0f;
}

void Adsr::gate(bool on) noexcept {
    if (on) {
        _state = EnvState::ATTACK;
        _attackStep = (1.0f - _output) / (std::max(0.0001f, _attackTime) * _sampleRate);
    } else {
        if (_state != EnvState::IDLE) {
            _state = EnvState::RELEASE;
            calcRelease();
        }
    }
}

float Adsr::calcMultiplier(float timeInSeconds) const noexcept {
    if (timeInSeconds <= 0.0001f) return 0.0f;
    // T60 coefficient calculation
    return expf(-6.907755f / (timeInSeconds * _sampleRate)); 
}

void Adsr::setAttack(float seconds) noexcept {
    _attackTime = std::max(0.001f, seconds);
}

void Adsr::setDecay(float seconds) noexcept {
    _decayTime = std::max(0.001f, seconds);
    if (_state == EnvState::DECAY) calcDecay();
}

void Adsr::setSustain(float level) noexcept {
    _sustainLevel = std::clamp(level, 0.0f, 1.0f);
}

void Adsr::setRelease(float seconds) noexcept {
    _releaseTime = std::max(0.001f, seconds);
    if (_state == EnvState::RELEASE) calcRelease();
}

void Adsr::calcDecay() noexcept {
    _decayMult = calcMultiplier(_decayTime);
}

void Adsr::calcRelease() noexcept {
    _releaseMult = calcMultiplier(_releaseTime);
}
