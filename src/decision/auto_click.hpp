//@ decision/auto_click.hpp
//@ Part of main.cpp (single translation unit). Not a standalone file: do not add it to the build.
    // v2.80 auto-click (off by default; practice mode only unless 'Auto-release in normal
    // runs' is on, then the AUTO INPUTS ON tag shows): while you're not holding, check every
    // step whether doing nothing crashes within the auto window. If it does and a click right
    // now (with your usual hold) is fully safe, the mod clicks for you, lets go after that
    // hold, and swallows your own click if it still comes. Cube, ball, UFO, robot, spider and
    // swing; not wave / ship (constant control) or while dashing.
    void tryAutoClick(PlayLayer* layer, PlayerObject* p1) {
        if (!buf.autoClick || buf.autoReleased || buf.autoClicked || layer->m_isPlatformer) {
            return;
        }
        if (!layer->m_isPracticeMode && !buf.autoNormalRuns) {
            return;
        }
        if (!buf.queue.empty() || buf.realInput || buf.appliedInput || !buf.isPlayer1 || !buf.ghost1) {
            return;
        }
        if (layer->m_level && layer->m_level->m_twoPlayerMode) {
            return;
        }
        bool dual = layer->m_gameState.m_isDualMode && layer->m_player2;
        auto* p2 = dual ? layer->m_player2 : nullptr;
        auto skipMode = [](PlayerObject* p) { return p && (p->m_isDart || p->m_isShip || p->m_isDashing); };
        if (p1->m_isDead || skipMode(p1) || skipMode(p2)) {
            return;
        }

        // 1. cheap: does doing nothing crash soon?
        int window = st(std::clamp(buf.autoWindow, 2, 24));
        buf.sawMover = false;
        buf.skipPress = true;
        buf.orbClick = false;  // no clicks at all in the do-nothing run
        SimEnd end;
        int idle = simulate(layer, p1, p2, dual, 0, true, window, -1, window, &end);
        buf.skipPress = false;
        buf.orbClick = buf.assumeOrbs;
        if (idle >= window || buf.ghostHitPortal) {
            return;
        }

        // 2. a click now, held as long as you usually hold, must be fully safe
        bool flyer = isFlying(p1) || (p2 && isFlying(p2));
        int horizon = st(flyer ? 48 : std::clamp(buf.groundHorizon, 48, 240));
        int hold = std::max(1, holdEstimate());
        buf.trackMovers = buf.avoidMovers;
        int press = simulateOrbs(layer, p1, p2, dual, 0, true, horizon, hold, horizon, &end);
        buf.trackMovers = false;
        if (press < horizon || (buf.avoidMovers && buf.sawMover)) {
            return;
        }
        buf.autoClicked = true;
        buf.autoClickTick = tickCount;
        sendButton(layer, true);
        queueInput(hold, false);
        say("AUTO click: doing nothing dies in {}t, clicking now is safe (hold {}t)", idle, hold);
    }
