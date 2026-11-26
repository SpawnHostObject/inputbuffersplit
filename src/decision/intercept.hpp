//@ decision/intercept.hpp
//@ Part of main.cpp (single translation unit). Not a standalone file: do not add it to the build.
    bool intercept(GJBaseGameLayer* self, bool down, bool isPlayer1) {
        ClickTimer timer;
        clickBudget();
        auto* layer = GameManager::get()->m_playLayer;
        if (!layer) {
            return false;
        }
        // v2.85 no-hitch: a search going on from an earlier tick starts after the delays it
        // already tried. A new input of yours meanwhile ends it: the waiting click becomes a
        // plain delayed click, and this input follows it as usual.
        bool resuming = resumeNow;
        searchFrom = resuming ? std::max(1, resumeEntry.from) : 1;
        budgetCutK = 0;
        struct SearchFromReset {
            ~SearchFromReset() {
                searchFrom = 1;
            }
        } searchFromReset;
        if (!resuming) {
            for (auto& q : buf.queue) {
                q.resume = false;
            }
        }
        // v2.80: the mod already clicked for you here; your own late click (press and its
        // release) is swallowed so it doesn't jump a second time
        if (buf.autoClicked && isPlayer1 && !resuming) {
            if (!down) {
                buf.autoClicked = false;
            }
            say("AUTO click: your own {} ignored (already clicked for you)", down ? "press" : "release");
            return true;
        }

        auto* level = layer->m_level;
        bool twoPlayer = level && level->m_twoPlayerMode;
        // v2.55: a P2-side input in a two-player level only steers Player 2 while dual mode is
        // on; before that it moves Player 1 (Relief's wave parts), so it's buffered like a P1 input.
        if (twoPlayer && !isPlayer1 && layer->m_gameState.m_isDualMode) {
            return false;
        }
        // In two-player levels P2 is someone else: never simulate P1's input on it.
        bool dual = !twoPlayer && layer->m_gameState.m_isDualMode && layer->m_player2;

        if (!resuming) {
            buf.isPlayer1 = isPlayer1;
            buf.realInput = down;
        }
        if (buf.autoReleased && !resuming) {
            // the game already let go for you: your real release has nothing left to do
            buf.autoReleased = false;
            buf.autoDash = false;
            if (!down) {
                return true;
            }
        }
        if (!buf.enabled) {
            buf.appliedInput = down;
            return false;
        }
        if (buf.practiceOnly && !layer->m_isPracticeMode) {
            buf.appliedInput = down;
            return false;
        }
        // v2.4: the ghost never presses left/right, so platformer predictions are meaningless.
        if (layer->m_isPlatformer) {
            buf.appliedInput = down;
            return false;
        }

        auto* p1 = self->m_player1;
        auto* p2 = dual ? layer->m_player2 : nullptr;

        // v2.18 Click Between Frames: a click that arrives in the middle of P1's physics step
        // landed `midTick` of the way through the tick (measured from how far P1 has moved
        // since the tick started). The ghost then starts with the rest of that tick, and
        // delayed inputs are queued one tick sooner, because the game's next tick boundary
        // is already the ghost's tick 1.
        float midTick = 0.f;
        if (buf.cbfSync && buf.inRealUpdate && p1 && std::abs(buf.tickDx) > 0.01f && !resuming) {
            midTick = std::clamp((p1->getPosition().x - buf.tickStartX) / buf.tickDx, 0.f, 0.999f);
        }
        buf.firstStep = 1.f - midTick;
        struct FirstStepReset {
            ~FirstStepReset() {
                buf.firstStep = 1.f;
            }
        } firstStepReset;
        int early = midTick > 0.f ? 1 : 0;

        bool anyShip = p1 && (p1->m_isShip || (p2 && p2->m_isShip));
        if (!p1 || p1->m_isDead || (anyShip && !buf.shipBuffer)) {
            resetPending();
            buf.appliedInput = down;
            return false;
        }

        if (down && buf.queue.empty()) {
            buf.releaseShift = 0;
        }

        bool p1Wave = p1->m_isDart;
        bool anyWave = p1Wave || (p2 && p2->m_isDart);
        bool mini = (p1Wave && p1->m_vehicleSize < 1.f) || (p2 && p2->m_isDart && p2->m_vehicleSize < 1.f);

        // Wave spam: don't fight fast inputs. Counted for every input, including ones made
        // while a delayed input is waiting (v2.5.1 skipped those, so a spam that started
        // with a buffered click never reached the streak and its taps got collapsed).
        // Once the streak is reached, anything still waiting is dropped and your inputs
        // go straight to the game until you pause for longer than the spam gap.
        // v2.44: ship is steered by feathering, so it gets its own, much shorter spam gap;
        // otherwise nearly every ship input counted as spam and was never buffered.
        int spamGap = st(anyShip && !anyWave ? std::clamp(buf.shipSpamGap, 2, 40) : std::clamp(buf.spamGap, 4, 40));
        if (!resuming) {
            if (buf.sinceInput < spamGap) {
                ++buf.rapid;
            }
            else {
                buf.rapid = 0;
            }
            buf.sinceInput = 0;
        }
        bool anyUfo = p1->m_isBird || (p2 && p2->m_isBird);
        bool anyBall = p1->m_isBall || (p2 && p2->m_isBall);
        bool anySwing = p1->m_isSwing || (p2 && p2->m_isSwing);
        if ((anyWave || anyShip || (anyUfo && buf.ufoSmart) || (anyBall && buf.ballSmart) || (anySwing && buf.swingSmart)) &&
            buf.rapid >= buf.spamStreak) {
            if (buf.explain) {
                say("pass {}: spam ({} fast inputs in a row, gap < {}t)", down ? "press" : "release", buf.rapid, spamGap);
            }
            // v2.47: path view marks inputs that skipped prediction (gray dot on you), so a
            // missing path isn't mistaken for a broken mod
            if (buf.showPath && buf.pathNode) {
                buf.pathNode->clear();
                buf.pathNode->drawDot(p1->getPosition(), 4.f, { 0.6f, 0.6f, 0.65f, 0.9f });
            }
            // v2.63.2: waiting inputs are played one per tick, in order, starting next tick, with
            // this one right after them. v2.61 dropped them (like v2.48), so a turn you made
            // just never happened; v2.49 sent them all at once, which turned a waiting press +
            // this release into a zero-length tap.
            if (!buf.queue.empty()) {
                int tick = 0;
                for (auto& q : buf.queue) {
                    q.ticksLeft = tick++;
                }
                queueInput(tick, down);
                return true;
            }
            buf.appliedInput = down;
            return false;
        }

        // v4.6 wave mode only: while a delayed input waits, a quick opposite input fires
        // one tick after it; pressing the same way again cancels that. (The default wave
        // mode shifts it like the cube does, below, so your zigzag keeps its shape.)
        if (!buf.queue.empty() && anyWave && buf.waveV46) {
            auto first = buf.queue.front();
            buf.queue.erase(buf.queue.begin() + 1, buf.queue.end());
            if (down != first.down) {
                queueInput(first.ticksLeft + 1, down);
            }
            return true;
        }

        // A delay is in progress: shift this input by the same amount so your
        // rhythm (tap length, hold length, follow-up clicks) is preserved.
        if (!buf.queue.empty()) {
            int after = buf.queue.back().ticksLeft;
            // low latency: a new press isn't shifted, it goes right after what's already queued
            queueInput(buf.lowLatency && down ? after : std::max(buf.shift - early, after), down);
            return true;
        }

        // Releases only matter for wave and robot.
        if (!p1Wave) {
            bool robot = p1->m_isRobot;
            bool wave2 = false;
            if (p2 && !robot) {
                wave2 = p2->m_isDart;
                if (!wave2) {
                    robot = p2->m_isRobot;
                }
            }
            // v2.16: letting go ends a dash, so dash releases are predicted like robot ones.
            bool dashing = p1->m_isDashing || (p2 && p2->m_isDashing);
            if (!wave2 && !down && !robot && !dashing && !anyShip) {
                if (buf.releaseShift > 0) {
                    // the press was delayed, so its release is delayed by the same amount:
                    // the hold length the ghost was tested with is the one the game gets
                    buf.shift = buf.releaseShift;
                    buf.releaseShift = 0;
                    queueInput(buf.shift, false);
                    return true;
                }
                buf.appliedInput = false;
                return false;
            }
        }

        bool anyFlyer = isFlying(p1) || (p2 && isFlying(p2));  // wave / ufo / swing
        int horizon = st(anyFlyer ? 48 : std::clamp(buf.groundHorizon, 48, 240));
        if (anyWave) {
            horizon = fullWaveHorizon(mini);
        }
        int maxDelay = st(anyWave ? buf.ticksWave : (anyShip ? std::clamp(buf.shipMaxDelay, 0, 16) : buf.ticksAll));
        if (buf.lowLatency) {
            maxDelay = std::min(maxDelay, st(4));  // at most 1 frame (60 fps) of delay
        }
        if (buf.perMode) {
            maxDelay = st(buf.modeTicks[modeIndex(p1, p2)]);  // v2.88: your frames for this mode (over low latency's cap)
        }
        // v3.0: a mode (normal or mini size) switched off on the Modes page gets raw clicks
        {
            int mi = modeIndex(p1, p2);
            PlayerObject* who = (mi == 4 && p2 && p2->m_isDart && !p1->m_isDart) ? p2 : p1;
            bool mini = who->m_vehicleSize < 0.95f;
            if (buf.modeOff & (1u << (mi * 2 + (mini ? 1 : 0)))) {
                maxDelay = 0;
            }
        }
        if (resuming) {
            maxDelay -= resumeEntry.elapsed;  // the time since your click counts
            if (maxDelay < searchFrom) {
                maxDelay = 0;
            }
        }
        if (maxDelay <= 0) {
            if (buf.explain) {
                say("pass {}: max delay is 0 for this mode", down ? "press" : "release");
            }
            buf.appliedInput = down;
            return false;
        }

        // Wave releases are predicted separately, so hold there. Robot jump height depends
        // on the hold, so use your usual robot hold (once known); everything else (cube,
        // ball, ufo, spider, swing) re-triggers while held, so model a tap.
        bool anyRobot = p1->m_isRobot || (p2 && p2->m_isRobot);
        int releaseAfter = -1;
        if (down && !anyWave && !anyShip) {
            releaseAfter = anyRobot ? robotHold.pick(-1, 50) : holdEstimate();
        }

        // On the ground a press is a jump; only move it onto an orb if the setting says so.
        bool grounded = p1->m_isOnGround && (!p2 || p2->m_isOnGround);
        if (buf.explain && down && grounded && !buf.orbAlignGround && !anyWave && !anyShip && buf.orbAlign && buf.smart && !resuming) {
            // v2.42: is there an orb right ahead that this ground jump could have been moved onto?
            SimEnd probeEnd;
            buf.skipPress = true;
            buf.orbClick = false;
            int window = st(std::clamp(buf.orbWindow, 1, 64));
            simulate(layer, p1, p2, dual, 0, true, window + 2, -1, window + 2, &probeEnd);
            buf.skipPress = false;
            buf.orbClick = buf.assumeOrbs;
            if (buf.firstOrbTick >= 0 && buf.firstOrbTick <= window) {
                say("orb: you'd touch an orb at t{}, but this press is from the ground ('also from the ground' is off)",
                    buf.firstOrbTick);
            }
        }
        if (down && !anyWave && !anyShip && buf.orbAlign && buf.smart && (!grounded || buf.orbAlignGround) && !resuming) {
            int orbLanded = 0;
            int k = memorySaysSkip(sliceOf(p1)) ? 0 : alignOrb(layer, p1, p2, dual, horizon, releaseAfter, memoryDelay(sliceOf(p1)), maxDelay, &orbLanded);
            if (k > 0 && trainInstead(layer, k, down, "orb")) {
                decided(layer, p1, p2, dual, 0, down, horizon, releaseAfter);
                buf.appliedInput = down;
                return false;
            }
            if (k > 0) {
                // v2.80.1: says so when it isn't a full save (the orb is used, but a crash still follows)
                say("ORB {} +{}t ({:g}f) -> {}t{}", buf.orbWasTiming ? "timing" : "press", k, k * TICK, orbLanded,
                    orbLanded < horizon ? fmt::format(" (not a full save: still crashes in {}t)", orbLanded) : std::string());
                noteSave(layer, k);
                decided(layer, p1, p2, dual, k, down, horizon, releaseAfter);
                buf.shift = k;
                queueInput(k - early, down);
                return true;
            }
        }

        buf.sawMover = false;
        buf.trackMovers = buf.avoidMovers || buf.predictMovers;
        SimEnd end;
        bool cubeStyleWave = anyWave && !buf.waveV46;
        bool robotPress = down && anyRobot && !anyWave && buf.robotSmart;
        // v2.82 robot release: letting go sets the jump's height, so it's judged like the press
        // (the jump has to land safely) instead of "lives a bit longer than letting go now"
        bool robotRelease = !down && anyRobot && !anyWave && !anyShip && buf.robotSmart;
        bool anySpider = p1->m_isSpider || (p2 && p2->m_isSpider);
        bool spiderPress = down && anySpider && !anyWave && !robotPress && buf.spiderSmart;
        bool ufoPress = down && anyUfo && !anyWave && !anyShip && buf.ufoSmart;  // v2.40
        // v2.51 swing: one flip, then nothing for the whole look-ahead, crashes into what your
        // next flip would have avoided. Scored like the UFO: could one more flip have saved it?
        bool swingPress = down && anySwing && !anyWave && !anyShip && !ufoPress && buf.swingSmart;
        ufoPress = ufoPress || swingPress;
        bool shipInput = anyShip && !anyWave;  // v2.25: press or release, both steer the ship
        buf.captureKiller = true;
        buf.baseTouchedDown = false;
        buf.killerSeen = false;
        buf.baseEscape = -1;
        int base = cubeStyleWave ? waveScore(layer, p1, p2, dual, 0, down, horizon, horizon, &end)
                 : robotPress    ? robotScore(layer, p1, p2, dual, 0, horizon, horizon, &end)
                 : spiderPress   ? spiderScore(layer, p1, p2, dual, 0, horizon, releaseAfter, horizon, &end)
                 : ufoPress      ? ufoScore(layer, p1, p2, dual, 0, horizon, releaseAfter, horizon, &end)
                 : shipInput     ? shipScore(layer, p1, p2, dual, 0, down, horizon, horizon, &end)
                                 : simulateOrbs(layer, p1, p2, dual, 0, down, horizon, releaseAfter, horizon, &end);
        int baseHold = robotUsedHold;  // v2.82: the hold that made pressing now safe
        bool baseSwitched = buf.lastRunSwitched;
        buf.captureKiller = false;
        buf.trackMovers = false;
        // v2.83: through a mode portal the ghost never clicks again, so the new mode's first
        // obstacle killed it. If one more click saves it, this input is fine as it is.
        if (baseSwitched && base < horizon && !cubeStyleWave && !shipInput && end != SimEnd::StopSearch && buf.baseEscape < 0) {
            int at = nextClickAt(layer, p1, p2, dual, 0, down, horizon, releaseAfter, base);
            if (at >= 0) {
                buf.baseEscape = at;
                buf.baseCrash = base;
                base = horizon;
            }
        }

        // v2.30 diagnostics: a ghost that dies within a few ticks while you live means the
        // prediction doesn't match you there. Log what killed it and how both hitboxes compare.
        if (buf.killerSeen && (base <= st(3) || (buf.deathLog && base < horizon))) {
            auto real = p1->getObjectRect();
            auto const& g = buf.ghostDeathRect;
            if (buf.killerType >= 0) {
                auto const& k = buf.killerRect;
                say("ghost killed at t{} by obj {} (type {}{}{}) at dx {:.1f} dy {:.1f} size {:.1f}x{:.1f}", buf.killerTick, buf.killerId,
                    buf.killerType, buf.killerNoTouch ? ", no-touch" : "", buf.killerDisabled ? ", disabled" : "",
                    k.getMidX() - real.getMidX(), k.getMidY() - real.getMidY(), k.size.width, k.size.height);
            }
            else {
                say("ghost killed at t{} by no object (crushed / out of bounds / slope)", buf.killerTick);
            }
            say("  hitbox ghost {:.1f}x{:.1f} vs you {:.1f}x{:.1f}, ghost moved dx {:.1f} dy {:.1f}{}{}", g.size.width, g.size.height,
                real.size.width, real.size.height, g.getMidX() - real.getMidX(), g.getMidY() - real.getMidY(),
                buf.predictMovers ? " (moving objects predicted)" : "",
                buf.triggersAhead > 0 ? fmt::format(" ({} move triggers ahead predicted)", buf.triggersAhead) : std::string());
            // v2.54: which players exist and which one your clicks move
            auto desc = [](PlayerObject* p) {
                if (!p) {
                    return std::string("none");
                }
                char const* mode = p->m_isDart ? "wave" : p->m_isShip ? "ship" : p->m_isBird ? "ufo" : p->m_isBall ? "ball"
                    : p->m_isRobot ? "robot" : p->m_isSpider ? "spider" : p->m_isSwing ? "swing" : "cube";
                return fmt::format("{}{}{}", mode, p->m_controlsDisabled ? " controls-off" : "", p->isVisible() ? "" : " hidden");
            };
            say("  players: P1 {} | P2 {} | dual {} | predicted {}", desc(layer->m_player1), desc(layer->m_player2),
                layer->m_gameState.m_isDualMode, p1 == layer->m_player1 ? "P1" : "P2");
        }

        // v2.36: with CBF, where inside the tick the click landed (0 = on the tick)
        std::string tickTag = midTick > 0.f ? fmt::format(" @{:.2f}", midTick) : std::string();
        if (base >= horizon) {
            // v2.43 more time at death spots. This input is only safe if your next input comes
            // in time, and level memory has seen you die here repeatedly: try small delays
            // that push the crash further away (or remove it), so you get more time to react.
            // Only at those spots, so normal play isn't delayed.
            bool reactMode = cubeStyleWave || shipInput || ufoPress || spiderPress;
            if (buf.moreTime && buf.learn && reactMode && buf.baseEscape >= 0 && spotAround(sliceOf(p1)).deaths >= 2) {
                int crash0 = buf.baseCrash;
                int bestK = 0;
                int bestCrash = crash0;
                int hold = (cubeStyleWave || shipInput) ? -1 : releaseAfter;
                // v2.67: wave / ship input that's already safe (if you keep reacting) is held back
                // at most 1 frame here. Up to 4f (16t) made fast zigzags at death spots feel late:
                // every later click in the rhythm gets shifted by the same amount.
                int moreMax = (cubeStyleWave || shipInput) ? std::min(maxDelay, st(4)) : maxDelay;
                for (int k = 1; k <= moreMax; ++k) {
                    SimEnd e;
                    if (clickOverTime()) {
                        break;
                    }
                    int ret = simulateOrbs(layer, p1, p2, dual, k, down, horizon, hold, horizon, &e);
                    if (e == SimEnd::StopSearch) {
                        break;
                    }
                    if (ret >= horizon) {
                        bestK = k;
                        bestCrash = -1;  // no crash at all
                        break;
                    }
                    int crashAt = k + ret;  // ticks from now
                    if (crashAt >= crash0 + st(8) && crashAt > bestCrash) {
                        bestK = k;
                        bestCrash = crashAt;
                    }
                }
                if (bestK > 0 && !buf.train) {
                    noteSave(layer, bestK);
                    say("MORE TIME {}{} +{}t: crash t{} -> {} (died here {}x)", down ? "press" : "release", tickTag, bestK, crash0,
                        bestCrash < 0 ? std::string("none") : fmt::format("t{}", bestCrash), spotAround(sliceOf(p1)).deaths);
                    decided(layer, p1, p2, dual, bestK, down, horizon, releaseAfter);
                    buf.shift = bestK;
                    queueInput(bestK - early, down);
                    return true;
                }
            }
            if (buf.explain) {
                if (buf.baseEscape >= 0) {
                    say("pass {}{}: safe only if you react by t{} (else crash at t{})", down ? "press" : "release", tickTag,
                        buf.baseEscape, buf.baseCrash);
                }
                else {
                    say("pass {}{}: safe now (survives the {}t look-ahead)", down ? "press" : "release", tickTag, horizon);
                }
            }
            // v2.82: a robot press that's only safe with a hold unlike your usual one
            if (robotPress && baseHold > 0) {
                int usual = robotHold.pick(st(20), 50);
                if (std::abs(baseHold - usual) >= st(3)) {
                    if (buf.explain) {
                        say("robot: safe with a {} hold ({}t, you usually hold {}t)", baseHold > usual ? "longer" : "shorter", baseHold,
                            usual);
                    }
                    releaseAfter = baseHold;  // the path view / drift check show the hold that works
                }
            }
            decided(layer, p1, p2, dual, 0, down, horizon, releaseAfter);
            buf.appliedInput = down;
            return false;
        }

        // v2.27: a cube / ball press that only crashes after a normal jump would have landed
        // (~90 ticks) dies to the next obstacle, not this click: no delay helps, so don't
        // spend the whole time budget searching (it was most of the per-click lag).
        bool groundPress = down && !anyFlyer && !anyRobot && !anySpider && !anyWave;
        // v2.63: only once the jump came back down. A long jump (Acheron's first: ~94 ticks in
        // the air, then onto a spike) crashes late too, and that crash *is* this click's.
        if (groundPress && buf.lateCrash > 0 && base >= st(buf.lateCrash) && buf.baseTouchedDown) {
            if (buf.explain) {
                say("pass press: crash only at {}t, after the jump would land (late crash rule)", base);
            }
            decided(layer, p1, p2, dual, 0, down, horizon, releaseAfter);
            buf.appliedInput = down;
            return false;
        }

        // v2.13 memory: saves at this spot kept ending in deaths, so the prediction can't be
        // trusted here. Leave the input alone.
        int slice = sliceOf(p1);
        if (memorySaysSkip(slice)) {
            say("memory: saves here failed before, input left alone");
            decided(layer, p1, p2, dual, 0, down, horizon, releaseAfter);
            pending.skipped = true;
            buf.appliedInput = down;
            return false;
        }

        int landed = 0;
        int saved = 0;
        bool fromMemory = false;
        crashFixed = false;
        // v2.13 memory: the delay that worked here on earlier runs is tried first, so the
        // same gap gets the same save every time.
        // The same judgement this mode's search uses, for a single delay.
        auto scoreAt = [&](int k, int target, SimEnd* e) {
            return cubeStyleWave ? waveScore(layer, p1, p2, dual, k, down, horizon, target, e)
                 : robotPress    ? robotScore(layer, p1, p2, dual, k, horizon, target, e)
                 : spiderPress   ? spiderScore(layer, p1, p2, dual, k, horizon, releaseAfter, target, e)
                 : ufoPress      ? ufoScore(layer, p1, p2, dual, k, horizon, releaseAfter, target, e)
                 : shipInput     ? shipScore(layer, p1, p2, dual, k, down, horizon, target, e)
                                 : simulateOrbs(layer, p1, p2, dual, k, down, horizon, releaseAfter, target, e);
        };

        // v2.69: a cube / ball jump save has to land safely. A save that only lived 12 ticks
        // longer than pressing now still crashed later in the same jump (Acheron's first jump:
        // +32t..+40t "saves" that died ~130 ticks later, every time); those clicks now go
        // through untouched instead of being moved by up to 10 frames for nothing.
        // v2.75: robot jumps and UFO / swing flaps too (no more half saves there either)
        int needed = (groundPress || robotPress || robotRelease || ufoPress) ? horizon : std::min(horizon, base + st(12));
        int learned = memoryDelay(slice);
        if (learned >= searchFrom && learned <= maxDelay && !(anyWave && buf.waveV46)) {
            int target = needed;
            SimEnd e;
            int ret = scoreAt(learned, target, &e);
            if (ret >= target) {
                saved = learned;
                landed = ret;
                fromMemory = true;
            }
        }
        if (saved <= 0) {
            saved = cubeStyleWave ? searchWave(layer, p1, p2, dual, down, horizon, maxDelay, base, &landed)
                  : robotPress    ? searchTwoPass(horizon, maxDelay, base, &landed, [&](int k, int target, SimEnd* e) {
                        return robotScore(layer, p1, p2, dual, k, horizon, target, e);
                    }, true)
                  : robotRelease  ? searchSmallest(horizon, maxDelay, base, &landed, [&](int k, int target, SimEnd* e) {
                        return simulateOrbs(layer, p1, p2, dual, k, false, horizon, -1, target, e);
                    }, searchBudget(), false, true)
                  : spiderPress   ? searchTwoPass(horizon, maxDelay, base, &landed, [&](int k, int target, SimEnd* e) {
                        return spiderScore(layer, p1, p2, dual, k, horizon, releaseAfter, target, e);
                    })
                  : shipInput     ? searchSmallest(horizon, maxDelay, base, &landed, [&](int k, int target, SimEnd* e) {
                        return shipScore(layer, p1, p2, dual, k, down, horizon, target, e);
                    })
                  : ufoPress      ? searchTwoPass(horizon, maxDelay, base, &landed, [&](int k, int target, SimEnd* e) {
                        return ufoScore(layer, p1, p2, dual, k, horizon, releaseAfter, target, e);
                    }, true)
                                  : searchDelay(layer, p1, p2, dual, down, anyWave, horizon, maxDelay, releaseAfter, base, &landed,
                                        groundPress);
        }
        // v2.85 no-hitch: the search ran out of time before trying every delay. Your click waits
        // one tick and the search goes on from there with fresh time (the delays it already
        // tried all crashed, so waiting a tick loses nothing), instead of giving up.
        int resumes = resuming ? resumeEntry.resumes : 0;
        if (saved <= 0 && buf.noHitch && budgetCutK > 0 && budgetCutK < maxDelay && resumes < MAX_RESUMES &&
            !(anyWave && buf.waveV46)) {
            QueuedInput next { std::max(0, 1 - early), down, true, budgetCutK, (resuming ? resumeEntry.elapsed : 0) + 1, resumes + 1 };
            if (buf.explain) {
                say("search goes on next tick (tried up to +{}t of {}t)", budgetCutK + (resuming ? resumeEntry.elapsed : 0),
                    maxDelay + (resuming ? resumeEntry.elapsed : 0));
            }
            buf.shift = 1;
            buf.queue.push_back(next);
            return true;
        }
        if (saved <= 0) {
            say("no save: {}{} dies in {}t (max {}t, best +{}t -> {}t of {}){}", down ? "press" : "release", tickTag, base, maxDelay,
                dbgBestK, dbgBestRet, horizon, !crashFixed ? "" : base < st(30) ? " - too late: waiting only makes it crash sooner" : " - same crash at any timing (next obstacle)");
            decided(layer, p1, p2, dual, 0, down, horizon, releaseAfter);
            buf.appliedInput = down;
            return false;
        }

        // v2.28 robustness check. A save can work in the exact predicted conditions and still
        // fail for real, because a real run is always a hair off (a fraction of a tick, a bit
        // of float error). So the chosen delay must also survive with you starting slightly
        // ahead of and behind where you are (half a tick of movement each way). If it only
        // works at the exact spot, the next few delays are tried for one that survives all
        // three; if none does, the original save is kept, so this never makes things worse.
        bool robustNote = false;
        if (buf.robustSaves && !(anyWave && buf.waveV46)) {
            int target = needed;
            // half a tick of x movement: normal speed (0.9) covers 311.58 units a second
            float shift = std::max(0.3f, 0.5f * (311.58f / 240.f) * (std::abs(p1->m_playerSpeed) / 0.9f));
            auto robust = [&](int k) {
                for (float off : { shift, -shift }) {
                    buf.ghostShiftX = off;
                    SimEnd e;
                    int ret = scoreAt(k, target, &e);
                    buf.ghostShiftX = 0.f;
                    if (ret < target) {
                        return false;
                    }
                }
                return true;
            };
            if (clickMs() < buf.budgetMs && !robust(saved)) {
                for (int k = saved + 1; k <= std::min(maxDelay, saved + st(3)) && !clickOverTime(); ++k) {
                    SimEnd e;
                    int ret = scoreAt(k, target, &e);
                    if (ret >= target && robust(k)) {
                        saved = k;
                        landed = ret;
                        robustNote = true;
                        break;
                    }
                }
            }
        }

        if (buf.avoidMovers) {
            // One extra run of the chosen delay with the moving-object watch on. (Watching
            // in every search run made cube searches hit the time budget.)
            buf.trackMovers = true;
            simulateOrbs(layer, p1, p2, dual, saved, down, horizon, releaseAfter, horizon, &end);
            buf.trackMovers = false;
        }
        if (buf.avoidMovers && buf.sawMover) {
            say("skip +{}t save: moving objects ahead", saved);
            decided(layer, p1, p2, dual, 0, down, horizon, releaseAfter);
            buf.appliedInput = down;
            return false;
        }

        if (trainInstead(layer, saved, down, swingPress ? "swing" : "")) {
            decided(layer, p1, p2, dual, 0, down, horizon, releaseAfter);
            buf.appliedInput = down;
            return false;
        }
        if (robotPress) {
            releaseAfter = robotUsedHold;  // the path view / drift check show the hold that works
        }
        noteSave(layer, saved);
        say("SAVED {}{} +{}t ({:g}f)  {}->{}  [max {}]{}{}", down ? "press" : "release", tickTag, saved, saved * TICK, base, landed, maxDelay,
            resuming ? fmt::format("  (search went on {}t)", resumeEntry.elapsed) : std::string(),
            robustNote ? std::string("  (robust)")
            : fromMemory ? std::string("  (memory)")
            : cubeStyleWave && marginUsed ? std::string("  +margin")
            : robotPress ? fmt::format("  hold {}t", robotUsedHold) : std::string());
        decided(layer, p1, p2, dual, saved, down, horizon, releaseAfter);
        buf.shift = saved;
        queueInput(saved - early, down);
        return true;
    }
