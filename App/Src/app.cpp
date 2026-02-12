/*
 * app.cpp
 *
 *  Created on: Jan 27, 2026
 *      Author: Ken
 */

#include <cstdint>
#include <vector>
#include <cstdio>
#include <cmath>

#include "app.h"
#include "codec.h"
#include "osc.h"
#include "main.h"
#include "potBank.h"
#include "tim.h"
#include "midiBuffer.h"
#include "pwmLed.h"
#include "voiceManager.h"
#include "oled.h"
#include "waveforms.h"
#include "adsrVisualizer.h"
#include "filterVisualizer.h"
#include "constants.h"

Codec codec;
PWMLed ledController;
Oled oled;

__attribute__((section(".ccmram"))) VoiceManager voiceManager;

int16_t buffer[Constants::CIRCULAR_BUFFER_SIZE] = {0};

PotBank hardwarePots(&hadc1, &hadc2, GPIOE, MUX_A_Pin, GPIOE, MUX_B_Pin);

extern MidiBuffer gMidiBuffer;

struct SynthParams {
    float volume;
    float cutoff;
    float resonance;
    float morph;
    float attack, decay, sustain, release;
} params;

// UI State Management
enum UI_View { VIEW_WAVETABLE, VIEW_FILTER, VIEW_ADSR };
UI_View currentView = VIEW_WAVETABLE;
uint32_t lastInteractionTime = 0;
const uint32_t UI_TIMEOUT_MS = 1200;
bool uiNeedsRefresh = false;
uint8_t lastChangedIndex = 255;

// CPU cycle debug
volatile uint32_t elapsed_us = 0;

extern "C" void cpp_main() {
    // CPU cycle debug
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    // init led controller
    if (ledController.init() != 0) {
        while(1);
    }

    ledController.ledAllOn(true);
    HAL_GPIO_TogglePin(GREEN_LED_GPIO_Port, GREEN_LED_Pin);
    HAL_Delay(100);
    
    // osc init
    voiceManager.process(buffer);
    voiceManager.process(&buffer[Constants::BUFFER_SIZE]);
    
    if (oled.init() == 0) {
        oled.fill(true); // Fill buffer with white
        oled.update();   // Send to screen via DMA
    } else {
        while(1);
    }
    // codec init
	auto status = codec.init(buffer, Constants::CIRCULAR_BUFFER_SIZE);
	if(status){
        while(1);
	}
    
    // init adc
    hardwarePots.init();
    
    // Init timer for adc scanning
    HAL_TIM_Base_Start_IT(&htim4);
    
    playStartupSequence();

	while(1){
        uint32_t currentTick = HAL_GetTick();

        static uint32_t lastHeartbeat = 0;
        if(currentTick - lastHeartbeat > 200) {
            HAL_GPIO_TogglePin(GREEN_LED_GPIO_Port, GREEN_LED_Pin);
            lastHeartbeat = HAL_GetTick();
        }
	
        if (hardwarePots.anyChanged()) {
            lastInteractionTime = currentTick;
            for (uint8_t i = 0; i < 8; i++) {
                if (hardwarePots.hasChanged(i)) {
                    handleParamChange(i);
                }
            }
        }

        if (currentView != VIEW_WAVETABLE && (currentTick - lastInteractionTime > UI_TIMEOUT_MS)) {
            currentView = VIEW_WAVETABLE;
            uiNeedsRefresh = true;
        }
        
        if (uiNeedsRefresh && !oled.isBusy()) {
            updateOledView();
            uiNeedsRefresh = false;
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

        // Button Debouncing and Edge Detection
        static uint32_t lastDebounceTime = 0;
        static bool lastRawA = true; // High by default (Pull-up)
        static bool lastRawB = true;

        if (HAL_GetTick() - lastDebounceTime > 25) { 
            lastDebounceTime = HAL_GetTick();

            // Button A Logic
            bool currentRawA = HAL_GPIO_ReadPin(BUTTON_A_GPIO_Port, BUTTON_A_Pin);
            if (currentRawA == GPIO_PIN_RESET && lastRawA == GPIO_PIN_SET) {
                cycleWaveform(0); // Trigger only on the "Falling Edge" (Press)
            }
            lastRawA = currentRawA;

            // Button B Logic
            bool currentRawB = HAL_GPIO_ReadPin(BUTTON_B_GPIO_Port, BUTTON_B_Pin);
            if (currentRawB == GPIO_PIN_RESET && lastRawB == GPIO_PIN_SET) {
                cycleWaveform(1); // Trigger only on the "Falling Edge" (Press)
            }
            lastRawB = currentRawB;
        }

        handleMidi();
    }
}

extern "C" void HAL_I2S_TxHalfCpltCallback(I2S_HandleTypeDef *hi2s) {
    std::fill(buffer, buffer + Constants::BUFFER_SIZE, 0);
    voiceManager.process(buffer);
}

extern "C" void HAL_I2S_TxCpltCallback(I2S_HandleTypeDef *hi2s) {
    uint32_t start_cycles = DWT->CYCCNT;

    std::fill(buffer + Constants::BUFFER_SIZE, buffer + (Constants::CIRCULAR_BUFFER_SIZE), 0);
    voiceManager.process(&buffer[Constants::BUFFER_SIZE]);

    // CPU cycle debug
    uint32_t end_cycles = DWT->CYCCNT;
    uint32_t elapsed_cycles = end_cycles - start_cycles;
    
    // Convert cycles to Microseconds
    // Formula: (Cycles / SystemCoreClock) * 1,000,000
    elapsed_us = (elapsed_cycles * 1000000) / SystemCoreClock;

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
    lastChangedIndex = index;
    Pot& p = hardwarePots.pots[index];
    
    // Update Shared State and Engine
    switch(index) {
        case 0:
            params.volume = p.scaleExp(0.f, 1.f);
            codec.setVolume(params.volume);
            break;
        case 1: 
            params.cutoff = p.scaleExp(20.f, 20000.f, 2.f);
            voiceManager.setCutoff(params.cutoff);
            currentView = VIEW_FILTER; 
            break;
        case 2: {
            float raw = p.getFloat();
            const float deadzone = 0.03f;
            if (raw <= deadzone) {
                params.resonance = 0.0f;
            } else {
                float normalized = (raw - deadzone) / (1.0f - deadzone);
                params.resonance = powf(normalized, 0.3f);
            }

            voiceManager.setResonance(params.resonance);
            currentView = VIEW_FILTER;
        }
            break;
        case 3: 
            params.morph = p.getFloat();
            voiceManager.setMorph(params.morph);
            currentView = VIEW_WAVETABLE;
            break;
        case 4: 
            params.attack = p.scaleExp(0.f, 2.f, 3.f);
            voiceManager.setAttack(params.attack);
            currentView = VIEW_ADSR;
            break;
        case 5: 
            params.decay = p.scaleExp(0.f, 2.f, 3.f);
            voiceManager.setDecay(params.decay); 
            currentView = VIEW_ADSR;
            break;
        case 6: 
            params.sustain = p.scaleLin(0.f, 1.f);
            voiceManager.setSustain(params.sustain);
            currentView = VIEW_ADSR;
            break;
        case 7: 
            params.release = p.scaleExp(0.f, 2.f, 3.f);
            voiceManager.setRelease(params.release);
            currentView = VIEW_ADSR;
            break;
    }

    uiNeedsRefresh = true;
}

void handleMidi() {
    MidiPacket packet;
    
    while (gMidiBuffer.pop(packet)) {
        uint8_t status   = packet.data[1];
        uint8_t data1    = packet.data[2];
        uint8_t data2    = packet.data[3];
        uint8_t message  = status & 0xF0;

        switch (message) {
            case 0x90: // Note On
                if (data2 > 0) {
                    voiceManager.noteOn(data1, data2);
                } else {
                    voiceManager.noteOff(data1);
                }
                break;

            case 0x80: // Note Off
                voiceManager.noteOff(data1);
                break;

            case 0xB0: // Control Change (CC)
                if (data1 == 1) { // Mod Wheel
                    voiceManager.setModWheel(data2);
                }
                // CC 123 (Panic)
                else if (data1 == 123) {
                    for(uint8_t i = 0; i < 127; i++) voiceManager.noteOff(i);
                }
                break;

            case 0xE0: // Pitch Bend
                voiceManager.setPitchBend(data1, data2);
                break;
        }
    }
}


void cycleWaveform(uint8_t slot) {
    // Determine indices for the dual-slot oscillator system
    uint8_t nextIdx = Osc::getActiveIdx(slot);
    uint8_t otherIdx = Osc::getActiveIdx(slot == 0 ? 1 : 0);

    // Increment and skip if it matches the other slot to avoid redundant pairs
    do {
        nextIdx = (nextIdx + 1) % WAVE_COUNT;
    } while (nextIdx == otherIdx);

    __disable_irq();
    Osc::loadWaveform(nextIdx, slot);
    __enable_irq();

    currentView = VIEW_WAVETABLE;
    lastInteractionTime = HAL_GetTick();
    uiNeedsRefresh = true; 
}

void updateOledView() {
    if (oled.isBusy()) return;

    char msg[22] = {0}; // Zero-initialize to prevent garbage text
    uint32_t now = HAL_GetTick();

    auto formatTime = [](char* buf, size_t len, const char* label, float sec) {
        if (sec < 1.0f) snprintf(buf, len, "%s: %.0fms", label, sec * 1000.0f);
        else snprintf(buf, len, "%s: %.2fs", label, sec);
    };

    switch(currentView) {
        case VIEW_WAVETABLE: {
            uint8_t leftIdx = Osc::getActiveIdx(0) & 0x07;
            uint8_t rightIdx = Osc::getActiveIdx(1) & 0x07;
            float oledWaveform[128];

            Osc::getMorphedPreview(oledWaveform, 128, params.morph);
            oled.drawBuffer(oledWaveform, 128);

            // Wavetable view draws its own header directly
            oled.drawString(0, 0, waveNames[leftIdx], true);
            oled.drawString(92, 0, waveNames[rightIdx], true);

            const int barStart = 44, barWidth = 40, barY = 3;
            for(int i = 0; i <= barWidth; ++i) oled.drawPixel(barStart + i, barY, true);
            
            int indicatorX = barStart + static_cast<int>(params.morph * barWidth);
            oled.drawVLine(indicatorX, 1, 5, true);
        } break;

        case VIEW_FILTER:
            FilterVisualizer::draw(oled, params.cutoff, params.resonance);
            if (lastChangedIndex == 2) snprintf(msg, sizeof(msg), "RESONANCE: %.2f", params.resonance);
            else if (params.cutoff < 1000.0f) snprintf(msg, sizeof(msg), "CUTOFF: %.0fHz", params.cutoff);
            else snprintf(msg, sizeof(msg), "CUTOFF: %.1fkHz", params.cutoff / 1000.0f);
            break;

        case VIEW_ADSR:
            AdsrVisualizer::draw(oled, params.attack, params.decay, params.sustain, params.release);
            if (lastChangedIndex == 4) formatTime(msg, sizeof(msg), "ATTACK", params.attack);
            else if (lastChangedIndex == 5) formatTime(msg, sizeof(msg), "DECAY", params.decay);
            else if (lastChangedIndex == 6) snprintf(msg, sizeof(msg), "SUSTAIN: %.2f", params.sustain);
            else if (lastChangedIndex == 7) formatTime(msg, sizeof(msg), "RELEASE", params.release);
            else snprintf(msg, sizeof(msg), "ENV GENERATOR");
            break;
    }

    // Volume Overlay (Priority)
    if (lastChangedIndex == 0 && (now - lastInteractionTime < UI_TIMEOUT_MS)) {
        for (int i = 0; i < 7; ++i) oled.drawLine(0, i, 127, i, false); // Clear header

        oled.drawString(0, 0, "VOL", true);
        const int barStart = 25, barEnd = 120, barWidth = barEnd - barStart;
        
        oled.drawLine(barStart, 3, barEnd, 3, true); 
        int volPos = barStart + static_cast<int>(params.volume * barWidth);
        oled.drawVLine(volPos, 1, 5, true);
        oled.drawVLine(volPos - 1, 2, 4, true); 
        oled.drawVLine(volPos + 1, 2, 4, true);
    } 
    // Standard Header (only if not viewing Wavetable or Volume)
    else if (currentView != VIEW_WAVETABLE && msg[0] != '\0') {
        oled.drawString(0, 0, msg, true);
    }

    oled.update();
}


void playStartupSequence() {
    const int centerX = 64;
    const int centerY = 32;
    const float maxRadius = 32.0f;
    const char* line1 = "STM32";
    const char* line2 = "SYNTH";
    
    int x1 = (128 - (5 * 6)) / 2;
    int x2 = (128 - (5 * 6)) / 2;

    // Square and Pentagon
    for (int sides = 4; sides <= 5; sides++) {
        for (float angleOffset = 0; angleOffset < (M_PI * 0.4f); angleOffset += 0.05f) {
            while(oled.isBusy());
            oled.fill(false);

            int lastX = -1, lastY = -1, firstX = -1, firstY = -1;
            for (int i = 0; i <= sides; i++) {
                float phi = angleOffset + (i * 2.0f * M_PI / sides);
                int x = centerX + static_cast<int>(cosf(phi) * maxRadius);
                int y = centerY + static_cast<int>(sinf(phi) * maxRadius);
                if (i == 0) { firstX = x; firstY = y; }
                else { oled.drawLine(lastX, lastY, x, y, true); }
                lastX = x; lastY = y;
            }
            oled.drawLine(lastX, lastY, firstX, firstY, true);
            oled.update();
            HAL_Delay(20);
        }
        
        HAL_GPIO_WritePin(GREEN_LED_GPIO_Port, GREEN_LED_Pin, GPIO_PIN_SET);
        HAL_Delay(20);
        HAL_GPIO_WritePin(GREEN_LED_GPIO_Port, GREEN_LED_Pin, GPIO_PIN_RESET);
    }

    // Hexagon with Extended Rotation and Logo
    for (float angleOffset = 0; angleOffset < (M_PI * 0.6f); angleOffset += 0.05f) {
        while(oled.isBusy());
        oled.fill(false);

        // Draw the Logo inside the spinning hexagon
        oled.drawString(x1, 24, line1, true);
        oled.drawString(x2, 34, line2, true);

        int lastX = -1, lastY = -1, firstX = -1, firstY = -1;
        for (int i = 0; i <= 6; i++) {
            float phi = angleOffset + (i * 2.0f * M_PI / 6);
            int x = centerX + static_cast<int>(cosf(phi) * maxRadius);
            int y = centerY + static_cast<int>(sinf(phi) * maxRadius);
            if (i == 0) { firstX = x; firstY = y; }
            else { oled.drawLine(lastX, lastY, x, y, true); }
            lastX = x; lastY = y;
        }
        oled.drawLine(lastX, lastY, firstX, firstY, true);
        
        oled.update();
        HAL_Delay(20);
    }

    // Clean transition to main UI
    ledController.ledAllOn(false);
    oled.fill(false);
    oled.update();
    while(oled.isBusy()); // Ensure the clear reaches the screen
}
