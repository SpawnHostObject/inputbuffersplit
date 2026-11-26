//@ autoplay/macro_guard.hpp
//@ Part of main.cpp (single translation unit). Not a standalone file: do not add it to the build.
    // ===== v3.1.1: the macro guard =====
    // Near a spot the macro died at before, the ghost plays the macro's next inputs from where you
    // really are (every 0.05 s, 1 s ahead). If it sees that death coming, the inputs before it are
    // tried 1-4 steps earlier or later, one at a time, until one way gets 0.5 s past it; that change
    // is kept in the macro (the rest isn't touched). Only near a known death, so a ghost that's
    // slightly wrong elsewhere never changes a good macro.
    struct MacroGuard {
        bool searching = false;
        int deathStep = 0;              // where the ghost sees it die (macro step)
        int need = 0;                   // ticks a fix has to live
        std::vector<std::pair<size_t, int>> tries;  // (macro index, shift)
        size_t next = 0;
        int lastCheck = -1000;
        int fixes = 0;
        int attemptLogged = -1;
    };
    MacroGuard mguard;
    // alpha.19: a death spot the guard found nothing for this attempt is left alone till the next one
    // (Heliopolis: the same search 20 times an attempt, 20 log lines, and frames lost)
    int mguardGaveUpStep = -1, mguardGaveUpAttempt = -1;

    std::vector<int> macroFlipsFrom(size_t from, int base, size_t changeIdx = SIZE_MAX, int shift = 0) {
        std::vector<int> flips;
        int last = INT_MIN;
        for (size_t i = from; i < autoplay.macro.size(); ++i) {
            int t = base + autoplay.macro[i].first + (i == changeIdx ? shift : 0);
            if (t <= last) {
                return {};  // the shift would cross the next input: not a valid try
            }
            flips.push_back(t);
            last = t;
        }
        return flips;
    }

    void macroGuard(PlayLayer* layer, PlayerObject* p1) {
        if (autoplay.macroXd || (autoplay.macroIsBrain && !autoplay.macroFixes.empty()) || autoplay.macroByX || autoplay.macroTwoPlayer || autoplay.macroDeaths.empty() || !autoplay.macroTuned ||
            autoplay.macroIdx >= autoplay.macro.size()) {
            mguard.searching = false;
            return;
        }
        int nowStep = autoplay.macroStepNow;
        int base = tickCount - nowStep;  // tick = base + macro step
        size_t from = autoplay.macroIdx;
        RoutePoint here { tickCount, nullptr, buf.appliedInput };
        if (!mguard.searching) {
            if (tickCount - mguard.lastCheck < st(12)) {
                return;
            }
            // only within 2 s before a death the macro had
            bool nearDeath = false;  // ("near" is a macro in Windows headers)
            for (int d : autoplay.macroDeaths) {
                if (d > nowStep && d - nowStep < st(480)) {
                    nearDeath = true;
                }
            }
            if (!nearDeath) {
                return;
            }
            mguard.lastCheck = tickCount;
            int length = st(240);
            auto flips = macroFlipsFrom(from, base);
            int lived = routeSim(layer, p1, here, flips, length, false);
            if (lived >= length) {
                return;
            }
            int dStep = nowStep + lived;
            bool known = false;
            for (int d : autoplay.macroDeaths) {
                if (std::abs(d - dStep) < st(30)) {
                    known = true;
                }
            }
            if (!known || lived < st(6)) {
                return;  // not the death it had (or too close to change anything)
            }
            if (mguardGaveUpAttempt == autoplay.attemptNo && std::abs(dStep - mguardGaveUpStep) < st(30)) {
                return;
            }
            mguard.searching = true;
            mguard.deathStep = dStep;
            mguard.need = std::min(length, lived + st(120));
            mguard.tries.clear();
            mguard.next = 0;
            // the inputs before the death, nearest first, each moved by 1..4 steps either way
            std::vector<size_t> idx;
            for (size_t i = from; i < autoplay.macro.size() && autoplay.macro[i].first <= dStep; ++i) {
                if (autoplay.macro[i].first > nowStep) {
                    idx.push_back(i);
                }
            }
            std::reverse(idx.begin(), idx.end());
            if (idx.size() > 6) {
                idx.resize(6);
            }
            for (int d : { 1, -1, 2, -2, 3, -3, 4, -4 }) {
                for (size_t i : idx) {
                    mguard.tries.push_back({ i, d });
                }
            }
            if (mguard.attemptLogged != autoplay.attemptNo) {
                mguard.attemptLogged = autoplay.attemptNo;
                say("macro guard: the ghost sees the macro die at step {} again: trying its inputs a little earlier or later", dStep);
            }
            return;
        }
        // searching: a few tries a tick
        for (int k = 0; k < 8 && mguard.next < mguard.tries.size(); ++k) {
            auto [i, d] = mguard.tries[mguard.next++];
            if (i < from || autoplay.macro[i].first + d <= nowStep) {
                continue;  // already played
            }
            auto flips = macroFlipsFrom(from, base, i, d);
            if (flips.empty()) {
                continue;
            }
            int lived = routeSim(layer, p1, here, flips, mguard.need, false);
            if (lived >= mguard.need) {
                int was = autoplay.macro[i].first;
                autoplay.macro[i].first += d;
                ++mguard.fixes;
                mguard.searching = false;
                say("macro guard: moved the {} at step {} by {:+d} step(s): the ghost gets past step {} that way", autoplay.macro[i].second ? "press" : "release",
                    was, d, mguard.deathStep);
                return;
            }
        }
        if (mguard.next >= mguard.tries.size()) {
            mguard.searching = false;
            mguardGaveUpStep = mguard.deathStep;
            mguardGaveUpAttempt = autoplay.attemptNo;
            say("macro guard: no small change gets past step {}: autoplay plans that part after the next death", mguard.deathStep);
        }
    }
