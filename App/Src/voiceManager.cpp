#include "voiceManager.h"
#include "app.h"

void VoiceManager::noteOn(uint8_t note) {
    _tickCount++;
    int bestVoice = -1;
    uint32_t oldestTime = 0xFFFFFFFF;
    bool foundReleased = false;

    // 1. Check if this exact note is already playing (re-trigger)
    for(int i = 0; i < MAX_VOICES; i++) {
        if(_noteMap[i] == note) {
            _voices[i].noteOn(note, 1.f);
            _lastUsed[i] = _tickCount;
            return;
        }
    }

    // 2. Search for the best candidate to take/steal
    for(int i = 0; i < MAX_VOICES; i++) {
        // A. Absolute priority: Voice is totally idle
        if(!_voices[i].isActive()) {
            bestVoice = i;
            break; 
        }

        // B. Secondary priority: Find the oldest voice among those being released
        // We prioritize stealing a "Released" note over an "Active" one.
        bool isReleasing = (_noteMap[i] == 255); 
        
        if (isReleasing && !foundReleased) {
            // First released voice we've found
            foundReleased = true;
            oldestTime = _lastUsed[i];
            bestVoice = i;
        } 
        else if (isReleasing == foundReleased) {
            // If both are released OR both are active, pick the oldest
            if (_lastUsed[i] < oldestTime) {
                oldestTime = _lastUsed[i];
                bestVoice = i;
            }
        }
    }

    // 3. Assign the note to the chosen voice
    if(bestVoice != -1) {
        _noteMap[bestVoice] = note;
        _lastUsed[bestVoice] = _tickCount;
        _voices[bestVoice].noteOn(note, 1.f);
    }
}

void VoiceManager::noteOff(uint8_t note) {
    for(int i = 0; i < MAX_VOICES; i++) {
        if(_noteMap[i] == note) {
            _voices[i].noteOff();
            _noteMap[i] = 255; // Mark as releasing
        }
    }
}

void VoiceManager::process(int16_t* buffer, uint16_t numFrames) {
    // This is now a true constant, so the compiler will be happy.
    // We use numFrames * 2 because we need Space for L and R for one callback
    static float mixBus[NUM_FRAMES * 2]; 
    
    // Safety: Ensure we aren't being asked for more frames than we allocated
    std::fill(mixBus, mixBus + (numFrames * 2), 0.0f);

    for(auto& v : _voices) {
        if(v.isActive()) {
            v.process(mixBus, numFrames); 
        }
    }

    // Master Scaling & Conversion
    float masterGain = 1.0f / (float)MAX_VOICES;

    for(int i = 0; i < numFrames * 2; i++) {
        float out = mixBus[i] * masterGain;

        // Soft-clipping/Limiting is better for audio than hard truncation
        if (out > 1.0f) out = 1.0f;
        if (out < -1.0f) out = -1.0f;

        buffer[i] = static_cast<int16_t>(lrintf(out * 32767.0f));
    }
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
