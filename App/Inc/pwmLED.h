#pragma once

#include <cstdint>
#include "i2c.h"
#include <array>

class PWMLed {
public:
    explicit PWMLed(uint8_t address = 0x40);
    ~PWMLed() = default;

    uint8_t init();
    uint8_t ledAllOn(bool state);

    uint8_t setChannel(uint8_t index, float brightness);
    uint8_t updateVoices(const std::array<float, 8>& levels);

private:
    uint8_t writeRegister(uint8_t reg, uint8_t val);
    
    const uint8_t _deviceAddr;

    enum Reg : uint8_t {
        MODE1          = 0x00,
        LED0_ON_L      = 0x06, // Base register for channel 0 
        ALL_LED_ON_H   = 0xFB,
        ALL_LED_OFF_H  = 0xFD
    };

    enum Bit : uint8_t {
        SLEEP          = 0x10, // bit 4
        AI             = 0x20, // bit 5
        FULL_ON        = 0x10, // bit 4 of LEDn_ON_H
        FULL_OFF       = 0x10  // bit 4 of LEDn_OFF_H
    };
};
