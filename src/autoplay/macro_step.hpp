//@ autoplay/macro_step.hpp
//@ Part of main.cpp (single translation unit). Not a standalone file: do not add it to the build.
    // at the start of each game step, before the players move
    // alpha.26: after GJBaseGameLayer::processCommands, once a frame, as xdBot does (src/main.cpp,
    // handlePlaying): every input up to this frame, then every position record up to it (x, y, angle)
    void macroStepXd(PlayLayer* layer) {
        if (!autoplay.active || autoplay.layer != layer || autoplay.macro.empty() || layer->m_isPracticeMode || buf.simulating ||
            !macroUsable(layer) || layer->m_levelEndAnimationStarted) {
            return;
        }
        int frame = macroNow(layer);
        if (frame == autoplay.xdLastFrame && frame != 0) {
            return;
        }
        autoplay.xdLastFrame = frame;
        autoplay.macroStepNow = frame;
        auto* p1 = layer->m_player1;
        if (!p1) {
            return;
        }
        if (autoplay.macroSyncedFor != autoplay.attemptNo) {
            autoplay.macroSyncedFor = autoplay.attemptNo;
            autoplay.macroMidStart = !layer->m_startPosObject && p1->getPositionX() > 60.f;
            size_t idx = 0;
            while (idx < autoplay.macro.size() && autoplay.macro[idx].first < frame) {
                ++idx;
            }
            autoplay.macroIdx = idx;
            autoplay.macroIdx2 = 0;
            while (autoplay.macroIdx2 < autoplay.macroP2.size() && autoplay.macroP2[autoplay.macroIdx2].first < frame) {
                ++autoplay.macroIdx2;
            }
            autoplay.xdFixIdx = 0;
            while (autoplay.xdFixIdx < autoplay.macroFixes.size() && autoplay.macroFixes[autoplay.xdFixIdx].frame < frame) {
                ++autoplay.xdFixIdx;
            }
            autoplay.macroLogged = false;
            say("autoplay: playing the macro {} like xdBot from frame {}", autoplay.fromMacro, frame);
        }
        if (p1->m_isDead) {
            return;
        }
        while (autoplay.macroIdx < autoplay.macro.size() && frame >= autoplay.macro[autoplay.macroIdx].first) {
            macroPress(layer, autoplay.macro[autoplay.macroIdx].second);
            ++autoplay.macroIdx;
        }
        if (autoplay.macroTwoPlayer) {
            while (autoplay.macroIdx2 < autoplay.macroP2.size() && frame >= autoplay.macroP2[autoplay.macroIdx2].first) {
                macroPress2(layer, autoplay.macroP2[autoplay.macroIdx2].second);
                ++autoplay.macroIdx2;
            }
        }
        if (!macroFixesSetting) {
            return;
        }
        auto& fx = autoplay.macroFixes;
        auto* p2 = layer->m_player2;
        while (autoplay.xdFixIdx < fx.size() && frame >= fx[autoplay.xdFixIdx].frame) {
            auto const& f = fx[autoplay.xdFixIdx];
            if (f.x != 0.f && f.y != 0.f) {
                p1->setPosition({ f.x, f.y });
                ++autoplay.macroFixCount;
            }
            if (f.r != 0.f) {
                p1->setRotation(f.r);
            }
            if (layer->m_gameState.m_isDualMode && p2 && f.p2) {
                if (f.x2 != 0.f && f.y2 != 0.f) {
                    p2->setPosition({ f.x2, f.y2 });
                }
                if (f.r2 != 0.f) {
                    p2->setRotation(f.r2);
                }
            }
            ++autoplay.xdFixIdx;
        }
    }

    void macroStep(PlayLayer* layer) {
        if (!autoplay.active || autoplay.layer != layer || autoplay.macro.empty() || layer->m_isPracticeMode || buf.simulating) {
            return;
        }
        if (autoplay.macroXd) {
            return;  // alpha.26: after the game's commands (macroStepXd)
        }
        if (progressPerStep <= 0) {
            return;  // one more step to know how the counter moves
        }
        autoplay.macroStepNow = macroNow(layer);
        if (!macroUsable(layer)) {
            if (autoplay.macroSyncedFor != autoplay.attemptNo) {
                autoplay.macroSyncedFor = autoplay.attemptNo;
                say("autoplay: the macro {} has no position records, so it only plays from the level's start: autoplay plays this start position itself",
                    autoplay.fromMacro);
            }
            return;
        }
        auto* p1 = layer->m_player1;
        if (!p1 || p1->m_isDead || (autoplay.takeOver && tickCount < autoplay.userUntil)) {
            return;
        }
        int now = macroNow(layer);
        float px = p1->getPositionX();
        // the macro picks up where the attempt is: by step from the level's start, by x from a start pos
        if (autoplay.macroSyncedFor != autoplay.attemptNo) {
            autoplay.macroSyncedFor = autoplay.attemptNo;
            autoplay.macroMidStart = !layer->m_startPosObject && px > 60.f;
            autoplay.macroLastPx = -1.f;
            autoplay.fixPtr = 0;
            autoplay.macroNudged.clear();
            size_t idx = 0;
            while (idx < autoplay.macro.size() && (autoplay.macroByX ? autoplay.macroX[idx] <= px : autoplay.macro[idx].first < now)) {
                ++idx;
            }
            autoplay.macroIdx = idx;
            autoplay.macroLogged = false;
            macroPress(layer, idx > 0 && autoplay.macro[idx - 1].second);
            if (autoplay.macroTwoPlayer) {
                size_t i2 = 0;
                bool byX2 = autoplay.macroByX && autoplay.macroX2.size() == autoplay.macroP2.size();
                while (i2 < autoplay.macroP2.size() && (byX2 ? autoplay.macroX2[i2] <= px : autoplay.macroP2[i2].first < now)) {
                    ++i2;
                }
                autoplay.macroIdx2 = i2;
                autoplay.held2 = !(i2 > 0 && autoplay.macroP2[i2 - 1].second);  // so the next line presses it either way
                macroPress2(layer, i2 > 0 && autoplay.macroP2[i2 - 1].second);
            }
            say("autoplay: playing the macro {} (it comes first) from {}", autoplay.fromMacro,
                autoplay.macroByX ? fmt::format("x {:.0f}", px) : fmt::format("step {}", now));
        }
        if (!autoplay.macroByX && (now >= autoplay.macroEnd || macroGapAt(now))) {
            return;
        }
        if (!autoplay.macroByX && autoplay.macroInGap) {
            return;  // leaving a gap is handled once a step in macroPlaying
        }
        // alpha.10: played by position an input goes in on the step nearest its x (not the first step
        // past it), and you're put exactly where the bot was when it pressed: Grief's mini wave let go
        // up to a step late and peaked 2.6 units higher than the bot, into a spike (1.7%)
        float dx = autoplay.macroLastPx >= 0.f ? px - autoplay.macroLastPx : 0.f;
        autoplay.macroLastPx = px;
        if (dx < 0.f || dx > 8.f) {
            dx = 0.f;
        }
        float ahead = autoplay.macroForceX ? dx * 0.5f : 0.f;
        while (autoplay.macroIdx < autoplay.macro.size() &&
               (autoplay.macroByX ? autoplay.macroX[autoplay.macroIdx] <= px + ahead : autoplay.macro[autoplay.macroIdx].first <= now)) {
            if (!autoplay.macroLogged) {
                autoplay.macroLogged = true;
                say("macro: an input for step {} went in at step {} (x {:.1f})", autoplay.macro[autoplay.macroIdx].first, now, px);
            }
            if (autoplay.macroForceX && macroFixesSetting && dx > 0.f) {
                float mx = autoplay.macroX[autoplay.macroIdx];
                if (std::abs(mx - px) <= dx && macroPlaceAt(layer, p1, mx)) {
                    px = mx;
                }
            }
            macroPress(layer, autoplay.macro[autoplay.macroIdx].second);
            ++autoplay.macroIdx;
        }
        if (autoplay.macroTwoPlayer) {
            bool byX = autoplay.macroByX && autoplay.macroX2.size() == autoplay.macroP2.size();
            while (autoplay.macroIdx2 < autoplay.macroP2.size() &&
                   (byX ? autoplay.macroX2[autoplay.macroIdx2] <= px + ahead : autoplay.macroP2[autoplay.macroIdx2].first <= now)) {
                // alpha.11: as with player 1's list: you're put on the bot's spot as it presses (Grief's
                // clicks are all in this list)
                if (byX && autoplay.macroForceX && macroFixesSetting && dx > 0.f) {
                    float mx = autoplay.macroX2[autoplay.macroIdx2];
                    if (std::abs(mx - px) <= dx && macroPlaceAt(layer, p1, mx)) {
                        px = mx;
                    }
                }
                macroPress2(layer, autoplay.macroP2[autoplay.macroIdx2].second);
                ++autoplay.macroIdx2;
            }
        }
        // alpha.13: the nudge at a spot it keeps dying at; alpha.15: after the presses and just before
        // the spot (10 units): alpha.14 nudged 25 before, and the press after it snapped you back
        if (autoplay.macroForceX && dx > 0.f && !autoplay.macroNudges.empty()) {
            for (auto const& [k, n] : autoplay.macroNudges) {
                if (n > 0 && n <= 3 && px >= k - 10.f && px < k - 2.f && !autoplay.macroNudged.contains(k)) {
                    autoplay.macroNudged.insert(k);
                    float nx = px + dx * MACRO_NUDGE[n];
                    if (macroPlaceAt(layer, p1, nx)) {
                        say("macro: moved {:.2f} units along the bot's path at x {:.1f} (before the spot at x {})", nx - px, px, k);
                        px = nx;
                    }
                    else {
                        say("macro: couldn't move you along the bot's path at x {:.1f} (no record there)", px);
                    }
                }
            }
        }
        macroApplyFix(layer, p1, now);
    }
