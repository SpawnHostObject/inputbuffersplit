//@ scoring/wave_search.hpp
//@ Part of main.cpp (single translation unit). Not a standalone file: do not add it to the build.
    int searchWave(PlayLayer* layer, PlayerObject* p1, PlayerObject* p2, bool dual, bool down, int horizon, int maxDelay, int base,
        int* landed) {
        marginUsed = false;
        using clock = std::chrono::steady_clock;
        auto start = clock::now();
        double budget = searchBudget();
        int target = std::min(horizon, base + st(12));
        dbgBestK = 0;
        dbgBestRet = -1;
        budgetCutK = 0;
        for (int k = searchFrom; k <= maxDelay; ++k) {
            SimEnd end;
            int ret = waveScore(layer, p1, p2, dual, k, down, horizon, target, &end);
            if (ret > dbgBestRet) {
                dbgBestRet = ret;
                dbgBestK = k;
            }
            if (end == SimEnd::StopSearch) {
                return 0;
            }
            if (ret >= target) {
                *landed = ret;
                // v2.10 safety margin: the first safe delay is the one that only just clears
                // the spike corner, so a pixel of difference in the real run can still clip
                // it. If the next two delays are safe too, take the middle one: a tick of
                // room on both sides.
                if (buf.waveMargin && k + 2 <= maxDelay) {
                    SimEnd e1, e2;
                    if (waveScore(layer, p1, p2, dual, k + 1, down, horizon, target, &e1) >= target &&
                        waveScore(layer, p1, p2, dual, k + 2, down, horizon, target, &e2) >= target) {
                        marginUsed = true;
                        return k + 1;
                    }
                }
                return k;
            }
            if (std::chrono::duration<double, std::milli>(clock::now() - start).count() > budget) {
                say("prediction budget hit at +{} ticks", k);
                budgetCutK = k;
                return 0;
            }
        }
        return 0;
    }
