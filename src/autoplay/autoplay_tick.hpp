//@ autoplay/autoplay_tick.hpp
//@ Part of main.cpp (single translation unit). Not a standalone file: do not add it to the build.
    void autoplayTick(PlayLayer* layer, PlayerObject* p1) {
        if (!autoplay.active || autoplay.layer != layer || p1->m_isDead || !buf.ghost1 || buf.owner != layer) {
            return;
        }
        measureProgress(layer);  // test 74: once a physics step
        // v3.1.1: the run's position every 3 steps (for the global brain)
        {
            int step = static_cast<int>(layer->m_gameState.m_currentProgress) / std::max(1, progressPerStepNow());
            if (step % 3 == 0) {
                brainRecordPos(layer, step);
            }
        }
        // Take Over: your clicks go first; the bot plans again once you've stopped for a moment
        if (autoplay.takeOver && tickCount < autoplay.userUntil) {
            autoplay.plan.clear();
            autoplay.verifyAt = -1;
            return;
        }
        resetPending();
        int at = autoplay.attemptTick++;
        if (at % trailEvery() == 0 && static_cast<size_t>(at / trailEvery()) == autoplay.attemptTrail.size()) {
            autoplay.attemptTrail.push_back(p1->getPosition());
        }
        // what it learned in earlier attempts: replayed exactly, no searching
        // (not in an attempt autoplay was started in the middle of: the inputs are from the start)
        // v3.0 test 40: not while the route plans (it replayed the inputs that led into a dead end
        // right up to it, so the route had no room: Stereo Madness 60%, 3 deaths on the same tick).
        // Only a whole level learned to the end is replayed.
        // test 70: a macro: played by the game's step counter from the start of the attempt, the way
        // bots play it, up to where it keeps dying (then the route plans from there)
        // test 72: the macro is pressed at the start of the game's step (macroStep, from
        // GJBaseGameLayer::processCommands) the way bots play macros; here autoplay just doesn't plan
        // while the macro plays
        if (macroPlaying(layer, p1)) {
            macroGuard(layer, p1);  // v3.1.1
            return;
        }
        bool replayOk = autoplay.macro.empty();  // test 43: back on, up to 3 s before the last death (see autoplayDied); test 70: not with a macro
        if (replayOk && !layer->m_isPracticeMode && !autoplay.userTouched && at < autoplay.knownUntil) {
            while (autoplay.replayIdx < autoplay.known.size() && autoplay.known[autoplay.replayIdx].first <= at) {
                if (autoplay.known[autoplay.replayIdx].first == at) {
                    autoplaySet(layer, autoplay.known[autoplay.replayIdx].second);
                }
                ++autoplay.replayIdx;
            }
            return;
        }
        bool dual = layer->m_gameState.m_isDualMode && layer->m_player2;
        // v3.0 test 27: the long route (not in dual parts: the short planner does those)
        if (!dual && routeTick(layer, p1, at)) {
            return;
        }
        if (dual && route.active) {
            routeReset();
        }
        auto* p2 = dual ? layer->m_player2 : nullptr;
        bool holdMode = p1->m_isDart || p1->m_isShip || (p2 && (p2->m_isDart || p2->m_isShip));
        // v3.0 test 17: the plan reaches 1.5 s ahead (300 ticks flying, 360 on the ground) and is
        // extended at its far end every frame. A hard part is planned for while it's still far
        // away (dozens of frames of search instead of one or two), and the next few ticks are
        // already settled, so nothing is decided at the last moment.
        bool flyer = isFlying(p1) || (p2 && isFlying(p2));
        PlanCtx c { layer, p1, p2, dual, st(flyer ? 300 : 360), buf.appliedInput, holdMode };
        c.at = at;
        c.banned = layer->m_isPracticeMode ? nullptr : &autoplay.banned;

        // follow the plan: flips due now (the queued plans were built before it, so they go)
        while (!autoplay.plan.empty() && autoplay.plan.front() <= tickCount) {
            autoplay.plan.erase(autoplay.plan.begin());
            autoplayFlip(layer);
            clearQueue();
        }
        c.held0 = buf.appliedInput;
        if (tickCount < autoplay.verifyAt) {
            return;
        }
        unsigned frame = CCDirector::get()->getTotalFrames();
        if (frame != autoplay.frame) {
            autoplay.frame = frame;
            autoplay.frameMs = 0.0;
        }
        // v3.0 test 17: about 45% of a frame for planning (so the frame rate holds at any FPS),
        // at most 8 ms on PC and 5 ms on phones
        double frameMs = std::max(1.0, CCDirector::get()->getDeltaTime() * 1000.0);
#ifdef GEODE_IS_MOBILE
        double frameBudget = std::clamp(frameMs * 0.45, 2.0, 5.0);
#else
        double frameBudget = std::clamp(frameMs * 0.45, 2.0, 8.0);
#endif
        if (at < st(6)) {
            frameBudget *= 3.0;  // the first ticks of an attempt or a respawn: plan properly at once
        }
        double left = frameBudget - autoplay.frameMs;
        if (autoplay.generating) {
            left = 3.0;  // test 26: short looks while it's safe, long ones only near a crash (below)
        }
        if (left < 0.25) {
            return;  // this frame's time is used up: keep following the plan, look again next frame
        }
        autoplay.searchStart = std::chrono::steady_clock::now();
        autoplay.searchCap = left;

        float savedStep = buf.firstStep;
        buf.firstStep = 1.f;  // autoplay acts at the start of a tick: the ghost's first step is whole
        buf.fullHorizon = true;
        std::vector<int> rel;
        for (int t : autoplay.plan) {
            rel.push_back(t - tickCount);
        }
        bool seeded = false;
        if (rel.empty() && c.held0 && !c.holdMode) {
            rel.push_back(0);  // outside wave / ship, holding means jumping again and again: let go
            seeded = true;
        }
        autoplay.sims = 0;
        int survived = simulatePlan(c, rel);
        bool changed = seeded;
        if (survived < st(30)) {
            // a crash is close: this frame may take longer (a tiny hitch beats a death)
            autoplay.searchCap = std::max(autoplay.searchCap, autoplay.generating ? 40.0 : frameBudget * 2.0);
        }
        if (survived >= c.horizon) {
            autoplay.stuck = 0;
        }
        else {
            // v3.0 test 20: work through the queue of plans, carrying on from last frame
            std::vector<int> curAbs;
            for (int t : rel) {
                curAbs.push_back(tickCount + t);
            }
            if (autoplay.queue.empty()) {
                autoplay.seen.clear();
                autoplay.seen.insert(curAbs);
                simulatePlan(c, rel);  // for the orbs it touches
                auto orbs = buf.orbTouches;
                enqueueChildren(c, curAbs, survived, orbs);
                autoplay.queueBase = tickCount;
            }
            int startLen = survived;
            while (!autoplay.queue.empty() && !planOutOfTime() && survived < c.horizon) {
                std::pop_heap(autoplay.queue.begin(), autoplay.queue.end());
                auto item = std::move(autoplay.queue.back());
                autoplay.queue.pop_back();
                std::vector<int> r;
                bool stale = false;
                for (int t : item.plan) {
                    stale = stale || t < tickCount;
                    r.push_back(t - tickCount);
                }
                if (stale) {
                    continue;
                }
                int len = simulatePlan(c, r);
                auto orbs = buf.orbTouches;
                if (len > item.score && !item.plan.empty()) {
                    // the newest press: the first flip added after the parent (a tap adds two)
                    int pressIndex = static_cast<int>(item.plan.size()) - 1;
                    if (pressIndex > 0 && !c.holdMode) {
                        bool tapRelease = (c.held0 != (pressIndex % 2 == 1)) && item.plan[pressIndex] - item.plan[pressIndex - 1] <= st(10);
                        if (tapRelease) {
                            --pressIndex;
                        }
                    }
                    enqueueNeighbours(c, item.plan, len, pressIndex);
                }
                if (len > survived) {
                    // only a plan that really lasts longer replaces the current one (v3.0 test 16
                    // also switched to equally bad plans: the wave turned for no reason)
                    rel = r;
                    survived = len;
                    changed = true;
                }
                if (len < c.horizon) {
                    enqueueChildren(c, item.plan, len, orbs);
                }
            }
            bool timedOut = planOutOfTime();
            if (survived > startLen) {
                autoplay.stuck = 0;
            }
            else if (autoplay.queue.empty() && (!timedOut || ++autoplay.stuck >= 3)) {
                // nothing better from here: take back the last flip that isn't about to happen, mark
                // it as failed there, and look for another way from the shorter plan
                autoplay.stuck = 0;
                for (size_t i = rel.size(); i-- > 0;) {
                    if (rel[i] > st(12)) {
                        if (c.banned) {
                            autoplay.banned.insert(at + rel[i]);
                        }
                        if (autoplay.reports < 8) {
                            ++autoplay.reports;
                            say("autoplay backs up at tick {} ({}): drops the flip at tick {} (plan lasted {} of {})", at,
                                MODE_NAMES[modeIndex(p1, p2)], at + rel[i], survived, c.horizon);
                        }
                        rel.resize(i);
                        changed = true;
                        survived = simulatePlan(c, rel);
                        clearQueue();
                        break;
                    }
                }
            }
        }
        // v3.0 test 18: when the plan still crashes soon, what kills the ghost (logged at a death)
        if (survived < c.horizon && survived < st(90)) {
            buf.captureKiller = true;
            buf.killerSeen = false;
            buf.baseTouchedDown = false;
            buf.baseEscape = -1;
            simulatePlan(c, rel);
            buf.captureKiller = false;
            if (buf.killerSeen) {
                autoplay.killer = fmt::format("ghost hit object {} (type {}{}{}) at x {:.0f} y {:.0f} when at x {:.0f} y {:.0f}", buf.killerId,
                    buf.killerType, buf.killerNoTouch ? ", no-touch" : "", buf.killerDisabled ? ", disabled" : "", buf.killerRect.getMidX(),
                    buf.killerRect.getMidY(), buf.ghostDeathRect.getMidX(), buf.ghostDeathRect.getMidY());
            }
            else {
                autoplay.killer = "ghost didn't hit an object (fell out of the level, or stopped)";
            }
        }
        else {
            autoplay.killer = "ghost thought this was safe";
        }
        buf.fullHorizon = false;
        buf.firstStep = savedStep;
        sweepGhostLayer();
        autoplay.frameMs += std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - autoplay.searchStart).count();
        if (changed) {
            autoplay.plan.clear();
            for (int t : rel) {
                autoplay.plan.push_back(tickCount + t);
            }
            // flips due right now
            while (!autoplay.plan.empty() && autoplay.plan.front() <= tickCount) {
                autoplay.plan.erase(autoplay.plan.begin());
                autoplayFlip(layer);
            }
        }
        autoplay.planSafeUntil = at + survived;
        // a full plan is re-checked every few ticks; one that's still short keeps being worked on
        // (within the frame's time)
        autoplay.verifyAt = tickCount + (survived >= c.horizon ? st(8) : 1);
    }
