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
#include "potBank.h"
#include "tim.h"

#define SAMPLE_RATE 48000

Codec codec;
Osc osc(720, 0.01, BUFFER_SIZE, SAMPLE_RATE);
int16_t buffer[BUFFER_SIZE] = {0};

PotBank hardwarePots(&hadc1, &hadc2, GPIOE, MUX_A_Pin, GPIOE, MUX_B_Pin);

extern "C" void cpp_main() {
    // osc init
	osc.process(buffer, NUM_FRAMES);
	osc.process(&buffer[NUM_FRAMES*2], NUM_FRAMES);
    osc.setAmplitude(0.01f);
    
    // codec init
	auto status = codec.init(buffer, BUFFER_SIZE);
	if(status){
        while(1);
	}
    
    // init adc
    hardwarePots.init();

    // Init timer for adc scanning
    HAL_TIM_Base_Start_IT(&htim4);
    //uint32_t lastPotVal = 0.f, lastPotVal2 = 0.f;
    
    bool notePlaying = false;
    HAL_GPIO_TogglePin(GREEN_LED_GPIO_Port, GREEN_LED_Pin);
	while(1){
        static uint32_t lastHeartbeat = 0;
        if(HAL_GetTick() - lastHeartbeat > 200) {
            HAL_GPIO_TogglePin(GREEN_LED_GPIO_Port, GREEN_LED_Pin);
            lastHeartbeat = HAL_GetTick();
        }
        
		if (HAL_GPIO_ReadPin(USER_BUTTON_GPIO_Port, USER_BUTTON_Pin) == GPIO_PIN_SET || HAL_GPIO_ReadPin(BUTTON_GPIO_Port, BUTTON_Pin) == GPIO_PIN_RESET) {
            if(!notePlaying){
                osc.noteOn();
                HAL_GPIO_WritePin(ORANGE_LED_GPIO_Port, ORANGE_LED_Pin, GPIO_PIN_SET);
                notePlaying = true;
            }
        } else {
            if(notePlaying){
                osc.noteOff();
                HAL_GPIO_WritePin(ORANGE_LED_GPIO_Port, ORANGE_LED_Pin, GPIO_PIN_RESET);
                notePlaying = false;
            }
        }

        // // Reading analog signal
        // HAL_ADC_Start(&hadc1);
        // if (HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK) {
        //     // Get the value (0-4095)
        //     uint32_t adc_val = HAL_ADC_GetValue(&hadc1);
        //     uint32_t diff = (adc_val > lastPotVal) ? (adc_val - lastPotVal) : (lastPotVal - adc_val);
        //     if(diff > 40){
        //         lastPotVal = adc_val;
        //         uint32_t midiNote = (adc_val >> 7) + 48;
        //         osc.setFreq(midiNote);
        //     }
        // }
        // HAL_ADC_Stop(&hadc1);

        // HAL_ADC_Start(&hadc2);
        // if (HAL_ADC_PollForConversion(&hadc2, 10) == HAL_OK) {
        //     // Get the value (0-4095)
        //     uint32_t adc_val = HAL_ADC_GetValue(&hadc2);
        //     uint32_t diff = (adc_val > lastPotVal2) ? (adc_val - lastPotVal2) : (lastPotVal2 - adc_val);
        //     if(diff > 20){
        //         float potNormalized = (float)adc_val / 4095.0f;
        //         osc.setMorph(potNormalized);
        //         lastPotVal2 = adc_val;
        //     }
        // }
        // HAL_ADC_Stop(&hadc2);
        
        // HAL_Delay(5);
	
        if (hardwarePots.anyChanged()) {
            for (uint8_t i = 0; i < 8; i++) {
                if (hardwarePots.hasChanged(i)) {
                    handleParamChange(i, hardwarePots.pots[i].getFloat(), hardwarePots.pots[i].getRaw());
                }
            }
        }
    }
}

extern "C" void HAL_I2S_TxHalfCpltCallback(I2S_HandleTypeDef *hi2s) {
    std::fill(buffer, buffer + (NUM_FRAMES * 2), 0);
    osc.process(buffer, NUM_FRAMES);
}

extern "C" void HAL_I2S_TxCpltCallback(I2S_HandleTypeDef *hi2s) {
    std::fill(buffer + (NUM_FRAMES * 2), buffer + (NUM_FRAMES * 4), 0);
    osc.process(&buffer[NUM_FRAMES*2],NUM_FRAMES);
}

extern "C" void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc) {
    hardwarePots.handleInterrupt(hadc);
}

extern "C" void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM4) {
        hardwarePots.startScan();
    }
}

void handleParamChange(uint8_t index, float value, uint16_t rawValue) {
    switch(index) {
        case 0: osc.setMorph(value); break;
        case 1: osc.setFreq((uint32_t)((rawValue >> 7) + 48)); break; // Example mapping
        case 4: osc.setAttack(value * 0.5); break;
        case 5: osc.setDecay(value* 0.5); break;
        case 6: osc.setSustain(value); break;
        case 7: osc.setRelease(value * 0.5); break;
    }
}
