#include "potBank.h"

PotBank::PotBank(ADC_HandleTypeDef* hadcX, ADC_HandleTypeDef* hadcY, GPIO_TypeDef* muxPortA, uint16_t pinA, GPIO_TypeDef* muxPortB, uint16_t pinB)
    : _hadcX(hadcX), _hadcY(hadcY), _muxPortA(muxPortA), _pinA(pinA), _muxPortB(muxPortB), _pinB(pinB), _currentStep(0) {}

void PotBank::init() {
    _changedFlags.reset();
    _busy = false;

    // initial scan of all 4 mux positions
    for (uint8_t step = 0; step < 4; step++) {
        _currentStep = step;
        applyMuxAddress();
        
        // Wait for mux and analog rails to settle
        for(volatile uint32_t i=0; i<500; i++) { __asm("nop"); }

        // single conversion on both ADCs
        HAL_ADC_Start(_hadcX);
        HAL_ADC_Start(_hadcY);

        // Polling
        if (HAL_ADC_PollForConversion(_hadcX, 10) == HAL_OK && 
            HAL_ADC_PollForConversion(_hadcY, 10) == HAL_OK) {
            
            pots[step].update(HAL_ADC_GetValue(_hadcX));
            pots[step + 4].update(HAL_ADC_GetValue(_hadcY));
            
            _changedFlags.set(step);
            _changedFlags.set(step + 4);
        }
        
        HAL_ADC_Stop(_hadcX);
        HAL_ADC_Stop(_hadcY);
    }

    // Reset step for the interrupt-based timer scan
    _currentStep = 0;
    applyMuxAddress();
}

void PotBank::handleInterrupt(ADC_HandleTypeDef* hadc) {
    // Synchronize on the second ADC finishing
    if (hadc->Instance == _hadcY->Instance) {
        
        // Update Pot in X-Bank (0-3)
        if (pots[_currentStep].update(HAL_ADC_GetValue(_hadcX))) {
            _changedFlags.set(_currentStep);
        }

        // Update Pot in Y-Bank (4-7)
        if (pots[_currentStep + 4].update(HAL_ADC_GetValue(_hadcY))) {
            _changedFlags.set(_currentStep + 4);
        }

        // Advance Multiplexer
        _currentStep = (_currentStep + 1) % 4;
        if (_currentStep != 0) {
            applyMuxAddress();
            triggerHardware();
        } else {
            _busy = false;
        }
    }
}

bool PotBank::hasChanged(uint8_t index) {
    if (index < 8 && _changedFlags.test(index)) {
        _changedFlags.reset(index);
        return true;
    }
    return false;
}

void PotBank::applyMuxAddress() {
    HAL_GPIO_WritePin(_muxPortA, _pinA, (_currentStep & 0x01) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(_muxPortB, _pinB, (_currentStep & 0x02) ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void PotBank::triggerHardware() {
    HAL_ADC_Start_IT(_hadcX);
    HAL_ADC_Start_IT(_hadcY);
}

void PotBank::startScan() {
    if (_busy) return;
    _busy = true;
    _currentStep = 0;
    applyMuxAddress();
    triggerHardware();
}
