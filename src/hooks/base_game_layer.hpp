//@ hooks/base_game_layer.hpp
//@ Part of main.cpp (single translation unit). Not a standalone file: do not add it to the build.
//@ 
//@ 
//@ 
//@ 
// v2.57 custom TPS: above 240 the level is stepped in 1/tps pieces. update() runs once per
// step and getModifiedDelta() hands the step through unrounded, so each call is exactly one
// physics step. (Below 240 the game's own step count would need patching, so 240 is the floor.)
static bool tpsDriving = false;
static double tpsCarry = 0.0;

class $modify(BufBaseGameLayer, GJBaseGameLayer) {
    // test 72: macro inputs go in here, at the start of the step, the way bots press them
    void processCommands(float dt, bool isHalfTick, bool isLastTick) {
        if (autoplay.active && !buf.simulating && static_cast<GJBaseGameLayer*>(autoplay.layer) == static_cast<GJBaseGameLayer*>(this)) {
            macroStep(autoplay.layer);
        }
        GJBaseGameLayer::processCommands(dt, isHalfTick, isLastTick);
        if (autoplay.macroXd && autoplay.active && !buf.simulating && static_cast<GJBaseGameLayer*>(autoplay.layer) == static_cast<GJBaseGameLayer*>(this)) {
            macroStepXd(autoplay.layer);
        }
    }

    void update(float dt) {
        // test 93: the setting is read twice a second, not every frame
        static int tpsSetting = 240;
        static int tpsAge = 1000;
        if (++tpsAge > 30) {
            tpsAge = 0;
            tpsSetting = static_cast<int>(Mod::get()->getSettingValue<int64_t>("tps"));
        }
        // v3.0 test 14: while autoplay plays, physics steps are exactly 1/240 s even at the normal
        // 240 TPS. The game's own steps are frame time / step count, so the same inputs moved
        // you a little differently each attempt (replays went differently, the ghost guessed wrong)
        bool fixedStep = autoplay.active && static_cast<GJBaseGameLayer*>(autoplay.layer) == static_cast<GJBaseGameLayer*>(this);
        int tps = (fixedStep && tpsOverride > 0) ? tpsOverride : tpsSetting;
        if ((tps <= 240 && !fixedStep) || tpsDriving || !typeinfo_cast<PlayLayer*>(this)) {
            GJBaseGameLayer::update(dt);
            return;
        }
        double step = 1.0 / std::max(tps, 240);
        if (fixedStep) {
            auto* pl = static_cast<PlayLayer*>(static_cast<GJBaseGameLayer*>(this));
            if (autoplay.needRestart) {
                autoplay.needRestart = false;
                tpsCarry = 0.0;
                pl->resetLevel();
                return;
            }
            if (autoplay.generating) {
                // v3.0 test 25: the hidden run, as many steps as fit in ~30 ms of this frame
                auto begin = std::chrono::steady_clock::now();
                tpsDriving = true;
                int steps = 0;
                while (autoplay.generating && steps < 20000 &&
                       std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - begin).count() < 30.0) {
                    GJBaseGameLayer::update(static_cast<float>(step));
                    ++steps;
                    if (!autoplay.generating || !autoplay.active) {
                        break;
                    }
                    if (pl->m_player1 && pl->m_player1->m_isDead) {
                        updateGenText(pl);
                        ++autoplay.genAttempts;
                        tpsDriving = false;
                        pl->resetLevel();
                        tpsDriving = true;
                        if (autoplay.sameDeaths >= 40) {
                            endGenerating("stuck at one spot, plays what it has");
                        }
                    }
                }
                tpsDriving = false;
                tpsCarry = 0.0;
                if (autoplay.generating) {
                    updateGenText(pl);
                    FMODAudioEngine::sharedEngine()->pauseAllAudio();
                }
                if (autoplay.needRestart) {
                    autoplay.needRestart = false;
                    pl->resetLevel();
                }
                return;
            }
        }
        tpsCarry += dt;
        int steps = static_cast<int>(tpsCarry / step);
        int cap = std::max(1, std::max(tps, 240) / 15);  // after a lag spike, catch up at most 1/15 s
        if (steps > cap) {
            steps = cap;
            tpsCarry = 0.0;
        }
        else {
            tpsCarry -= steps * step;
        }
        tpsDriving = true;
        // alpha.27: an xdBot macro made at a lower FPS: several physics steps a game frame, as then
        // (alpha.28: off: the game didn't take several steps a frame, it only slowed down)
        int chunk = 1;
        if (chunk > 1) {
            static int heldSteps = 0;
            heldSteps += steps;
            while (heldSteps >= chunk) {
                GJBaseGameLayer::update(static_cast<float>(step * chunk));
                heldSteps -= chunk;
            }
        }
        else {
            for (int i = 0; i < steps; ++i) {
                GJBaseGameLayer::update(static_cast<float>(step));
            }
        }
        tpsDriving = false;
    }

    double getModifiedDelta(float dt) {
        if (!tpsDriving) {
            return GJBaseGameLayer::getModifiedDelta(dt);
        }
        m_extraDelta = 0.0;
        if (m_resumeTimer > 0) {
            --m_resumeTimer;
            return 0.0;
        }
        return dt;
    }

    static void onModify(auto& self) {
        (void)self.setHookPriority("GJBaseGameLayer::canBeActivatedByPlayer", HOOK_PRIORITY);
        (void)self.setHookPriority("GJBaseGameLayer::handleButton", HOOK_PRIORITY);
        (void)self.setHookPriority("GJBaseGameLayer::collisionCheckObjects", HOOK_PRIORITY);
        (void)self.setHookPriority("GJBaseGameLayer::playerTouchedRing", HOOK_PRIORITY);
        (void)self.setHookPriority("GJBaseGameLayer::flipGravity", HOOK_PRIORITY);
        (void)self.setHookPriority("GJBaseGameLayer::gameEventTriggered", HOOK_PRIORITY);
        (void)self.setHookPriority("GJBaseGameLayer::playerWillSwitchMode", HOOK_PRIORITY);
    }

    // During a simulation the ghost may use each object once; the real activation
    // state is snapshotted and restored afterwards so the real player is unaffected.
    bool canBeActivatedByPlayer(PlayerObject* player, EffectGameObject* object) {
        if (buf.simulating) {
            if (!buf.smart || !isGhost(player) || !object) {
                return false;
            }
            // v2.16: an orb can be checked on every tick you're inside it. Before, the first
            // check already "spent" it, so the ghost could only click an orb on the very
            // first tick it touched it. Now an orb is only spent once the ghost used it.
            if (typeinfo_cast<RingObject*>(object)) {
                if (buf.orbsUsed.contains(object)) {
                    return false;
                }
                snapshotObject(object);
                return true;
            }
            if (buf.simUsed.contains(object)) {
                return false;
            }
            snapshotObject(object);
            return true;
        }
        return GJBaseGameLayer::canBeActivatedByPlayer(player, object);
    }

    void collisionCheckObjects(PlayerObject* player, gd::vector<GameObject*>* objects, int objectCount, float dt) {
        if (!buf.simulating) {
            GJBaseGameLayer::collisionCheckObjects(player, objects, objectCount, dt);
            return;
        }
        static gd::vector<GameObject*> relevant;
        relevant.clear();
        if (static_cast<int>(relevant.capacity()) < objectCount) {
            relevant.reserve(objectCount);
        }

        // Hot loop: runs for every nearby object, every ghost tick. One type lookup per
        // object, no bounds-checked at(), and the ghost rect is only fetched if needed.
        uint64_t keep = buf.smart ? (COLLIDING_TYPES | INTERACT_TYPES) : COLLIDING_TYPES;
        bool haveRect = false;
        CCRect ghostRect;
        auto& list = *objects;
        bool noteSeen = buf.offscreenFill;
        for (int i = 0; i < objectCount; ++i) {
            auto* object = list[i];
            if (!object) {
                continue;
            }
            if (noteSeen) {
                buf.seenThisCheck.insert(object);
            }
            auto type = static_cast<unsigned>(object->m_objectType);
            uint64_t bit = type < 64 ? (1ull << type) : 0;
            if (keep & bit) {
                // v2.31: a hazard you're touching right now while alive is one the game doesn't
                // let kill you (a spike made to follow the player, noclip...). Seen in GRIEF:
                // a spike always sitting exactly on the player killed every wave prediction on
                // its first tick.
                // Decided on the first ticks and kept by object, so a spike that follows you
                // stays ignored even when "predict moving objects" moves it along.
                if (!buf.simDisabled.empty() && buf.simDisabled.contains(object)) {
                    continue;
                }
                if (buf.haveRealRect && (bit & HAZARD_TYPES)) {
                    if (!buf.harmless.empty() && buf.harmless.contains(object)) {
                        continue;
                    }
                    if (buf.simTick <= 1) {
                        auto const& r = object->getObjectRect();
                        if (r.intersectsRect(buf.realRect1) || r.intersectsRect(buf.realRect2)) {
                            buf.harmless.insert(object);
                            continue;
                        }
                    }
                }
                relevant.push_back(object);
                if (buf.trackMovers && (COLLIDING_TYPES & bit)) {
                    watchObject(object);
                }
                continue;
            }
            if (!buf.smart) {
                continue;
            }
            bool stop = (STOP_TYPES & bit) != 0;
            bool size = (SIZE_TYPES & bit) != 0;
            float speed = (stop || size) ? 0.f : speedPortalValue(object->m_objectID);
            if (!stop && !size && speed <= 0.f) {
                continue;
            }
            if (!haveRect) {
                ghostRect = player->getObjectRect();
                haveRect = true;
            }
            if (ghostRect.intersectsRect(object->getObjectRect())) {
                if (stop) {
                    // v2.78.1: a mode portal you're still inside when you click was already taken
                    // (you're in its mode now). It stopped every prediction at t0, so nothing right
                    // after a portal was predicted (Amethyst's first pink orb: the click was never
                    // moved onto the orb). Only portals ahead stop a prediction; teleport orbs
                    // still do, since they need a click.
                    auto const& r = object->getObjectRect();
                    if (buf.harmless.contains(object)) {
                        continue;
                    }
                    if (buf.haveStartRects && buf.simTick <= 1 && object->m_objectType != GameObjectType::TeleportOrb &&
                        (r.intersectsRect(buf.realRect1) || r.intersectsRect(buf.realRect2))) {
                        buf.harmless.insert(object);
                        continue;
                    }
                    // v2.79: mode portals ahead are taken by the ghost (once per run)
                    // v2.84: teleport and solo portals too
                    if (buf.portalPredict && isGhost(player) &&
                        (ghostTakePortal(player, object) || ghostTeleport(player, object) || ghostSoloPortal(object) ||
                            ghostDualPortal(player, object))) {
                        buf.harmless.insert(object);
                        continue;
                    }
                    buf.ghostHitPortal = true;
                }
                else if (size) {
                    bool mini = object->m_objectType == GameObjectType::MiniSizePortal;
                    if ((player->m_vehicleSize < 1.f) != mini) {
                        player->togglePlayerScale(mini, true);
                    }
                }
                else if (player->m_playerSpeed != speed) {
                    // v3.0 test 52: the game's own speed change (no effects), not just the speed
                    // number: with only the number, the ghost after a speed portal moved at a slightly
                    // wrong speed and the route fell behind you
                    player->updateTimeMod(speed, true);
                    player->m_playerSpeed = speed;
                }
            }
        }
        // v2.85: objects a predicted toggle switched on: the game leaves switched-off objects
        // out, so the ones near the ghost are added (switched on just for this check)
        static std::vector<GameObject*> woke;
        woke.clear();
        if (!buf.simEnabled.empty() && player) {
            auto pos = player->getPosition();
            for (auto* object : buf.simEnabled) {
                auto type = static_cast<unsigned>(object->m_objectType);
                uint64_t bit = type < 64 ? (1ull << type) : 0;
                if (!(keep & bit) || buf.simDisabled.contains(object) || std::abs(object->m_positionX - pos.x) > 200.f ||
                    std::abs(object->m_positionY - pos.y) > 200.f) {
                    continue;
                }
                if (object->m_isGroupDisabled) {
                    object->m_isGroupDisabled = false;
                    woke.push_back(object);
                }
                relevant.push_back(object);
            }
        }
        GJBaseGameLayer::collisionCheckObjects(player, &relevant, static_cast<int>(relevant.size()), dt);
        for (auto* object : woke) {
            object->m_isGroupDisabled = true;
        }
    }

    void handleButton(bool down, int button, bool isPlayer1) {
        // v3.0: while autoplay plays, your own clicks don't reach the game (Take Over: they do,
        // and the bot steps back for a moment after each one)
        if (autoplay.active && !buf.simulating && !buf.injecting) {
            if (!autoplay.takeOver) {
                return;
            }
            autoplay.userUntil = tickCount + st(down ? 90 : 60);
            autoplay.userTouched = true;
            autoplay.plan.clear();
            autoplay.verifyAt = -1;
        }
        if (!buf.simulating && button == 1 && !buf.injecting) {
            auto* p1 = this->m_player1;
            if (down || (p1 && (p1->m_isDart || p1->m_isRobot))) {
                finishDrift("");
            }
            recordRealInput(down, p1);
        }
        if (!buf.simulating && button == 1 && !buf.injecting) {
            // v2.21 performance log: what this click cost, measured around the whole decision.
            buf.simRuns = 0;
            buf.simTicks = 0;
            auto start = std::chrono::steady_clock::now();
            bool held = intercept(this, down, isPlayer1);
            sweepGhostLayer();
            if (buf.perfLog && buf.simRuns > 0) {
                double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
                say("perf: {:.2f} ms, {} runs, {} ghost ticks ({:.1f} us/tick)", ms, buf.simRuns, buf.simTicks,
                    ms * 1000.0 / static_cast<double>(std::max(1L, buf.simTicks)));
            }
            if (held) {
                return;
            }
        }
        if (!buf.simulating && button == 1) {
            benchInput(down);  // v2.75: what the game really gets (your clicks and delayed ones)
        }
        GJBaseGameLayer::handleButton(down, button, isPlayer1);
    }

    void playerTouchedRing(PlayerObject* player, RingObject* ring) {
        if (buf.simulating) {
            if (!buf.smart || !isGhost(player)) {
                return;
            }
            snapshotObject(ring);
            buf.touchedOrb = true;
            if (buf.firstOrbTick < 0) {
                buf.firstOrbTick = buf.simTick;
            }
            buf.lastOrbTick = buf.simTick;
            if (buf.orbUsedTick >= 0 && !buf.orbsUsed.contains(ring)) {
                buf.laterOrbTouched = true;
            }
        }
        GJBaseGameLayer::playerTouchedRing(player, ring);

        // v2.2 calculation mode: assume you click every orb you touch, on time. Without
        // this the ghost (which only has the one click being tested) misses the next orb in a
        // chain, falls, and the mod thinks a perfectly good click is a death.
        if (buf.simulating && buf.plan && player == buf.ghost1) {
            bool seen = false;
            for (size_t i = 0; i < buf.orbTouches.size(); ++i) {
                if (buf.orbTouches[i].first == ring) {
                    seen = true;
                    if (i < buf.orbTouchLast.size()) {
                        buf.orbTouchLast[i] = buf.simTick;
                    }
                }
            }
            if (!seen) {
                buf.orbTouches.push_back({ ring, buf.simTick });
                buf.orbTouchLast.resize(buf.orbTouches.size() - 1);
                buf.orbTouchLast.push_back(buf.simTick);
            }
        }
        bool autoOrb = buf.orbClick || (buf.orbAfterFirst && buf.orbUsedTick >= 0 && !buf.orbsUsed.contains(ring));
        if (buf.simulating && autoOrb) {
            bool first = player == buf.ghost1;
            bool& held = first ? buf.held1 : buf.held2;
            bool& autoClick = first ? buf.auto1 : buf.auto2;
            int& autoAt = first ? buf.autoAt1 : buf.autoAt2;
            if (buf.orbClickDelay > 0) {
                if (!held && autoAt == -1) {
                    autoAt = buf.simTick + buf.orbClickDelay;  // v2.26: click later inside the orb
                }
            }
            else if (!held && !autoClick) {
                player->pushButton(PlayerButton::Jump);
                autoClick = true;
            }
        }
    }

    // Gravity portals / blue pads / blue orbs: flip only the ghost, no layer effects.
    void flipGravity(PlayerObject* player, bool flip, bool noEffects) {
        if (buf.simulating) {
            if (buf.smart && isGhost(player)) {
                player->flipGravity(flip, true);
            }
            return;
        }
        GJBaseGameLayer::flipGravity(player, flip, noEffects);
    }

    // Never let ghost activity fire event triggers in the real level.
    void gameEventTriggered(GJGameEvent event, int material, int playerID) {
        if (buf.simulating) {
            return;
        }
        GJBaseGameLayer::gameEventTriggered(event, material, playerID);
    }

    // v2.74: nothing the hidden ghost touches during a prediction may change the real level.
    // Toggle orbs, orbs / pads / collectibles with groups, item and counter triggers could
    // otherwise toggle or spawn groups and change item counters for real (once per
    // prediction run, many times a click), which broke triggers in some levels.
    void toggleGroupTriggered(int group, bool activate, gd::vector<int> const& remapKeys, int triggerID, int controlID) {
        if (buf.simulating) {
            // v2.75: a toggle orb that switches a group off: the ghost stops colliding with
            // that group for the rest of this prediction (the real objects aren't touched)
            // v2.85: and one that switches a group on, the ghost starts colliding with it
            if (auto* objects = this->getGroup(group)) {
                simToggle(objects, activate);
            }
            return;
        }
        GJBaseGameLayer::toggleGroupTriggered(group, activate, remapKeys, triggerID, controlID);
    }

    void spawnGroup(int group, bool ordered, double delay, gd::vector<int> const& remapKeys, int triggerID, int controlID) {
        if (buf.simulating) {
            return;
        }
        // v2.85: a delayed spawn: what it sets off is predicted from when it will happen
        if (buf.predictTriggers && delay > 0.0 && !ordered && remapKeys.empty() && group > 0 && pendingSpawns.size() < 64) {
            float tickLength = buf.tickLength > 0.001f && buf.tickLength < 2.f ? buf.tickLength : TICK;
            pendingSpawns.push_back({ group, tickCount + static_cast<int>(std::lround(delay * 60.0 / tickLength)) });
        }
        GJBaseGameLayer::spawnGroup(group, ordered, delay, remapKeys, triggerID, controlID);
    }

    void toggleGroup(int id, bool activate) {
        if (buf.simulating) {
            return;
        }
        GJBaseGameLayer::toggleGroup(id, activate);
    }

    void pickupItem(EffectGameObject* object) {
        if (buf.simulating) {
            return;
        }
        GJBaseGameLayer::pickupItem(object);
    }

    void updateCounters(int itemId, int value) {
        if (buf.simulating) {
            return;
        }
        GJBaseGameLayer::updateCounters(itemId, value);
    }

    void activateItemEditTrigger(ItemTriggerGameObject* object) {
        if (buf.simulating) {
            return;
        }
        GJBaseGameLayer::activateItemEditTrigger(object);
    }

    void activateItemCompareTrigger(ItemTriggerGameObject* object, gd::vector<int> const& remapKeys) {
        if (buf.simulating) {
            return;
        }
        GJBaseGameLayer::activateItemCompareTrigger(object, remapKeys);
    }

    // Safety net in case a mode portal slips through: end prediction, change nothing.
    void playerWillSwitchMode(PlayerObject* player, GameObject* object) {
        if (buf.simulating) {
            buf.ghostHitPortal = true;
            return;
        }
        GJBaseGameLayer::playerWillSwitchMode(player, object);
        // v3.0 test 50: the corridor the game really made, next to the ghost's guess (export)
        if (autoplay.active && object && buf.portalLogs < 12 && boundedMode(object->m_objectType)) {
            ++buf.portalLogs;
            float lo = 0.f, hi = 0.f;
            corridorFor(object->getPositionY(), corridorHeight(this, object->m_objectType), lo, hi);
            say("portal check: {} portal at y {:.0f}: game corridor {:.0f}-{:.0f} (free {}), ghost corridor {:.0f}-{:.0f}",
                static_cast<int>(object->m_objectType), object->getPositionY(), getMinPortalY(), getMaxPortalY(), portalIsFree(this, object), lo,
                hi);
        }
    }
};
