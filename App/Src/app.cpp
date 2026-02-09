/*
 * app.cpp
 *
 *  Created on: Jan 27, 2026
 *      Author: Ken
 */

#include "codec.h"
#include <cstdint>
#include <vector>
#include "app.h"
#include "osc.h"
#include "main.h"
#include <cmath>
#include "potBank.h"
#include "tim.h"
#include "midiBuffer.h"
#include "pwmLed.h"
#include "voiceManager.h"
#include "oled.h"
#include "waveforms.h"

#define SAMPLE_RATE 48000

Codec codec;
PWMLed ledController;
Oled oled;

__attribute__((section(".ccmram"))) VoiceManager voiceManager(SAMPLE_RATE, BUFFER_SIZE);

int16_t buffer[BUFFER_SIZE] = {0};

PotBank hardwarePots(&hadc1, &hadc2, GPIOE, MUX_A_Pin, GPIOE, MUX_B_Pin);

extern MidiBuffer gMidiBuffer;

extern "C" void cpp_main() {
    // init led controller
    if (ledController.init() != 0) {
        while(1);
    }

    ledController.ledAllOn(true);
    HAL_GPIO_TogglePin(GREEN_LED_GPIO_Port, GREEN_LED_Pin);
    HAL_Delay(100);
    
    // osc init
    voiceManager.process(buffer, NUM_FRAMES);
    voiceManager.process(&buffer[NUM_FRAMES * 2], NUM_FRAMES);
    
    if (oled.init() == 0) {
        oled.fill(true); // Fill buffer with white
        oled.update();   // Send to screen via DMA
    } else {
        while(1);
    }
    // codec init
	auto status = codec.init(buffer, BUFFER_SIZE);
	if(status){
        while(1);
	}
    
    // init adc
    hardwarePots.init();
    
    // Init timer for adc scanning
    HAL_TIM_Base_Start_IT(&htim4);
    
    // startup sequence
    ledController.ledAllOn(false);
    HAL_GPIO_TogglePin(GREEN_LED_GPIO_Port, GREEN_LED_Pin);
    HAL_Delay(50);
    ledController.ledAllOn(true);
    HAL_GPIO_TogglePin(GREEN_LED_GPIO_Port, GREEN_LED_Pin);
    HAL_Delay(50);
    ledController.ledAllOn(false);
    HAL_GPIO_TogglePin(GREEN_LED_GPIO_Port, GREEN_LED_Pin);
    HAL_Delay(50);
    ledController.ledAllOn(true);
    HAL_GPIO_TogglePin(GREEN_LED_GPIO_Port, GREEN_LED_Pin);
    HAL_Delay(50);
    ledController.ledAllOn(false);
    HAL_GPIO_TogglePin(GREEN_LED_GPIO_Port, GREEN_LED_Pin);

	while(1){
        static uint32_t lastHeartbeat = 0;
        if(HAL_GetTick() - lastHeartbeat > 200) {
            HAL_GPIO_TogglePin(GREEN_LED_GPIO_Port, GREEN_LED_Pin);
            lastHeartbeat = HAL_GetTick();
        }
	
        if (hardwarePots.anyChanged()) {
            for (uint8_t i = 0; i < 8; i++) {
                if (hardwarePots.hasChanged(i)) {
                    handleParamChange(i);
                }
            }
        }

        // LED Voice indicator update
        static uint32_t lastLedUpdate = 0;
        if(HAL_GetTick() - lastLedUpdate > 20) {
            lastLedUpdate = HAL_GetTick();
            
            // Grab all levels from VoiceManager at once
            std::array<float, 8> levels;
            for(uint8_t i = 0; i < 8; ++i) {
                levels[i] = voiceManager.getVoiceLevel(i);
            }
            
            // Burst update the I2C LED controller
            ledController.updateVoices(levels);
        }

        static uint32_t lastDebounceTime = 0;
        if (HAL_GetTick() - lastDebounceTime > 20) { // Check state every 20ms
            lastDebounceTime = HAL_GetTick();

            // Button A
            static bool stableStateA = true;
            bool rawA = HAL_GPIO_ReadPin(BUTTON_A_GPIO_Port, BUTTON_A_Pin);
            if (rawA != stableStateA) {
                if (rawA == GPIO_PIN_RESET) cycleWaveform(0); // Pressed
                stableStateA = rawA;
            }

            // Button B
            static bool stableStateB = true;
            bool rawB = HAL_GPIO_ReadPin(BUTTON_B_GPIO_Port, BUTTON_B_Pin);
            if (rawB != stableStateB) {
                if (rawB == GPIO_PIN_RESET) cycleWaveform(1); // Pressed
                stableStateB = rawB;
            }
        }

        handleMidi();
    }
}

extern "C" void HAL_I2S_TxHalfCpltCallback(I2S_HandleTypeDef *hi2s) {
    std::fill(buffer, buffer + (NUM_FRAMES * 2), 0);
    voiceManager.process(buffer, NUM_FRAMES);
}

extern "C" void HAL_I2S_TxCpltCallback(I2S_HandleTypeDef *hi2s) {
    std::fill(buffer + (NUM_FRAMES * 2), buffer + (NUM_FRAMES * 4), 0);
    voiceManager.process(&buffer[NUM_FRAMES*2], NUM_FRAMES);
}

extern "C" void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc) {
    hardwarePots.handleInterrupt(hadc);
}

extern "C" void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM4) {
        hardwarePots.startScan();
    }
}

void refreshOledWaveform() {
    float oledWaveform[128];
    float currentMorph = hardwarePots.pots[3].getFloat(); 
    
    Osc::getMorphedPreview(oledWaveform, 128, currentMorph);

    oled.drawBuffer(oledWaveform, 128);
    oled.update();
}

void handleParamChange(uint8_t index) {
    Pot& p = hardwarePots.pots[index];
    switch(index) {
        case 0: codec.setVolume(p.scaleExp(0.f, 1.f)); break;
        case 1: voiceManager.setCutoff(p.scaleExp(20.f, 15000.f, 2.f)); break;
        case 2: voiceManager.setResonance(p.getFloat()); break;
        case 3: 
        {
            float newMorph = p.getFloat();
            voiceManager.setMorph(newMorph); 
            refreshOledWaveform();
        }
            break;
        case 4: voiceManager.setAttack(p.scaleExp(0.f, 3.f, 2.f)); break;
        case 5: voiceManager.setDecay(p.scaleExp(0.f, 3.f, 2.f)); break;
        case 6: voiceManager.setSustain(p.scaleLin(0.f, 1.f)); break;
        case 7: voiceManager.setRelease(p.scaleExp(0.f, 3.f, 2.f)); break;
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
                    voiceManager.noteOn(data1); // data1 is MIDI note
                    HAL_GPIO_WritePin(ORANGE_LED_GPIO_Port, ORANGE_LED_Pin, GPIO_PIN_SET);
                } else {
                    voiceManager.noteOff(data1); // Velocity 0 often means Note Off
                    HAL_GPIO_WritePin(ORANGE_LED_GPIO_Port, ORANGE_LED_Pin, GPIO_PIN_RESET);
                }
                break;

            case 0x80: // Note Off
                voiceManager.noteOff(data1);
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


void cycleWaveform(uint8_t slot) {
    uint8_t nextIdx = Osc::getActiveIdx(slot);
    uint8_t otherIdx = Osc::getActiveIdx(slot == 0 ? 1 : 0);

    // Increment and skip if it matches the other slot
    do {
        nextIdx = (nextIdx + 1) % WAVE_COUNT;
    } while (nextIdx == otherIdx);

    __disable_irq();
    Osc::loadWaveform(nextIdx, slot);
    __enable_irq();
    refreshOledWaveform();
}
