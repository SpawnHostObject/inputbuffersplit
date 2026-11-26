//@ autoplay/macro_play.hpp
//@ Part of main.cpp (single translation unit). Not a standalone file: do not add it to the build.
    bool macroPlaying(PlayLayer* layer, PlayerObject* p1) {
        if (!macroUsable(layer)) {
            return false;
        }
        if (autoplay.macroByX) {
            return autoplay.macroIdx < autoplay.macro.size();
        }
        int now = macroNow(layer);
        bool gap = macroGapAt(now);
        if (gap != autoplay.macroInGap) {
            autoplay.macroInGap = gap;
            if (gap) {
                // test 82 (like Nova): the route starts from you holding the macro's own inputs and checks
                // them with the ghost; only where they'd die does it look for others (test 75 planned the
                // stretch from nothing, with half a second to do it)
                routeStart(layer, p1);
                if (route.active) {
                    int base = tickCount - autoplay.macroStepNow;
                    route.flips.clear();
                    route.groups.clear();
                    for (auto const& [m, down] : autoplay.macro) {
                        int t = base + m;
                        if (m >= autoplay.macroStepNow && t > tickCount) {
                            route.flips.push_back(t);
                            route.groups.push_back({ t, t });
                        }
                    }
                }
                say("autoplay: plans the stretch the macro dies in (from step {}), starting from the macro's own inputs", now);
            }
            else {
                // back to the macro: its button as it has it here
                size_t idx = 0;
                while (idx < autoplay.macro.size() && autoplay.macro[idx].first < now) {
                    ++idx;
                }
                autoplay.macroIdx = idx;
                autoplaySet(layer, idx > 0 && autoplay.macro[idx - 1].second);
                say("autoplay: back to the macro at step {}", now);
            }
        }
        return !gap && now < autoplay.macroEnd;
    }

    // the game's own button path (handleButton), as macro bots press
    void macroPress(PlayLayer* layer, bool down) {
        if (buf.appliedInput == down) {
            return;
        }
        buf.injecting = true;
        buf.appliedInput = down;
        layer->handleButton(down, 1, true);
        buf.injecting = false;
        ++autoplay.inputs;
        autoplay.attemptFlips.push_back({ autoplay.attemptTick, down });
        brainRecord(layer, down);
    }

    void macroApplyFix(PlayLayer* layer, PlayerObject* p1, int now);
    bool macroRecordAt(float x, float& y, float& r, bool second = false);
    bool macroPlaceAt(PlayLayer* layer, PlayerObject* p1, float x);
    // test 84: player 2's button in a 2-player level
    void macroPress2(PlayLayer* layer, bool down) {
        if (autoplay.held2 == down) {
            return;
        }
        buf.injecting = true;
        autoplay.held2 = down;
        layer->handleButton(down, 1, false);
        buf.injecting = false;
        ++autoplay.inputs;
    }
