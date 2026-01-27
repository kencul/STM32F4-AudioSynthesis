/*
 * Codec.cpp
 *
 *  Created on: Jan 27, 2026
 *      Author: Ken
 */

#include "Codec.h"

Codec::Codec() {
	// TODO Auto-generated constructor stub

}

Codec::~Codec() {
	// TODO Auto-generated destructor stub
}

uint8_t Codec::init() {
	uint8_t status = 0;
	HAL_GPIO_WritePin(CODEC_RESET_GPIO_Port, CODEC_RESET_Pin, GPIO_PIN_SET);
	// Address 4: Power Control 2, Data: 10101111 (Headphone on always, speakers off always)
	status += write(0x04, 0xaf);
	// Address 6: Interface Control 2, Data: 00000111 (Slave mode, normal polarity (doesn't matter), DSP mode off, I2S Format, 16bit data)
	status += write(0x06, 0x07);
	// Address 2: Power Control 1, Data: 10011110 (Powered up)
	status += write(0x02, 0x9e);

	return status;
}

uint8_t Codec::write(uint8_t reg, uint8_t val) {
	HAL_StatusTypeDef status = HAL_OK;

	status = HAL_I2C_Mem_Write(&hi2c1, AUDIO_I2C_ADDR, reg, 1, &val, sizeof(val), HAL_MAX_DELAY);
	if (status != HAL_OK)
		return 1;
	return 0;
}
