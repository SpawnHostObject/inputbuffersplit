//@ scoring/horizons.hpp
//@ Part of main.cpp (single translation unit). Not a standalone file: do not add it to the build.
    // Full wave look-ahead from the setting (mini = 15 less).
    int fullWaveHorizon(bool mini) {
        int wh = std::clamp(buf.waveHorizon, 20, 160);
        return st(mini ? std::max(20, wh - 15) : wh);  // v4.6: 65 normal / 50 mini
    }
