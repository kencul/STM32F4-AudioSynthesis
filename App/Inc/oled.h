#pragma once

#include <cstdint>
#include <vector>
#include "i2c.h"

class Oled {
public:
    explicit Oled(uint8_t address = 0x3C);
    uint8_t init();
    void fill(bool white);
    uint8_t update();

    void drawPixel(int x, int y, bool color);
    void drawBuffer(const float* data, uint16_t size);
    void drawVLine(int x, int y1, int y2, bool color);

private:
    uint8_t sendCommand(uint8_t cmd);
    
    const uint8_t _addr;
    uint8_t _buffer[1024]; // 128 * 64 / 8 bits

    enum Control : uint8_t {
        COMMAND = 0x00, //
        DATA    = 0x40  //
    };
};
