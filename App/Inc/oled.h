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

    bool isBusy() const { return hi2c2.State != HAL_I2C_STATE_READY; }

    void drawPixel(int x, int y, bool color);
    void drawBuffer(const float* data, uint16_t size);
    void drawVLine(int x, int y1, int y2, bool color);
    void drawLine(int x0, int y0, int x1, int y1, bool color);
    void drawChar(int x, int y, char c, bool color);
    void drawString(int x, int y, const char* str, bool color);

private:
    uint8_t sendCommand(uint8_t cmd);
    
    const uint8_t _addr;
    uint8_t _buffer[1024]; // 128 * 64 / 8 bits

    enum Control : uint8_t {
        COMMAND = 0x00, //
        DATA    = 0x40  //
    };
};
