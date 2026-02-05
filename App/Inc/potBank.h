#ifndef POTBANK_H
#define POTBANK_H

#include "main.h" // Required for HAL types
#include "Pot.h"
#include <bitset>

class PotBank {
public:
    PotBank(ADC_HandleTypeDef* hadcX, ADC_HandleTypeDef* hadcY, GPIO_TypeDef* muxPortA, uint16_t pinA, GPIO_TypeDef* muxPortB, uint16_t pinB);

    void init();
    
    // To be called in HAL_ADC_ConvCpltCallback
    void handleInterrupt(ADC_HandleTypeDef* hadc);

    // Check if a specific pot changed and clear its flag
    bool hasChanged(uint8_t index);
    
    // Check if any pot in the bank has changed
    bool anyChanged() const { return _changedFlags.any(); }

    Pot pots[8];

    // Interface for timer interrupt
    void startScan();
    bool isBusy() const { return _busy; }

private:
    ADC_HandleTypeDef *_hadcX, *_hadcY;
    GPIO_TypeDef* _muxPortA;
    uint16_t _pinA;
    GPIO_TypeDef* _muxPortB;
    uint16_t _pinB;
    
    uint8_t _currentStep;
    std::bitset<8> _changedFlags;
    
    bool _busy = false;

    void applyMuxAddress();
    void triggerHardware();
};

#endif
