#include "oled.h"
#include <cstring>
#include <cmath>
#include "glcdfont.h"

Oled::Oled(uint8_t address) : _addr(address << 1) {
    std::memset(_buffer, 0x00, sizeof(_buffer));
}

uint8_t Oled::init() {
    uint8_t status = 0;
    
    // Initialization Sequence
    status += sendCommand(0xAE); // Display OFF
    status += sendCommand(0xD5); // Set Display Clock Divide Ratio
    status += sendCommand(0x80); 
    status += sendCommand(0xA8); // Set Multiplex Ratio
    status += sendCommand(0x3F); 
    status += sendCommand(0xD3); // Set Display Offset
    status += sendCommand(0x00);
    status += sendCommand(0x40); // Set Display Start Line
    status += sendCommand(0x8D); // Charge Pump
    status += sendCommand(0x14); // Enable Charge Pump
    status += sendCommand(0x20); // Set Memory Addressing Mode
    status += sendCommand(0x00); // Horizontal Addressing Mode
    status += sendCommand(0xA1); // Set Segment Re-map (X-mirror)
    status += sendCommand(0xC8); // Set COM Output Scan Direction (Y-mirror)
    status += sendCommand(0xDA); // Set COM Pins Hardware Configuration
    status += sendCommand(0x12);
    status += sendCommand(0x81); // Set Contrast Control
    status += sendCommand(0xCF);
    status += sendCommand(0xAF); // Display ON
    
    return status;
}

void Oled::fill(bool white) {
    // Fill the buffer with 0xFF for white or 0x00 for black
    std::memset(_buffer, white ? 0xFF : 0x00, sizeof(_buffer));
}

uint8_t Oled::update() {
    if (hi2c2.State != HAL_I2C_STATE_READY) return 1;

    // Reset column and page pointers to start of GDDRAM
    sendCommand(0x21); // Set Column Address
    sendCommand(0);    // Column start
    sendCommand(127);  // Column end
    sendCommand(0x22); // Set Page Address
    sendCommand(0);    // Page start
    sendCommand(7);    // Page end (64 lines / 8 bits per page = 8 pages)

    // Use DMA to send the entire 1024-byte buffer in the background
    if (HAL_I2C_Mem_Write_DMA(&hi2c2, _addr, Control::DATA, 1, _buffer, sizeof(_buffer)) != HAL_OK) {
        return 1;
    }
    return 0;
}

uint8_t Oled::sendCommand(uint8_t cmd) {
    return (HAL_I2C_Mem_Write(&hi2c2, _addr, Control::COMMAND, 1, &cmd, 1, HAL_MAX_DELAY) == HAL_OK) ? 0 : 1;
}

void Oled::drawPixel(int x, int y, bool color) {
    if (x < 0 || x >= 128 || y < 0 || y >= 64) return;

    // SSD1306 is organized in pages (8 rows of pixels per page)
    if (color) {
        _buffer[x + (y / 8) * 128] |= (1 << (y % 8));
    } else {
        _buffer[x + (y / 8) * 128] &= ~(1 << (y % 8));
    }
}

void Oled::drawVLine(int x, int y1, int y2, bool color) {
    if (x < 0 || x >= 128) return;
    
    // Ensure y1 is the smaller value for the loop
    int start = (y1 < y2) ? y1 : y2;
    int end = (y1 < y2) ? y2 : y1;
    
    for (int y = start; y <= end; ++y) {
        drawPixel(x, y, color);
    }
}

void Oled::drawBuffer(const float* data, uint16_t size) {
    this->fill(false); 
    int lastY = -1;
    
    const int topLimit = 8;
    const int bottomLimit = 63;
    const float centerY = 35.5f;
    const float amplitudeScale = 27.0f; // Leaves ~1px margin within the active area

    for (uint16_t x = 0; x < size && x < 128; x++) {
        int y = static_cast<int>(centerY - (data[x] * amplitudeScale));
        
        if (y < topLimit) y = topLimit;
        if (y > bottomLimit) y = bottomLimit;

        if (lastY != -1) {
            drawVLine(x, lastY, y, true);
        } else {
            drawPixel(x, y, true);
        }
        lastY = y;
    }
}

void Oled::drawLine(int x0, int y0, int x1, int y1, bool color) {
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;

    while (true) {
        drawPixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void Oled::drawChar(int x, int y, char c, bool color) {
    if (c > 127) return; 

    uint16_t fontIdx = c * 5; 
    
    for (int col = 0; col < 5; col++) {
        uint8_t line = font[fontIdx + col];
        for (int row = 0; row < 7; row++) {
            if (line & (1 << row)) {
                drawPixel(x + col, y + row, color);
            }
        }
    }
}

void Oled::drawString(int x, int y, const char* str, bool color) {
    while (*str) {
        drawChar(x, y, *str++, color);
        x += 6; // 5 pixels for char + 1 pixel for spacing
        if (x > 122) break; // Screen wrap protection
    }
}
