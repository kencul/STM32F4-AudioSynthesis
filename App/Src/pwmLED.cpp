#include "pwmLed.h"
#include <algorithm>

PWMLed::PWMLed(uint8_t address) : _deviceAddr(address << 1) {}

uint8_t PWMLed::init() {
    uint8_t status = 0;

    // Wake oscillator by clearing SLEEP bit
    // Enable Auto-Increment for sequential register writes
    status += writeRegister(Reg::MODE1, Bit::AI);
    
    // Default to all LEDs off
    status += ledAllOn(false);

    return status;
}

uint8_t PWMLed::setChannel(uint8_t index, float brightness) {
    if (index > 15) return 1;

    // Clamp brightness and convert to 12-bit (0-4095) 
    brightness = std::max(0.0f, std::min(1.0f, brightness));
    uint16_t offValue = static_cast<uint16_t>(brightness * 4095);

    // 4 regs per channel: ON_L, ON_H, OFF_L, OFF_H 
    uint8_t regAddr = Reg::LED0_ON_L + (index * 4);
    
    uint8_t data[4];
    data[0] = 0x00;           // ON Low
    data[1] = 0x00;           // ON High
    data[2] = offValue & 0xFF; // OFF Low
    data[3] = (offValue >> 8) & 0x0F; // OFF High

    if (HAL_I2C_Mem_Write(&hi2c1, _deviceAddr, regAddr, 1, data, 4, HAL_MAX_DELAY) != HAL_OK) {
        return 1;
    }
    return 0;
}

uint8_t PWMLed::updateVoices(const std::array<float, 8>& levels) {
    uint8_t data[32]; // 8 channels * 4 registers
    
    for (uint8_t i = 0; i < 8; ++i) {
        // Apply Visual Curve (Level^2) and convert to 12-bit
        float val = std::clamp(levels[i], 0.0f, 1.0f);
        uint16_t offValue = static_cast<uint16_t>(val * val * 4095.0f);
        
        uint8_t base = i * 4;
        data[base + 0] = 0x00;           // ON L
        data[base + 1] = 0x00;           // ON H
        data[base + 2] = offValue & 0xFF; // OFF L
        data[base + 3] = (offValue >> 8) & 0x0F; // OFF H
    }

    // Start at LED0_ON_L (0x06) and write all 32 bytes in one burst
    if (HAL_I2C_Mem_Write(&hi2c1, _deviceAddr, 0x06, 1, data, 32, 10) != HAL_OK) {
        return 1;
    }
    return 0;
}

uint8_t PWMLed::ledAllOn(bool state) {
    uint8_t status = 0;
    status += writeRegister(Reg::ALL_LED_ON_H, state ? Bit::FULL_ON : 0x00);
    status += writeRegister(Reg::ALL_LED_OFF_H, state ? 0x00 : Bit::FULL_OFF);
    return status;
}

uint8_t PWMLed::writeRegister(uint8_t reg, uint8_t val) {
    return (HAL_I2C_Mem_Write(&hi2c1, _deviceAddr, reg, 1, &val, 1, HAL_MAX_DELAY) == HAL_OK) ? 0 : 1;
}
