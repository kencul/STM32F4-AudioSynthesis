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
#include "midiBuffer.h"

#define SAMPLE_RATE 48000

Codec codec;
Osc osc(720, 0.5, BUFFER_SIZE, SAMPLE_RATE);
int16_t buffer[BUFFER_SIZE] = {0};

PotBank hardwarePots(&hadc1, &hadc2, GPIOE, MUX_A_Pin, GPIOE, MUX_B_Pin);

extern MidiBuffer gMidiBuffer;

extern "C" void cpp_main() {
    // osc init
	osc.process(buffer, NUM_FRAMES);
	osc.process(&buffer[NUM_FRAMES*2], NUM_FRAMES);
    osc.setAmplitude(0.5f);
    
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
    uint16_t cf = 20;
    HAL_GPIO_TogglePin(GREEN_LED_GPIO_Port, GREEN_LED_Pin);
	while(1){
        static uint32_t lastHeartbeat = 0;
        if(HAL_GetTick() - lastHeartbeat > 200) {
            HAL_GPIO_TogglePin(GREEN_LED_GPIO_Port, GREEN_LED_Pin);
            lastHeartbeat = HAL_GetTick();
        }
        
		// if (HAL_GPIO_ReadPin(USER_BUTTON_GPIO_Port, USER_BUTTON_Pin) == GPIO_PIN_SET || HAL_GPIO_ReadPin(BUTTON_GPIO_Port, BUTTON_Pin) == GPIO_PIN_RESET) {
        //     if(!notePlaying){
        //         notePlaying = true;
        //         //osc.setAmplitude(0.0001f);
        //     }
        // } else {
        //     if(notePlaying){
        //         // osc.noteOff();
        //         // HAL_GPIO_WritePin(ORANGE_LED_GPIO_Port, ORANGE_LED_Pin, GPIO_PIN_RESET);
        //         notePlaying = false;
        //         // //osc.setAmplitude(0.f);
        //     }
        // }
	
        if (hardwarePots.anyChanged()) {
            for (uint8_t i = 0; i < 8; i++) {
                if (hardwarePots.hasChanged(i)) {
                    handleParamChange(i);
                }
            }
        }

        handleMidi();
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

void handleParamChange(uint8_t index) {
    Pot& p = hardwarePots.pots[index];
    switch(index) {
        case 0: codec.setVolume(p.scaleExp(0.f, 1.f)); break;
        case 1: osc.setCutOff(p.scaleExp(20.f, 15000.f, 2.f)); break;
        case 2: osc.setResonance(p.getFloat()); break;
        case 3: osc.setMorph(p.getFloat()); break;
        case 4: osc.setAttack(p.scaleExp(0.f, 3.f, 2.f)); break;
        case 5: osc.setDecay(p.scaleExp(0.f, 3.f, 2.f)); break;
        case 6: osc.setSustain(p.scaleLin(0.f, 1.f)); break;
        case 7: osc.setRelease(p.scaleExp(0.f, 3.f, 2.f)); break;
    }
}

void handleMidi() {
    MidiPacket packet;
    
    // Process all pending packets in the buffer
    while (gMidiBuffer.pop(packet)) {
        // packet.data[0] is the USB header (Cable Number / CIN)
        uint8_t status   = packet.data[1];
        uint8_t data1    = packet.data[2];
        uint8_t data2    = packet.data[3];
        uint8_t message  = status & 0xF0;

        switch (message) {
            case 0x90: // Note On
                if (data2 > 0) {
                    osc.noteOn(data1); // data1 is MIDI note
                    HAL_GPIO_WritePin(ORANGE_LED_GPIO_Port, ORANGE_LED_Pin, GPIO_PIN_SET);
                } else {
                    osc.noteOff(); // Velocity 0 often means Note Off
                    HAL_GPIO_WritePin(ORANGE_LED_GPIO_Port, ORANGE_LED_Pin, GPIO_PIN_RESET);
                }
                break;

            case 0x80: // Note Off
                osc.noteOff();
                HAL_GPIO_WritePin(ORANGE_LED_GPIO_Port, ORANGE_LED_Pin, GPIO_PIN_RESET);
                break;

            case 0xB0: // Control Change (CC)
                // if (data1 == 74) { // Standard Brightness/Cutoff CC
                //     // Scale 0-127 to a useful frequency range
                //     float cutoff = (data2 / 127.0f) * 8000.0f + 20.0f;
                //     osc.getFilter().setCutoff(cutoff);
                // }
                // else if (data1 == 71) { // Standard Resonance CC
                //     float res = (data2 / 127.0f);
                //     osc.getFilter().setResonance(res);
                // }
                break;
        }
    }
}
