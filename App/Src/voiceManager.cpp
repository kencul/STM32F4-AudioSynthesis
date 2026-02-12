#include "voiceManager.h"
#include "app.h"
#include "constants.h"

void VoiceManager::noteOn(uint8_t note, uint8_t velocity) {
    _tickCount++;
    int bestVoice = -1;
    uint32_t oldestTime = 0xFFFFFFFF;
    bool foundReleased = false;

    float velGain = static_cast<float>(velocity) / 127.0f;

    // Re-trigger check
    for(int i = 0; i < MAX_VOICES; i++) {
        if(_noteMap[i] == note) {
            _voices[i].noteOn(note, velGain);
            _lastUsed[i] = _tickCount;
            return;
        }
    }

    // Find best voice: Idle > Released > Active
    for(int i = 0; i < MAX_VOICES; i++) {
        // Absolute priority: Grab a silent voice
        if(!_voices[i].isActive()) {
            bestVoice = i;
            break; 
        }

        // Grab the oldest released voice
        bool isReleasing = (_noteMap[i] == 255); 
        
        if (isReleasing && !foundReleased) {
            foundReleased = true;
            oldestTime = _lastUsed[i];
            bestVoice = i;
        } 
        else if (isReleasing == foundReleased) {
            // If both are same status, take oldest
            if (_lastUsed[i] < oldestTime) {
                oldestTime = _lastUsed[i];
                bestVoice = i;
            }
        }
    }

    // Dispatch note
    if(bestVoice != -1) {
        _noteMap[bestVoice] = note;
        _lastUsed[bestVoice] = _tickCount;
        
        // Osc internally handles immediate start vs soft-kill
        _voices[bestVoice].noteOn(note, velGain);
    }
}

void VoiceManager::noteOff(uint8_t note) {
    for(int i = 0; i < MAX_VOICES; i++) {
        if(_noteMap[i] == note) {
            _voices[i].noteOff();
            _noteMap[i] = 255; 
        }
    }
}

void VoiceManager::process(int16_t* buffer) {
    // Tick global LFO once per block
    Osc::updateGlobalLFO();

    std::fill(mixBus, mixBus + Constants::BUFFER_SIZE, 0.0f);

    for(int i = 0; i < MAX_VOICES; ++i) {
        auto& v = _voices[i];
        if(v.isActive()) {
            v.process(mixBus);
            _voiceLevels[i] = v.getAdsrLevel();
        } else {
            _voiceLevels[i] = 0.0f;
        }
    }

    const float masterGain = (1.0f / static_cast<float>(MAX_VOICES)) * 32767.0f;

    for(int i = 0; i < Constants::BUFFER_SIZE; i++) {
        float out = mixBus[i] * masterGain;
        out = std::clamp(out, -32767.0f, 32767.0f);
        buffer[i] = static_cast<int16_t>(out);
    }
}

void VoiceManager::setPitchBend(uint8_t lsb, uint8_t msb) {
    int16_t bend = (static_cast<int16_t>(msb) << 7 | lsb) - 8192;
    Osc::setPitchBend(bend);
    for(auto& v : _voices) v.applyPitchBend();
}

void VoiceManager::setModWheel(uint8_t value) {
    float mod = static_cast<float>(value) / 127.0f;
    for(auto& v : _voices) v.setModWheel(mod);
}

void VoiceManager::setCutoff(float freq) {
    for(auto& v : _voices) v.setCutoff(freq);
}

void VoiceManager::setResonance(float res) {
    for(auto& v : _voices) v.setResonance(res);
}

void VoiceManager::setMorph(float morph) {
    for(auto& v : _voices) v.setMorph(morph);
}

void VoiceManager::setAttack(float seconds) {
    for(auto& v : _voices) v.setAttack(seconds);
}

void VoiceManager::setDecay(float seconds) {
    for(auto& v : _voices) v.setDecay(seconds);
}

void VoiceManager::setSustain(float level) {
    for(auto& v : _voices) v.setSustain(level);
}

void VoiceManager::setRelease(float seconds) {
    for(auto& v : _voices) v.setRelease(seconds);
}
