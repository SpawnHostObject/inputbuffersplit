//@ input/hold_stats.hpp
//@ Part of main.cpp (single translation unit). Not a standalone file: do not add it to the build.
    // Your recent input timings, so predictions model what you actually do instead of
    // an infinite hold: cube taps (holding auto-re-jumps) and robot holds (they set the
    // jump height).
    constexpr int HOLD_SAMPLES = 8;

    struct HoldStats {
        int samples[HOLD_SAMPLES] = {};
        int count = 0;
        int index = 0;

        void add(int ticks) {
            samples[index] = std::clamp(ticks, 1, 240);
            index = (index + 1) % HOLD_SAMPLES;
            count = std::min(count + 1, HOLD_SAMPLES);
        }

        // percentile of the recent samples, or `fallback` with no samples yet
        int pick(int fallback, int percent) const {
            if (count == 0) {
                return fallback;
            }
            int tmp[HOLD_SAMPLES];
            std::copy(samples, samples + count, tmp);
            int at = std::min(count - 1, count * percent / 100);
            std::nth_element(tmp, tmp + at, tmp + count);
            return tmp[at];
        }
    };

    HoldStats tapHold, robotHold;
    int tickCount = 0;

    enum class InputKind { Tap, Wave, Robot, Other };
    int pressTick = -1;
    InputKind pressKind = InputKind::Other;

    // Cube / ball / ufo / spider / swing are tapped. Ship, wave and robot are held, and
    // each keeps its own samples so long holds don't pollute the cube's tap length.
    InputKind kindOf(PlayerObject* p) {
        if (!p || p->m_isShip) {
            return InputKind::Other;
        }
        if (p->m_isDart) {
            return InputKind::Wave;
        }
        return p->m_isRobot ? InputKind::Robot : InputKind::Tap;
    }

    void recordRealInput(bool down, PlayerObject* p1) {
        if (down) {
            pressTick = tickCount;
            pressKind = kindOf(p1);
            return;
        }
        if (pressTick < 0) {
            return;
        }
        int held = tickCount - pressTick;
        pressTick = -1;
        if (buf.autoReleased) {
            return;  // the game already let go earlier; this length isn't a real hold
        }
        switch (pressKind) {
            case InputKind::Tap: tapHold.add(held); break;
            case InputKind::Robot: robotHold.add(held); break;
            default: break;
        }
    }

    // A hold that spans a death / restart / new level isn't a real tap length.
    void forgetPress() {
        pressTick = -1;
    }

    int holdEstimate() {
        return tapHold.pick(st(12), 50);  // ~50 ms, a typical tap
    }
