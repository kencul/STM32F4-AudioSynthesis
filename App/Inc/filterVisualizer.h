#pragma once
#include "oled.h"
#include <cmath>
#include <algorithm>

class FilterVisualizer {
public:
    static void draw(Oled& oled, float cutoffHz, float resonance) {
        oled.fill(false);

        const int width = 128;
        const int screenBottom = 63;
        const int textGap = 8;
        
        // Baseline for 0dB (Passband) shifted down to accommodate gap
        const int baselineY = 30; 
        const float peakScale = 25.0f; 
        
        const float minFreq = 20.0f;
        const float maxFreq = 20000.0f;
        const float logMin = logf(minFreq);
        const float logMax = logf(maxFreq);

        int lastY = -1;

        for (int x = 0; x < width; x++) {
            float normX = static_cast<float>(x) / static_cast<float>(width);
            float f = minFreq * expf(normX * (logMax - logMin));

            float r = f / cutoffHz;
            float r2 = r * r;
            float k = (1.0f - resonance);
            if (k < 0.01f) k = 0.01f;

            float magnitude = 1.0f / sqrtf(powf(1.0f - r2, 2.0f) + powf(r * k, 2.0f));

            int y;
            if (magnitude >= 1.0f) {
                y = baselineY - static_cast<int>(log2f(magnitude) * peakScale * 0.25f);
            } else {
                y = baselineY + static_cast<int>((1.0f - magnitude) * (screenBottom - baselineY) * 1.5f);
            }

            if (y < textGap) y = textGap; // Protect the text area

            if (y >= screenBottom) {
                if (lastY != -1 && lastY < screenBottom) {
                    oled.drawVLine(x, lastY, screenBottom, true);
                }
                break; 
            }

            if (lastY != -1) {
                oled.drawVLine(x, lastY, y, true);
            } else {
                oled.drawPixel(x, y, true);
            }
            lastY = y;
        }
    }
};
