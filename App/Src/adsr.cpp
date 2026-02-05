#include "adsr.h"
#include <algorithm>

Adsr::Adsr(float sr, float attack, float decay, float sustain, float release){
    _sampleRate = sr;
    _sustainLevel = sustain;

    _sustainLevel = std::clamp(sustain, 0.0f, 1.0f);
    setAttack(attack);
    setRelease(release);
    _decayTime = std::max(0.0001f, decay);
    
    // Calculate all steps
    calcDecay();
}

Adsr::~Adsr(){}

// Interface for the Osc to interact with
void Adsr::gate(bool on){
    if(on){
        _state = EnvState::ATTACK;
        calcAttack(_output);

    } else {
        _state = EnvState::RELEASE;
        calcRelease(_output);
    }
}

float Adsr::getNextSample(){
    switch (_state) {
        case EnvState::IDLE:
            _output = 0.0f;
            break;

        case EnvState::ATTACK:
            _output += _attackStep;
            if (_output >= 1.0f) {
                _output = 1.0f;
                _state = EnvState::DECAY;
            }
            break;

        case EnvState::DECAY:
            _output -= _decayStep;
            if (_output <= _sustainLevel) {
                _output = _sustainLevel;
                _state = EnvState::SUSTAIN;
            }
            break;

        case EnvState::SUSTAIN:
            _output = _sustainLevel;
            break;

        case EnvState::RELEASE:
            _output -= _releaseStep;
            if (_output <= 0.0f) {
                _output = 0.0f;
                _state = EnvState::IDLE;
            }
            break;
    }
    return _output;
}

bool Adsr::isActive(){
    return _state != EnvState::IDLE;
}

// Setters for parameters
void Adsr::setAttack(float seconds){
    _attackTime = std::max(0.0001f, seconds);
    return;
}

void Adsr::setDecay(float seconds){
    _decayTime = std::max(0.0001f, seconds);
    calcDecay();
    return;
}

void Adsr::setSustain(float level){
    _sustainLevel = std::clamp(level, 0.f, 1.f);
    calcDecay();
    return;
}

void Adsr::setRelease(float seconds){
    _releaseTime = std::max(0.0001f, seconds);
    return;
}

void Adsr::calcAttack(float startValue){
    _attackStep = (1.0f - startValue) / (_attackTime * _sampleRate);
    return;
}
void Adsr::calcDecay(){
    _decayStep = (1.0f - _sustainLevel) / (_decayTime * _sampleRate);
    return;
}
void Adsr::calcRelease(float startValue){
    _releaseStep = startValue / (_releaseTime * _sampleRate);
    return;
}
