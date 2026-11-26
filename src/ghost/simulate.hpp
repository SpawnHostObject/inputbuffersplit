//@ ghost/simulate.hpp
//@ Part of main.cpp (single translation unit). Not a standalone file: do not add it to the build.
    enum class SimEnd { Ok, StopSearch };

    // Returns ticks survived after the input (horizon = fully safe).
    // releaseAfter: ticks after the input at which the ghost does the opposite (-1 = never).
    // stopAt: stop as soon as this many ticks were survived (the caller already knows
    // that's good enough), instead of always running the full horizon.
    int simulate(PlayLayer* layer, PlayerObject* p1, PlayerObject* p2, bool dual, int delay, bool down, int horizon, int releaseAfter,
        int stopAt, SimEnd* end) {
        *end = SimEnd::Ok;
        if (!buf.ghost1) {
            return horizon;
        }
        buf.simulating = true;
        markLevelNodes(layer);
        buf.ghostDied = false;
        buf.ghostHitPortal = false;
        buf.ghostSwitched = false;
        buf.ghostSolo = false;
        buf.firstOrbTick = -1;
        buf.lastOrbTick = -1;
        buf.laterOrbTouched = false;
        buf.orbUsedTick = -1;

        bool second = dual && buf.ghost2 && p2;
        buf.simSecond = second;
        buf.haveRealRect = buf.ignoreTouching;
        buf.harmless.clear();
        // v2.78.1: always known, so mode portals you're still inside are skipped (see
        // collisionCheckObjects)
        buf.haveStartRects = true;
        PlayerObject* from1 = buf.simFrom ? buf.simFrom : p1;
        buf.realRect1 = from1->getObjectRect();
        buf.realRect2 = second ? p2->getObjectRect() : buf.realRect1;
        copyState(buf.ghost1, from1);
        if (second) {
            copyState(buf.ghost2, p2);
        }
        buf.held1 = buf.held2 = buf.simFrom ? buf.simFromHeld : buf.appliedInput;
        int off = buf.simFrom ? buf.simOffset : 0;
        buf.gBounds = false;
        if (buf.offscreenFill) {
            if (buf.simFrom) {
                buf.gBounds = buf.simFromBounds;
                buf.gBoundLo = buf.simFromLo;
                buf.gBoundHi = buf.simFromHi;
            }
            else {
                realBounds(layer, p1, buf.gBounds, buf.gBoundLo, buf.gBoundHi);
            }
        }
        buf.auto1 = buf.auto2 = false;
        buf.autoAt1 = buf.autoAt2 = -1;

        // v4.6 wave: before a delayed input fires, the ghost is in the opposite state
        // (holding before a delayed release, released before a delayed press).
        bool waveRun = p1->m_isDart || (second && p2->m_isDart);
        if (buf.waveV46 && waveRun && delay > 0 && !buf.skipPress) {
            pressGhost(buf.ghost1, !down);
            if (second) {
                pressGhost(buf.ghost2, !down);
            }
        }

        float dt = buf.tickLength;
        if (!(dt > 0.001f && dt < 2.0f)) {
            dt = TICK;
        }

        // (v1 forced a fake !down button event here for delay > 0, which made delayed
        //  runs start from a different jump-buffer state than the "press now" run.
        //  The ghost now simply mirrors the real player's button state, except in wave,
        //  which keeps the v4.6 behaviour above.)

        bool flying = isFlying(p1) || (dual && p2 && isFlying(p2));
        bool spiderRun = buf.spiderSmart && (p1->m_isSpider || (second && p2->m_isSpider));
        // v2.40: a UFO that lands on something has finished its flap, like a cube jump
        bool ufoRun = buf.ufoSmart && (p1->m_isBird || (second && p2->m_isBird));
        // v2.45 ball: a flip in a tight corridor can land after only a few ticks, and a ball
        // often lands right next to a spike on the new surface, so short flips count as
        // landings and the landing has to last longer before the flip is called safe.
        bool ballRun = buf.ballSmart && (p1->m_isBall || (second && p2->m_isBall));
        int total = delay + horizon;
        int survived = 0;
        int airTicks = 0;
        int groundTicks = 0;
        int tick = 0;
        bool died = false;
        bool safe = false;

        collectMovers();
        collectScripted(layer, off + total);
        if (off > 0 && buf.crossPos) {
            // test 27: starting further along the route: triggers between you and the start went
            // off when the route passed them
            double startX = buf.ghost1->getPositionX();
            auto const& rp = *buf.crossPos;
            for (auto& m : scriptedMoves) {
                if (m.armed || m.x > startX + 60.0) {
                    continue;
                }
                int at = -1;
                for (size_t k = 0; k < rp.size() && static_cast<int>(k) + buf.crossBase < off; ++k) {
                    bool reached = m.touch ? CCRect(m.rect.origin.x - 15.f, m.rect.origin.y - 15.f, m.rect.size.width + 30.f,
                                                 m.rect.size.height + 30.f)
                                                 .containsPoint(rp[k])
                                           : rp[k].x >= m.x;
                    if (reached) {
                        at = buf.crossBase + static_cast<int>(k);
                        break;
                    }
                }
                if (at < 0) {
                    continue;  // the route didn't get to it before this test starts: the test sets it off itself
                }
                m.armed = true;
                m.fireStep = std::max(0, at) + m.delay;
            }
        }
        for (; tick < total; ++tick) {
            second = buf.simSecond;  // v2.84: after a solo portal only P1 goes on (v2.85: after a dual portal both)
            if (!movers.empty()) {
                placeMovers(off + tick + 1);  // where they'll be after this step
            }
            if (!scriptedMoves.empty()) {
                // v2.81: a move / rotate / toggle trigger the ghost has reached goes off
                fireScripted(buf.ghost1->getPositionX(), buf.ghost1->getObjectRect(), off + tick);
                placeScripted(off + tick + 1);
            }
            buf.simTick = tick;
            // v2.26: orb auto-clicks that wait a few ticks into the orb fire here, if the
            // ghost is still inside an orb by then.
            if (buf.autoAt1 >= 0 && tick >= buf.autoAt1) {
                if (!buf.held1 && buf.ghost1->m_touchingRings->count() > 0) {
                    buf.ghost1->pushButton(PlayerButton::Jump);
                    buf.auto1 = true;
                }
                buf.autoAt1 = -2;
            }
            if (second && buf.autoAt2 >= 0 && tick >= buf.autoAt2) {
                if (!buf.held2 && buf.ghost2->m_touchingRings->count() > 0) {
                    buf.ghost2->pushButton(PlayerButton::Jump);
                    buf.auto2 = true;
                }
                buf.autoAt2 = -2;
            }
            if (tick == delay && !buf.skipPress) {
                pressGhost(buf.ghost1, down);
                if (second) {
                    pressGhost(buf.ghost2, down);
                }
            }
            if (buf.retapAt >= 0) {
                // v2.15: a second tap (let go, press, let go): "flip back" for the spider
                int t = tick - delay - buf.retapAt;
                if (t == 0 || t == 1 || t == 5) {
                    pressGhost(buf.ghost1, t == 1);
                    if (second) {
                        pressGhost(buf.ghost2, t == 1);
                    }
                }
            }
            // v2.16: a dash lasts as long as you hold, so the ghost doesn't let go mid-dash.
            if (down && releaseAfter >= 0 && tick == delay + releaseAfter && buf.ghost1->m_isDashing) {
                ++releaseAfter;
            }
            if (releaseAfter >= 0 && tick == delay + releaseAfter) {
                // let go of a press (or, for a wave release, press again: the turn back)
                pressGhost(buf.ghost1, !down);
                if (second) {
                    pressGhost(buf.ghost2, !down);
                }
            }
            if (buf.plan) {
                for (int t : *buf.plan) {
                    if (t == tick) {
                        bool now = !buf.held1;
                        pressGhost(buf.ghost1, now);
                        if (second) {
                            pressGhost(buf.ghost2, now);
                        }
                    }
                }
            }

            // v2.18: with CBF the click lands partway through a tick, so the ghost's first
            // step is only the rest of that tick; after it, ghost and game are on the same
            // tick boundaries again.
            float stepDt = tick == 0 ? dt * buf.firstStep : dt;
            advanceGhost(buf.ghost1, stepDt);
            if (second) {
                advanceGhost(buf.ghost2, stepDt);
            }
            releaseAutoClicks();
            if (buf.ghostDied) {
                died = true;
                break;
            }

            // v2.36: after a mid-tick (CBF) click the game finishes the tick and then runs its
            // normal collision check with the full step, so the ghost does the same.
            checkGhost(layer, buf.ghost1, dt);
            if (!buf.ghostDied && second) {
                checkGhost(layer, buf.ghost2, dt);
            }
            if (buf.ghostDied) {
                died = true;
                break;
            }
            if (buf.yBand && std::abs(buf.ghost1->getPositionY() - buf.yBandCenter) > buf.yBandHalf) {
                died = true;  // test 45: out of the route's band (not a real death)
                break;
            }
            if (buf.recordPath) {
                buf.recordPath->push_back(buf.ghost1->getPosition());
            }
            if (buf.cpSink && buf.cpEvery > 0 && (tick + 1) % buf.cpEvery == 0) {
                buf.cpSink(tick + 1, buf.ghost1, buf.held1);  // test 27: a route state to go on from
            }
            if (buf.ghostHitPortal) {
                safe = tick >= delay;
                break;
            }

            if (tick < delay) {
                continue;
            }
            ++survived;
            if (survived >= stopAt) {
                break;
            }
            // v2.79: after a portal changed the ghost's mode, only surviving the look-ahead counts
            // (the cube's "landed = safe" rule doesn't fit the new mode)
            if ((flying && !ufoRun) || buf.ghostSwitched || buf.fullHorizon) {
                continue;
            }

            bool grounded = buf.ghost1->m_isOnGround && (!second || buf.ghost2->m_isOnGround);
            if (grounded && airTicks > 0 && buf.captureKiller) {
                buf.baseTouchedDown = true;
            }
            if (!grounded) {
                ++airTicks;
                groundTicks = 0;
                continue;
            }
            // v2.15 spider: a flip is a teleport with no air time, so the cube's "was in the
            // air, then landed" never happened and every spider run went the full 130 ticks
            // into obstacles that need your next click. Now: standing on a surface for
            // SPIDER_LAND ticks after the input = the flip is done and safe.
            if (spiderRun) {
                if (++groundTicks >= st(SPIDER_LAND)) {
                    safe = true;
                    break;
                }
                continue;
            }
            // v2.61: v2.48 landing rule (the v2.51 ceiling-bump shortcut is gone)
            if (airTicks < st(ballRun ? 2 : 6)) {
                continue;
            }
            if (groundTicks > st(ballRun ? 8 : 2)) {
                safe = true;
                break;
            }
            ++groundTicks;
        }

        restoreScripted();
        if (!movers.empty()) {
            placeMovers(0);  // back exactly where they were
            movers.clear();
        }
        ++buf.simRuns;
        buf.simTicks += tick + 1;

        // Died or hit an unsimulated portal before the delayed input even fired:
        // longer delays can't do better, stop searching.
        if ((died || buf.ghostHitPortal) && tick < delay) {
            *end = SimEnd::StopSearch;
        }
        restoreObjects();
        buf.ghost1->stopAllActions();
        if (buf.ghost2) {
            buf.ghost2->stopAllActions();
        }
        // v2.78.3: a ghost that used an orb or dashed started its particles (orb burst, dash
        // fire) right where the orb is, before you got there
        if (buf.firstOrbTick >= 0) {
            buf.ghost1->stopParticles();
            if (buf.ghost2) {
                buf.ghost2->stopParticles();
            }
        }
        buf.lastRunSwitched = buf.ghostSwitched;
        buf.simulating = false;
        return safe ? horizon : survived;
    }
