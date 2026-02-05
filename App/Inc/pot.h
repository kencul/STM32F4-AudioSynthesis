#ifndef POT_H
#define POT_H

#include <cstdint>
#include <cmath>

class Pot {
public:
    Pot();
    
    // Configures filtering and hysteresis
    void init(float alpha, uint8_t threshold);
    
    // Updates internal state; returns true if the value crossed the hysteresis threshold
    bool update(uint16_t rawValue);

    float getFloat() const;
    uint16_t getRaw() const;
    float scaleLin(float min, float max) const;

    float scaleLog(float min, float max);
    
    float scaleExp(float min, float max, float curve = 2.0f);

private:
    float _alpha;
    float _filteredValue = -1.f;
    uint16_t _lastStableValue;
    uint8_t _threshold;
};

#endif
