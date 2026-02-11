#pragma once
#include "oled.h"
#include <cmath>
#include <algorithm>

class AdsrVisualizer {
public:
    static void draw(Oled& oled, float a, float d, float s, float r) {        
        oled.fill(false);
        
        const int bottomY = 63;
        const int topY = 8;
        const int height = bottomY - topY;
        const int xS = 16;
        const float timeToPixelScale = (112.0f / 3.0f) / 2.0f;
        
        int xA = static_cast<int>(a * timeToPixelScale);
        int xD = static_cast<int>(d * timeToPixelScale);
        int xR = static_cast<int>(r * timeToPixelScale);
        
        int sustainY = bottomY - static_cast<int>(s * height);
        int curX = 0;

        // Attack
        oled.drawLine(curX, bottomY, curX + xA, topY, true);
        curX += xA;
        
        // Decay
        if (xD > 0) {
            float decayLevel = 1.0f;
            // Calculate a multiplier that reaches the target in exactly xD steps
            float vDecayMult = powf(0.01f, 1.0f / static_cast<float>(xD));
            
            for(int x = 0; x < xD; x++) {
                float nextLevel = s + (decayLevel - s) * vDecayMult;
                oled.drawLine(curX + x, bottomY - static_cast<int>(decayLevel * height), 
                              curX + x + 1, bottomY - static_cast<int>(nextLevel * height), true);
                decayLevel = nextLevel;
            }
        } else {
            // Instant decay to sustain
            oled.drawLine(curX, topY, curX, sustainY, true);
        }
        curX += xD;
        
        // Sustain
        oled.drawLine(curX, sustainY, curX + xS, sustainY, true);
        curX += xS;
        
        // Release
        if (xR > 0) {
            float releaseLevel = s;
            float vReleaseMult = powf(0.01f, 1.0f / static_cast<float>(xR));
            
            for(int x = 0; x < xR; x++) {
                float nextLevel = releaseLevel * vReleaseMult;
                oled.drawLine(curX + x, bottomY - static_cast<int>(releaseLevel * height), 
                              curX + x + 1, bottomY - static_cast<int>(nextLevel * height), true);
                releaseLevel = nextLevel;
            }
        } else {
            // Instant release to bottom
            oled.drawLine(curX, sustainY, curX, bottomY, true);
        }
    }
};
