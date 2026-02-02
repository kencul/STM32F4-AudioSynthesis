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

        uint32_t adc_val = 0;

        // Reading analog signal
        HAL_ADC_Start(&hadc1);
        if (HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK) {
            // Get the value (0-4095)
            adc_val = HAL_ADC_GetValue(&hadc1);
            float potVal = (float)adc_val / 4095.0f;
            osc.setAmplitude(0.01f * on);
            osc.setFreq(40 + 2000*potVal);
        }
        // HAL_ADC_Stop(&hadc1);
        // uint32_t pot1 = Read_ADC_Channel(ADC_CHANNEL_1);  // PA1
        // uint32_t pot2 = Read_ADC_Channel(ADC_CHANNEL_14); // PC4

        // // Example: Pot 1 controls Frequency, Pot 2 controls Volume
        // float freq = 200.0f + ((float)pot1 / 4095.0f * 800.0f);
        // float vol  = (float)pot2 / 4095.0f * 0.1f;

        // osc.setFreq(40.f + 1500.f* freq);
        // osc.setAmplitude(vol * 0.01f);
	}
}

extern "C" void HAL_I2S_TxHalfCpltCallback(I2S_HandleTypeDef *hi2s) {
    osc.process(buffer, NUM_FRAMES);
}

extern "C" void HAL_I2S_TxCpltCallback(I2S_HandleTypeDef *hi2s) {
    osc.process(&buffer[NUM_FRAMES*2],NUM_FRAMES);
}

uint32_t Read_ADC_Channel(uint32_t channel){
    ADC_ChannelConfTypeDef sConfig = {0};
    
    sConfig.Channel = channel;
    sConfig.Rank = 1;
    sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
    
    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) {
        Error_Handler();
    }

    HAL_ADC_Start(&hadc1);
    HAL_ADC_PollForConversion(&hadc1, 10);
    uint32_t val = HAL_ADC_GetValue(&hadc1);
    HAL_ADC_Stop(&hadc1);
    
    return val;
}
