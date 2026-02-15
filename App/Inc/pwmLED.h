#pragma once

#include <cstdint>
#include "i2c.h"
#include <array>
#include "constants.h"

class PWMLed {
public:
    explicit PWMLed(uint8_t address = 0x40);
    ~PWMLed() = default;

    uint8_t init();
    uint8_t ledAllOn(bool state);

    uint8_t setChannel(uint8_t index, float brightness);
    uint8_t updateVoices(const std::array<float, Constants::NUM_VOICES>& levels);

private:
    uint8_t writeRegister(uint8_t reg, uint8_t val);
    
    const uint8_t _deviceAddr;

    enum Reg : uint8_t {
        MODE1          = 0x00,
        MODE2          = 0x01,
        LED0_ON_L      = 0x06,
        ALL_LED_ON_H   = 0xFB,
        ALL_LED_OFF_H  = 0xFD
    };

    enum Bit : uint8_t {
        SLEEP    = 0x10, 
        AI       = 0x20, 
        // MODE2 Bits
        INVRT    = 0x10, // bit 4
        OUTDRV   = 0x04, // bit 2
        OUTNE_01 = 0x01, // bit 0 (OUTNE consists of bits 0 and 1)
        FULL_ON        = 0x10, // bit 4 of LEDn_ON_H
        FULL_OFF       = 0x10  // bit 4 of LEDn_OFF_H
    };
};
