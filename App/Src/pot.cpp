#include "pot.h"
#include <math.h>

Pot::Pot() : _alpha(0.1f), _filteredValue(0.0f), _lastStableValue(0), _threshold(20) {}

void Pot::init(float alpha, uint8_t threshold) {
    _alpha = alpha;
    _threshold = threshold;
}

bool Pot::update(uint16_t rawValue) {
    // if this is the first ever reading, set without filtering
    if(_filteredValue < 0){
        _filteredValue = (float)rawValue;
        _lastStableValue = rawValue;
        return true;
    } else {
        // EMA Filter: y[n] = α * x[n] + (1 - α) * y[n-1]
        _filteredValue = (_alpha * (float)rawValue) + ((1.0f - _alpha) * _filteredValue);
    }

    uint16_t currentInt = (uint16_t)_filteredValue;
    
    // Hysteresis check
    if (std::abs((int32_t)currentInt - (int32_t)_lastStableValue) > _threshold) {
        _lastStableValue = currentInt;
        return true; 
    }
    return false;
}

float Pot::getFloat() const {
    return (float)_lastStableValue / 4095.0f;
}

uint16_t Pot::getRaw() const {
    return _lastStableValue;
}

float Pot::scaleLin(float min, float max) const {
    return min + (getFloat() * (max - min));
}

float Pot::scaleLog(float min, float max) {
    // Standard frequency mapping
    return min * expf(logf(max / min) * getFloat());
}

float Pot::scaleExp(float min, float max, float curve) {
    // Great for ADSR times so the "short" end of the knob has more detail
    return min + powf(getFloat(), curve) * (max - min);
}
