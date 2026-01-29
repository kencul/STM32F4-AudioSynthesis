/*
 * app.cpp
 *
 *  Created on: Jan 27, 2026
 *      Author: Ken
 */

#include "codec.h"
#include <stdio.h>
#include <cstdint>
#include <vector>
#include "app.h"
#include "osc.h"
#include "i2s.h"
#include "main.h"

#define SAMPLE_RATE 48000

using namespace std;
Codec codec;
Osc osc(720, 0.01, BUFFER_SIZE, SAMPLE_RATE);
int16_t buffer[BUFFER_SIZE] = {0};

extern "C" void cpp_main() {
	osc.process(buffer, NUM_FRAMES);
	osc.process(&buffer[NUM_FRAMES*2], NUM_FRAMES);
	auto status = codec.init(buffer, BUFFER_SIZE);
	if(status){
		while(1);
	}
	while(1){
		if (HAL_GPIO_ReadPin(USER_BUTTON_GPIO_Port, USER_BUTTON_Pin) == GPIO_PIN_SET) {
            // Button is HELD: Set amplitude to 0.5
            osc.setAmplitude(0.01f);
            
            // Optional: Turn on Orange LED while playing
            HAL_GPIO_WritePin(ORANGE_LED_GPIO_Port, ORANGE_LED_Pin, GPIO_PIN_SET);
        } else {
            // Button is RELEASED: Mute
            osc.setAmplitude(0.0f);
            HAL_GPIO_WritePin(ORANGE_LED_GPIO_Port, ORANGE_LED_Pin, GPIO_PIN_RESET);
        }
	}
}

extern "C" void HAL_I2S_TxHalfCpltCallback(I2S_HandleTypeDef *hi2s) {
    osc.process(buffer, NUM_FRAMES);
}

extern "C" void HAL_I2S_TxCpltCallback(I2S_HandleTypeDef *hi2s) {
    osc.process(&buffer[NUM_FRAMES*2],NUM_FRAMES);
}
