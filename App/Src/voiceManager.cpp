#include "voiceManager.h"
#include "app.h"

void VoiceManager::noteOn(uint8_t note, uint8_t velocity) {
    _tickCount++;
    int bestVoice = -1;
    uint32_t oldestTime = 0xFFFFFFFF;
    bool foundReleased = false;

    float velGain = static_cast<float>(velocity) / 127.0f;

    // Check if this exact note is already playing and retrigger
    for(int i = 0; i < MAX_VOICES; i++) {
        if(_noteMap[i] == note) {
            _voices[i].noteOn(note, velGain);
            _lastUsed[i] = _tickCount;
            return;
        }
    }

    // Search for the best candidate to take/steal
    for(int i = 0; i < MAX_VOICES; i++) {
        // Voice is totally idle
        if(!_voices[i].isActive()) {
            bestVoice = i;
            break; 
        }

        // Prioritize stealing a "Released" note over an "Active" one.
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

    // Assign the note to the chosen voice
    if(bestVoice != -1) {
        _noteMap[bestVoice] = note;
        _lastUsed[bestVoice] = _tickCount;
        _voices[bestVoice].noteOn(note, velGain);
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
    std::fill(mixBus, mixBus + (numFrames * 2), 0.0f);

    for(int i = 0; i < MAX_VOICES; ++i) {
        auto& v = _voices[i];
        
        if(v.isActive()) {
            v.process(mixBus, numFrames);
            _voiceLevels[i] = v.getAdsrLevel();
        } else {
            _voiceLevels[i] = 0.0f;
        }
    }

    // Pre-calculate scaling factor
    const float masterGain = (1.0f / static_cast<float>(MAX_VOICES)) * 32767.0f;

    for(int i = 0; i < numFrames * 2; i++) {
        float out = mixBus[i] * masterGain;
        out = std::clamp(out, -32767.0f, 32767.0f);

        buffer[i] = static_cast<int16_t>(out);
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
