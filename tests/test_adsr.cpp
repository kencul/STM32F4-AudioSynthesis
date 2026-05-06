/*
 * ADSR host-side verification tests.
 *
 * Three behavioral contracts:
 *   1. Attack reaches 1.0 within ±10% of the configured attack time.
 *   2. kill() ramps to silence within ~5 ms (≤300 samples at 48 kHz).
 *   3. Release converges to exactly 0.0 and transitions to IDLE — the
 *      -0.01f offset in the release formula is what makes this possible.
 */

#include "adsr.h"
#include "constants.h"

#include <cmath>
#include <cstdio>

static int passed = 0;
static int failed = 0;

static void check(bool ok, const char* name) {
    if (ok) { printf("PASS  %s\n", name); ++passed; }
    else     { printf("FAIL  %s\n", name); ++failed; }
}

/*
 * Attack step is (1.0 - output) / (attackTime * SR). Starting from 0, the
 * envelope should reach 1.0 in exactly attackTime seconds. We allow ±10%
 * to account for the one-sample overshoot clamp.
 */
static void test_attack_time_accuracy() {
    constexpr float ATTACK_SEC = 0.1f;
    constexpr int   SR         = Constants::SAMPLE_RATE;
    constexpr int   EXPECTED   = static_cast<int>(ATTACK_SEC * SR);
    constexpr int   WINDOW_LO  = static_cast<int>(EXPECTED * 0.90f);
    constexpr int   WINDOW_HI  = static_cast<int>(EXPECTED * 1.10f) + 1;

    Adsr adsr;
    adsr.init();
    adsr.setAttack(ATTACK_SEC);
    adsr.setDecay(1.0f);
    adsr.setSustain(0.5f);
    adsr.setRelease(1.0f);
    adsr.gate(true);

    int peakSample = -1;
    for (int i = 0; i < WINDOW_HI; ++i) {
        float out = adsr.getNextSample();
        if (out >= 1.0f && peakSample < 0) {
            peakSample = i;
            break;
        }
    }

    printf("      attack peak at sample %d  (window [%d, %d])\n",
           peakSample, WINDOW_LO, WINDOW_HI - 1);
    check(peakSample >= WINDOW_LO && peakSample < WINDOW_HI,
          "ADSR: attack reaches 1.0 within +-10%% of configured time");
}

/*
 * kill() promises ~5 ms silence for click-free voice stealing.
 * At 48 kHz that's 240 samples; we allow up to 300 to cover rounding.
 */
static void test_kill_completes_in_5ms() {
    constexpr int MAX_SAMPLES = 300; // generous over 240 (5 ms at 48 kHz)

    Adsr adsr;
    adsr.init();
    adsr.setAttack(0.001f);
    adsr.setDecay(0.1f);
    adsr.setSustain(0.7f);
    adsr.setRelease(0.5f);
    adsr.gate(true);

    // Advance into sustain so output is non-zero when kill() is called
    for (int i = 0; i < 2000; ++i) adsr.getNextSample();

    adsr.kill();

    int silenceSample = -1;
    for (int i = 0; i < MAX_SAMPLES; ++i) {
        float out = adsr.getNextSample();
        if (out == 0.0f && !adsr.isActive()) {
            silenceSample = i;
            break;
        }
    }

    printf("      kill reached IDLE at sample %d  (limit %d)\n",
           silenceSample, MAX_SAMPLES - 1);
    check(silenceSample >= 0,
          "ADSR: kill() ramps to silence and reaches IDLE within ~5 ms (300 samples)");
}

/*
 * The release formula is:  output = -0.01 + (output + 0.01) * releaseMult
 * Without the -0.01 offset, the exponential asymptotes above zero and IDLE
 * is never reached. This test confirms the envelope actually converges.
 */
static void test_release_converges_to_idle() {
    constexpr float RELEASE_SEC   = 0.3f;
    constexpr int   SR            = Constants::SAMPLE_RATE;
    // Budget: 3× the release time should be more than enough
    constexpr int   MAX_SAMPLES   = static_cast<int>(RELEASE_SEC * SR * 3);

    Adsr adsr;
    adsr.init();
    adsr.setAttack(0.001f);
    adsr.setDecay(0.05f);
    adsr.setSustain(0.7f);
    adsr.setRelease(RELEASE_SEC);
    adsr.gate(true);

    // Advance into sustain
    for (int i = 0; i < 5000; ++i) adsr.getNextSample();

    adsr.gate(false); // start release

    bool reachedIdle = false;
    for (int i = 0; i < MAX_SAMPLES; ++i) {
        adsr.getNextSample();
        if (!adsr.isActive()) {
            printf("      release reached IDLE at sample %d  (budget %d)\n",
                   i, MAX_SAMPLES);
            reachedIdle = true;
            break;
        }
    }
    check(reachedIdle,
          "ADSR: release converges to 0.0 and transitions to IDLE (offset trick)");
}

int main() {
    printf("=== ADSR Tests ===\n");
    test_attack_time_accuracy();
    test_kill_completes_in_5ms();
    test_release_converges_to_idle();
    printf("\n%d / %d passed\n", passed, passed + failed);
    return failed > 0 ? 1 : 0;
}
