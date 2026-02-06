/*
 * app.h
 *
 *  Created on: Jan 27, 2026
 *      Author: Ken
 */

#ifndef SRC_APP_H_
#define SRC_APP_H_

#include <stdio.h>
#include "i2s.h"
#include "adc.h"

#define NUM_FRAMES 256
#define BUFFER_SIZE (NUM_FRAMES * 4)

#ifdef __cplusplus
extern "C" {
#endif

// This is the bridge function.
// It is defined in app.cpp and called in main.c
void cpp_main(void);

void HAL_I2S_TxHalfCpltCallback(I2S_HandleTypeDef *hi2s);
void HAL_I2S_TxCpltCallback(I2S_HandleTypeDef *hi2s);
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc);
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim);

#ifdef __cplusplus
}
#endif

void handleParamChange(uint8_t index);
void handleMidi();

#endif /* SRC_APP_H_ */
