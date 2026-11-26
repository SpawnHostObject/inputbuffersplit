//@ debug/live_path.hpp
//@ Part of main.cpp (single translation unit). Not a standalone file: do not add it to the build.
    // v2.65 live path (F10): between clicks the path keeps updating (v2.70: 20 times a second):
    // blue = where you go if you do nothing, yellow = where a click right now takes you.
    // After a click, that click's path (red / green) stays up briefly first.
    void livePath(PlayLayer* layer, PlayerObject* p1) {
        if (!buf.showPath || buf.perfMode || !buf.pathNode || !buf.ghost1 || buf.simulating || p1->m_isDead || layer->m_isPlatformer) {
            return;
        }
        if (tickCount < buf.pathHoldUntil || !buf.queue.empty() || tickCount % st(12) != 0) {
            return;
        }
        bool twoPlayer = layer->m_level && layer->m_level->m_twoPlayerMode;
        bool dual = !twoPlayer && layer->m_gameState.m_isDualMode && layer->m_player2;
        auto* p2 = dual ? layer->m_player2 : nullptr;
        bool wave = p1->m_isDart || (p2 && p2->m_isDart);
        bool ship = p1->m_isShip || (p2 && p2->m_isShip);
        bool flyer = isFlying(p1) || (p2 && isFlying(p2));
        bool mini = (p1->m_isDart && p1->m_vehicleSize < 1.f) || (p2 && p2->m_isDart && p2->m_vehicleSize < 1.f);
        int horizon = wave ? fullWaveHorizon(mini) : st(flyer ? 48 : std::clamp(buf.groundHorizon, 48, 240));
        bool down = !buf.appliedInput;
        int releaseAfter = down && !wave && !ship ? holdEstimate() : -1;

        buf.pathNode->clear();
        bool idleDied = false;
        buf.skipPress = true;
        auto idle = recordRun(layer, p1, p2, dual, 0, down, horizon, -1, &idleDied);
        buf.skipPress = false;
        drawPath(idle, 0, { 0.45f, 0.7f, 1.f, 0.6f }, idleDied);
        bool clickDied = false;
        auto click = recordRun(layer, p1, p2, dual, 0, down, horizon, releaseAfter, &clickDied);
        drawPath(click, 0, { 1.f, 0.85f, 0.3f, 0.85f }, clickDied);
    }
