#include "pwmLed.h"

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

uint8_t PWMLed::ledAllOn(bool state) {
    uint8_t status = 0;

    if (state) {
        // Set bit 4 of ALL_LED_ON_H for full-on mode
        // LEDn_OFF registers are ignored in this state
        status += writeRegister(Reg::ALL_LED_ON_L, 0x00);
        status += writeRegister(Reg::ALL_LED_ON_H, Bit::FULL_ON);
        status += writeRegister(Reg::ALL_LED_OFF_L, 0x00);
        status += writeRegister(Reg::ALL_LED_OFF_H, 0x00);
    } else {
        // Set bit 4 of ALL_LED_OFF_H for full-off mode
        status += writeRegister(Reg::ALL_LED_ON_L, 0x00);
        status += writeRegister(Reg::ALL_LED_ON_H, 0x00);
        status += writeRegister(Reg::ALL_LED_OFF_L, 0x00);
        status += writeRegister(Reg::ALL_LED_OFF_H, Bit::FULL_OFF);
    }

    return status;
}

uint8_t PWMLed::writeRegister(uint8_t reg, uint8_t val) {
    return (HAL_I2C_Mem_Write(&hi2c1, _deviceAddr, reg, 1, &val, 1, HAL_MAX_DELAY) == HAL_OK) ? 0 : 1;
}
