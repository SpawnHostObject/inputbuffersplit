//@ core/timing.hpp
//@ Part of main.cpp (single translation unit). Not a standalone file: do not add it to the build.
    // v2.23 unlocked FPS: GD runs round(max(1, frameTime * 240)) physics steps per frame,
    // so only 60 / 120 / 240 FPS give 240 steps a second (144 FPS -> 288, 360 FPS -> 360).
    // Every window in this mod was written in 240-per-second ticks, so at other frame rates
    // it covered the wrong amount of time (at 360 FPS the cube's look-ahead ended before a
    // jump could land, so every click looked like a death). `st()` converts a 240-tick
    // amount into physics steps at the current rate. At 60 / 120 / 240 FPS it changes nothing.
    float stepScale() {
        float dt = buf.tickLength;
        if (!(dt > 0.001f && dt < 2.f)) {
            return 1.f;
        }
        return std::clamp(TICK / dt, 0.25f, 8.f);
    }

    // alpha.18: the TPS a macro was made at, used while autoplay plays it (0: the setting). A macro
    // played at its own TPS is played by time like on the bot that made it (Grief at 360: past 36%,
    // where played by position at 240 it died)
    int tpsOverride = 0;

    int st(int ticks240) {
        if (ticks240 <= 0) {
            return ticks240;
        }
        return std::max(1, static_cast<int>(std::lround(ticks240 * stepScale())));
    }
