//@ autoplay/inputs.hpp
//@ Part of main.cpp (single translation unit). Not a standalone file: do not add it to the build.
    void fetchOnlineMacro(int levelId);
    bool applyMacro(struct Macro const& m, std::string const& credit);
    void brainFetch(int levelId, std::string print, int forceTps);

    void autoplaySet(PlayLayer* layer, bool down) {
        if (buf.appliedInput == down) {
            return;
        }
        autoplayButton(layer, down);
        ++autoplay.inputs;
        autoplay.attemptFlips.push_back({ autoplay.attemptTick - 1, down });
        brainRecord(layer, down);
    }

    void autoplayFlip(PlayLayer* layer) {
        autoplaySet(layer, !buf.appliedInput);
    }

    // a new attempt: the learned inputs are replayed from the start again
    void autoplayAttemptStart(PlayLayer* layer) {
        // v3.0 test 51: another start position: what was learned counts ticks from the old one
        float sx = layer->m_startPosObject ? layer->m_startPosObject->getPositionX() : -1.f;
        if (autoplay.startX > -1.5f && std::abs(sx - autoplay.startX) > 1.f) {
            autoplay.known.clear();
            autoplay.knownTrail.clear();
            autoplay.knownUntil = 0;
            autoplay.knowsWholeLevel = false;
            autoplay.bestReached = -1;
            autoplay.bestFlips.clear();
            autoplay.bestTrail.clear();
            autoplay.deathTicks.clear();
            autoplay.banned.clear();
            say("autoplay: another start position, it learns from here");
        }
        autoplay.startX = sx;
        // normal mode: always start let go, so the learned inputs replay the same. Practice: a
        // checkpoint can put you mid-flight (ship), where letting go means crashing at once, so
        // autoplay plans from however you respawn (v3.0 test 13)
        if (buf.appliedInput && !layer->m_isPracticeMode) {
            autoplayButton(layer, false);
        }
        autoplay.planSafeUntil = -1;
        autoplay.attemptTick = 0;
        autoplay.attemptFlips.clear();
        autoplay.attemptTrail.clear();
        autoplay.plan.clear();
        routeReset();
        autoplay.verifyAt = -1;
        autoplay.replayIdx = 0;
        autoplay.macroIdx = 0;
        autoplay.macroArmed = !autoplay.macro.empty();
        ++autoplay.attemptNo;
        autoplay.runFlips.clear();  // test 90
        autoplay.runFixes.clear();
        macroOffsetSetting = static_cast<int>(Mod::get()->getSettingValue<int64_t>("macro-offset"));  // test 93
        macroFixesSetting = Mod::get()->getSettingValue<bool>("macro-fixes");
        autoplay.userTouched = false;
        autoplay.diedThisAttempt = false;
        autoplay.reports = 0;
        clearQueue();
    }
