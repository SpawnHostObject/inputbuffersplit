//@ scoring/delay_search.hpp
//@ Part of main.cpp (single translation unit). Not a standalone file: do not add it to the build.
    // Find the smallest delay that clearly beats pressing now. 0 = don't buffer.
    int searchDelay(PlayLayer* layer, PlayerObject* p1, PlayerObject* p2, bool dual, bool down, bool anyWave, int horizon, int maxDelay,
        int releaseAfter, int base, int* landed, bool needFull = false) {
        using clock = std::chrono::steady_clock;
        auto start = clock::now();
        double budget = searchBudget();
        auto overBudget = [&] {
            return std::chrono::duration<double, std::milli>(clock::now() - start).count() > budget;
        };

        int target = needFull ? horizon : std::min(horizon, base + st(12));
        dbgBestK = 0;
        dbgBestRet = -1;
        int sameStreak = 0;
        int waveBestK = 0;
        int waveBestRet = base;
        budgetCutK = 0;
        for (int k = searchFrom; k <= maxDelay; ++k) {
            SimEnd end;
            // Wave needs the full run to compare gains; elsewhere reaching `target` is enough.
            int ret = simulateOrbs(layer, p1, p2, dual, k, down, horizon, releaseAfter, anyWave ? horizon : target, &end);
            if (ret > dbgBestRet) {
                dbgBestRet = ret;
                dbgBestK = k;
            }
            if (end == SimEnd::StopSearch) {
                break;
            }
            if (anyWave) {
                // v4.6 wave rule: delaying a turn in a flat corridor naturally adds ~2 ticks
                // per tick of delay against the opposite wall, so only buffer when the delayed
                // run clearly clears a spike, saw or corner. A fully safe run needs
                // 2*k+6 extra ticks and ends the search; otherwise keep the best run that
                // gains at least 2*k+10 and beats the previous best by more than 2.
                if (ret >= horizon && ret - base >= k * 2 + 6) {
                    waveBestK = k;
                    waveBestRet = ret;
                    break;
                }
                if (ret >= base + k * 2 + 10 && ret > waveBestRet + 2) {
                    waveBestK = k;
                    waveBestRet = ret;
                    if (ret >= horizon) {
                        break;
                    }
                }
            }
            else if (ret < target && sameCrash(k, ret, base, sameStreak)) {
                break;
            }
            else if (ret >= target) {
                // v2.63: a delay that only lives 12 ticks longer can still crash later in the same
                // jump (Acheron's first jump: saved, then died on landing). Prefer the smallest
                // delay whose jump lands safely; if none does, keep this one.
                SimEnd full;
                if (ret < horizon && simulateOrbs(layer, p1, p2, dual, k, down, horizon, releaseAfter, horizon, &full) < horizon) {
                    for (int j = k + 1; j <= maxDelay && !overBudget(); ++j) {
                        SimEnd e;
                        int r = simulateOrbs(layer, p1, p2, dual, j, down, horizon, releaseAfter, horizon, &e);
                        if (e == SimEnd::StopSearch) {
                            break;
                        }
                        if (r >= horizon) {
                            *landed = r;
                            return j;
                        }
                    }
                }
                *landed = ret;
                return k;
            }
            if (overBudget()) {
                say("prediction budget hit at +{} ticks", k);
                budgetCutK = anyWave ? 0 : k;
                break;
            }
        }
        if (anyWave && waveBestK > 0) {
            *landed = waveBestRet;
            return waveBestK;
        }
        return 0;
    }
