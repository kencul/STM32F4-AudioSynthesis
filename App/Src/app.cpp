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
#include "csound.h"

#define SAMPLE_RATE 48000

volatile bool audio_ready = false;

using namespace std;
Codec codec;
// Osc osc(720, 0.01, BUFFER_SIZE, SAMPLE_RATE);
int16_t buffer[BUFFER_SIZE] = {0};

CSOUND* cs;

const char* csd_template = R"csd(
    <CsoundSynthesizer>
    <CsOptions>
    -n
    </CsOptions>
    <CsInstruments>
    sr = 48000
    ksmps = 256
    nchnls = 2
    0dbfs = 1.0
    
    giSine ftgen 1, 0, 16384, 10, 1
    
    instr 1
      aout oscil 0.5, 440, 1
      outs aout, aout
    endin
    </CsInstruments>
    <CsScore>
    i 1 0 3600
    </CsScore>
    </CsoundSynthesizer>
    )csd";


extern "C" void cpp_main() {
    
    // cs = csoundCreate(NULL, NULL);

    //if (cs == NULL) while(1); // Trap: Out of memory
    
    // int result = csoundCompileCSD(cs, csd_template, 1, 0);
    
    // if (result != 0) {
    //     // Trap: Syntax error in CSD or missing opcodes
    //     while(1); 
    // }

    // csoundStart(cs);

	// // osc.process(buffer, NUM_FRAMES);
	// // osc.process(&buffer[NUM_FRAMES*2], NUM_FRAMES);
	// auto status = codec.init(buffer, BUFFER_SIZE);
	// if(status){
	// 	while(1);
	// }
	// while(1){
	// 	if (HAL_GPIO_ReadPin(USER_BUTTON_GPIO_Port, USER_BUTTON_Pin) == GPIO_PIN_SET) {
    //         // Button is HELD: Set amplitude to 0.5
    //         osc.setAmplitude(0.01f);
            
    //         // Optional: Turn on Orange LED while playing
             HAL_GPIO_WritePin(ORANGE_LED_GPIO_Port, ORANGE_LED_Pin, GPIO_PIN_SET);
    //     } else {
    //         // Button is RELEASED: Mute
    //         osc.setAmplitude(0.0f);
    HAL_Delay(300);
    HAL_GPIO_WritePin(ORANGE_LED_GPIO_Port, ORANGE_LED_Pin, GPIO_PIN_RESET);
    HAL_Delay(300);
    //     }
	// }
}

extern "C" void HAL_I2S_TxHalfCpltCallback(I2S_HandleTypeDef *hi2s) {
    if (audio_ready && cs != nullptr) {
        int perform_status = csoundPerformKsmps(cs);
        
        if (perform_status == 0) {
            // 3. Get the calculated samples (floats)
            const MYFLT* spout = csoundGetSpout(cs);
            
            // 4. Convert and copy into the DMA buffer
            // Csound generates interleaved audio [L, R, L, R...]
            for (int i = 0; i < 64; i++) {
                // Convert float -1.0..1.0 to signed 16-bit
                float sample = spout[i];
                
                // Hard clipping for safety
                if (sample > 1.0f) sample = 1.0f;
                if (sample < -1.0f) sample = -1.0f;
                
                buffer[i] = (int16_t)(sample * 32767.0f);
            }
        }
    }
}

extern "C" void HAL_I2S_TxCpltCallback(I2S_HandleTypeDef *hi2s) {
    if (audio_ready && cs != nullptr) {
        int perform_status = csoundPerformKsmps(cs);
        
        if (perform_status == 0) {
            // 3. Get the calculated samples (floats)
            const MYFLT* spout = csoundGetSpout(cs);
            
            // 4. Convert and copy into the DMA buffer
            // Csound generates interleaved audio [L, R, L, R...]
            for (int i = 0; i < 64; i++) {
                // Convert float -1.0..1.0 to signed 16-bit
                float sample = spout[i];
                
                // Hard clipping for safety
                if (sample > 1.0f) sample = 1.0f;
                if (sample < -1.0f) sample = -1.0f;
                
                buffer[i + (NUM_FRAMES * 2)] = (int16_t)(sample * 32767.0f);
            }
        }
    }
}
