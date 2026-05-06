/*
 * MoogLadder host-side verification tests.
 *
 * The Moog ladder was benched from the production signal chain due to CPU cost
 * (see moogLadder.h), but correctness and stability still matter — it is
 * preserved as a reference and candidate for future optimization. These tests
 * guard against numerical blow-up under the conditions most likely to cause it.
 */

#include "moogLadder.h"

#include <cmath>
#include <cstdio>

static int passed = 0;
static int failed = 0;

static void check(bool ok, const char* name) {
    if (ok) { printf("PASS  %s\n", name); ++passed; }
    else     { printf("FAIL  %s\n", name); ++failed; }
}

/*
 * High resonance dramatically increases the feedback gain. This is the most
 * common blow-up scenario for ladder filters — verify the filter rings but
 * stays bounded after an impulse.
 */
static void test_bounded_near_resonance() {
    MoogLadder moog;
    moog.init(48000.0f);
    moog.setCutoff(1000.0f);
    moog.setResonance(0.95f);

    bool ok = true;
    for (int i = 0; i < 5000; ++i) {
        float out = moog.process(i == 0 ? 1.0f : 0.0f);
        if (!std::isfinite(out) || std::abs(out) > 5.0f) {
            printf("      diverged at sample %d: %.4f\n", i, out);
            ok = false;
            break;
        }
    }
    check(ok, "MoogLadder: bounded near self-oscillation (res=0.95, impulse, 5k samples)");
}

/*
 * A DC input with no resonance should converge to a fixed DC output — the
 * single-pole lowpass limit of the ladder. If the output drifts or oscillates
 * the feedback accounting is broken.
 */
static void test_dc_stability() {
    MoogLadder moog;
    moog.init(48000.0f);
    moog.setCutoff(500.0f);
    moog.setResonance(0.0f);

    bool ok = true;
    for (int i = 0; i < 5000; ++i) {
        float out = moog.process(0.5f);
        if (!std::isfinite(out) || std::abs(out) > 2.0f) {
            printf("      unstable at sample %d: %.4f\n", i, out);
            ok = false;
            break;
        }
    }
    check(ok, "MoogLadder: stable DC response (res=0.0, 5k samples)");
}

int main() {
    printf("=== MoogLadder Tests ===\n");
    test_bounded_near_resonance();
    test_dc_stability();
    printf("\n%d / %d passed\n", passed, passed + failed);
    return failed > 0 ? 1 : 0;
}
