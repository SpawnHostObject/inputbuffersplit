//@ decision/deliver_pending.hpp
//@ Part of main.cpp (single translation unit). Not a standalone file: do not add it to the build.
    // v2.85 no-hitch: the decision for a click whose search ran out of time goes on, from where
    // you are now. No save found: the click goes to the game now.
    void resumeClick(PlayLayer* layer, QueuedInput const& q) {
        resumeNow = true;
        resumeEntry = q;
        bool held = intercept(layer, q.down, buf.isPlayer1);
        resumeNow = false;
        sweepGhostLayer();
        if (!held) {
            sendButton(layer, q.down);
        }
    }

    void deliverPending(PlayLayer* layer, PlayerObject* player) {
        if (buf.sinceInput <= 998) {
            ++buf.sinceInput;
        }
        // v2.85: nothing is sent to the game while you're dead (a press then carried over into
        // the next attempt)
        if (player->m_isDead) {
            return;
        }
        if ((buf.avoidMovers || buf.predictMovers) && !watched.empty()) {
            updateWatched();
        }

        bool twoPlayer = layer->m_level && layer->m_level->m_twoPlayerMode;
        bool dual = !twoPlayer && layer->m_gameState.m_isDualMode && layer->m_player2;
        if (!buf.shipBuffer && (player->m_isShip || (dual && layer->m_player2->m_isShip))) {
            resetPending();
            buf.autoReleased = false;
            if (buf.appliedInput != buf.realInput) {
                sendButton(layer, buf.realInput);
            }
            return;
        }
        // Left wave / robot while auto-released: the game goes back to what you're really holding.
        bool autoMode = player->m_isDart || player->m_isRobot || (dual && (layer->m_player2->m_isDart || layer->m_player2->m_isRobot));
        if (buf.autoReleased && !buf.autoDash && !autoMode) {
            buf.autoReleased = false;
        }

        // v2.80: an auto-click you never followed up on ends after a while
        if (buf.autoClicked && !buf.realInput && tickCount - buf.autoClickTick > st(30)) {
            buf.autoClicked = false;
        }
        if (buf.queue.empty()) {
            if (buf.appliedInput != buf.realInput && !buf.autoReleased && !buf.autoClicked) {
                sendButton(layer, buf.realInput);
            }
            tryAutoRelease(layer, player);
            tryAutoClick(layer, player);
            sweepGhostLayer();
            return;
        }

        // Check before decrementing: an input queued with N ticks fires right before
        // physics step N, which is exactly what simulate(delay = N) tested.
        while (!buf.queue.empty() && buf.queue.front().ticksLeft <= 0) {
            if (buf.queue.front().resume) {
                // v2.85 no-hitch: the search for this click goes on now
                auto q = buf.queue.front();
                buf.queue.pop_front();
                resumeClick(layer, q);
                continue;
            }
            bool down = buf.queue.front().down;
            buf.queue.pop_front();
            buf.releaseShift = down ? buf.shift : 0;
            if (down != buf.appliedInput) {
                sendButton(layer, down);
            }
        }
        for (auto& q : buf.queue) {
            --q.ticksLeft;
        }
        if (buf.queue.empty()) {
            buf.shift = 0;
        }
    }
