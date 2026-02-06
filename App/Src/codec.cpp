/*
 * Codec.cpp
 *
 *  Created on: Jan 27, 2026
 *      Author: Ken
 */

#include "codec.h"
#include "i2c.h"
#include "gpio.h"
#include "i2s.h"
#include <cstdint>

Codec::Codec() {
	// TODO Auto-generated constructor stub

}

Codec::~Codec() {
	// TODO Auto-generated destructor stub
}

uint8_t Codec::init(int16_t * buffer, size_t bufferSize) {
	uint8_t status = 0;
	HAL_GPIO_WritePin(CODEC_RESET_GPIO_Port, CODEC_RESET_Pin, GPIO_PIN_SET);
	// Address 4: Power Control 2, Data: 10101111 (Headphone on always, speakers off always)
	status += write(0x04, 0xaf);
	// Address 6: Interface Control 2, Data: 00000111 (Slave mode, normal polarity (doesn't matter), DSP mode off, I2S Format, 16bit data)
	status += write(0x06, 0x07);
	
	status += setVolume(0.7f);
	
	// Manufacturer provided initialization
	status += write(0x00, 0x99);
    status += write(0x47, 0x80);
    status += write(0x32, 0xBB);
    status += write(0x32, 0x3B);
    status += write(0x00, 0x00);
	
	HAL_I2S_Transmit_DMA(&hi2s3, (uint16_t *)buffer, bufferSize);
	
	// Address 2: Power Control 1, Data: 10011110 (Powered up)
	status += write(0x02, 0x9e);
	
	// Digital soft ramp and digital zero cross for smooth volume changes
	status += write(0x0E, 0x03);
	return status;
}

uint8_t Codec::write(uint8_t reg, uint8_t val) {
	HAL_StatusTypeDef status = HAL_OK;
	
	status = HAL_I2C_Mem_Write(&hi2c1, AUDIO_I2C_ADDR, reg, 1, &val, sizeof(val), HAL_MAX_DELAY);
	if (status != HAL_OK)
	return 1;
return 0;
}

uint8_t Codec::setVolume(float volume) {
	static uint8_t lastVolume = 0;
    uint8_t regVal;
	
	// Codec headphone output: 0 = 0dB, 0x01 = mute
    // Hard Mute near bottom
    if (volume <= 0.001f) {
		regVal = 0x01; 
    } 
    // Full scale at the very top
    else if (volume >= 1.0f) {
		regVal = 0x00;
    }
    else {
		regVal = (uint8_t)((255-120) * volume) + 120; // 52 = -96dB aka effective silence
    }

	if(lastVolume == regVal) return 0;
	lastVolume = regVal;
	//Freeze the registers so the changes don't apply yet
	uint8_t status = 0;
	// status += write(0x0E, 0x03);
    // status += write(0x0E, 0x03 | 0x08);
    status += write(0x22, regVal); // Headphone Left
    status += write(0x23, regVal); // Headphone Right
	// status += write(0x0E, 0x03);
    return status;
}
