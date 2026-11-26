//@ autoplay/macro_timing.hpp
//@ Part of main.cpp (single translation unit). Not a standalone file: do not add it to the build.
    // ===== test 72: macros pressed like a bot =====
    // test 74: the game's step counter goes up by 2 a step (the macro's frames by 1: tests 70-73
    // played every macro at double speed). How much it moves a step is measured, and the macro's
    // step is the counter divided by that
    int progressPerStep = 0;
    int lastProgress = -1;
    int progressPerStepNow() {
        return progressPerStep > 0 ? progressPerStep : 2;
    }

    void measureProgress(PlayLayer* layer) {
        int p = static_cast<int>(layer->m_gameState.m_currentProgress);
        int d = p - lastProgress;
        if (lastProgress >= 0 && d >= 1 && d <= 8 && d != progressPerStep) {
            progressPerStep = d;
            say("macro: the game's step counter moves {} a step", d);
        }
        lastProgress = p;
    }

    int macroNow(PlayLayer* layer) {
        if (autoplay.macroXd) {
            return static_cast<int>(layer->m_gameState.m_levelTime * physicsTps()) + 1 + macroOffsetSetting;
        }
        int per = progressPerStep > 0 ? progressPerStep : 2;
        return static_cast<int>(layer->m_gameState.m_currentProgress) / per + macroOffsetSetting + autoplay.macroAutoOffset;
    }

    // test 73: a macro without position records only lines up from the level's start (test 71/72
    // played it by step from a start position too: the wrong inputs in the wrong places)
    bool macroUsable(PlayLayer* layer) {
        if (autoplay.macro.empty() || layer->m_isPracticeMode) {
            return false;
        }
        autoplay.macroByX = autoplay.macroForceX || (layer->m_startPosObject && !autoplay.macroX.empty());
        return !layer->m_startPosObject || autoplay.macroByX;
    }

    bool macroGapAt(int step) {
        for (auto const& [a, b] : autoplay.macroGaps) {
            if (step >= a && step < b) {
                return true;
            }
        }
        return false;
    }
