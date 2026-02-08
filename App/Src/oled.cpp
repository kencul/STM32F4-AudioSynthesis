#include "oled.h"
#include <cstring>

Oled::Oled(uint8_t address) : _addr(address << 1) {
    std::memset(_buffer, 0x00, sizeof(_buffer));
}

uint8_t Oled::init() {
    uint8_t status = 0;
    
    // Fundamental Initialization Sequence
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
    // Check if the I2C bus is currently busy with OLED DMA or Codec tasks
    if (hi2c1.State != HAL_I2C_STATE_READY) return 1;

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

    for (uint16_t x = 0; x < size && x < 128; x++) {
        // Use 31 as center, 31 as scale to reach 0 and 62
        // Adding 0.5f before casting provides a "round to nearest" behavior
        int y = 31 - static_cast<int>(data[x] * 31.0f + 0.5f);
        
        // Clamp to absolute bounds just in case of float precision spikes
        if (y < 0) y = 0;
        if (y > 63) y = 63;

        if (lastY != -1) {
            drawVLine(x, lastY, y, true);
        } else {
            drawPixel(x, y, true);
        }
        lastY = y;
    }
}
