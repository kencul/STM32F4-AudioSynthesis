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
#include "main.h"
#include <cmath>

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
    int on = 0;
    uint32_t lastPotVal = 0.f, lastPotVal2 = 0.f;
	while(1){
		if (HAL_GPIO_ReadPin(USER_BUTTON_GPIO_Port, USER_BUTTON_Pin) == GPIO_PIN_SET || HAL_GPIO_ReadPin(BUTTON_GPIO_Port, BUTTON_Pin) == GPIO_PIN_RESET) {
            // Button is HELD: Set amplitude to 0.5
            //osc.setAmplitude(0.01f);
            on = 1;
            // Optional: Turn on Orange LED while playing
            HAL_GPIO_WritePin(ORANGE_LED_GPIO_Port, ORANGE_LED_Pin, GPIO_PIN_SET);
        } else {
            // Button is RELEASED: Mute
            //osc.setAmplitude(0.0f);
            on = 0;
            HAL_GPIO_WritePin(ORANGE_LED_GPIO_Port, ORANGE_LED_Pin, GPIO_PIN_RESET);
        }

        // Reading analog signal
        HAL_ADC_Start(&hadc1);
        if (HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK) {
            // Get the value (0-4095)
            uint32_t adc_val = HAL_ADC_GetValue(&hadc1);
            uint32_t diff = (adc_val > lastPotVal) ? (adc_val - lastPotVal) : (lastPotVal - adc_val);
            if(diff > 40){
                lastPotVal = adc_val;
                adc_val = adc_val >> 7;
                osc.setFreq(adc_val + 48);
            }
        }
        HAL_ADC_Stop(&hadc1);

        HAL_ADC_Start(&hadc2);
        if (HAL_ADC_PollForConversion(&hadc2, 10) == HAL_OK) {
            // Get the value (0-4095)
            uint32_t adc_val = HAL_ADC_GetValue(&hadc2);
            uint32_t diff = (adc_val > lastPotVal2) ? (adc_val - lastPotVal2) : (lastPotVal2 - adc_val);
            if(diff > 20){
                float potNormalized = (float)adc_val / 4095.0f;
                osc.setMorph(potNormalized);
                lastPotVal2 = adc_val;
            }
        }
        HAL_ADC_Stop(&hadc2);
        
        osc.setAmplitude(0.01f * on);
        HAL_Delay(5);
	}
}

extern "C" void HAL_I2S_TxHalfCpltCallback(I2S_HandleTypeDef *hi2s) {
    osc.process(buffer, NUM_FRAMES);
}

extern "C" void HAL_I2S_TxCpltCallback(I2S_HandleTypeDef *hi2s) {
    osc.process(&buffer[NUM_FRAMES*2],NUM_FRAMES);
}
