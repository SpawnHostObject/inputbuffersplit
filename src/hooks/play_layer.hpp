//@ hooks/play_layer.hpp
//@ Part of main.cpp (single translation unit). Not a standalone file: do not add it to the build.
class $modify(BufPlayLayer, PlayLayer) {
    static void onModify(auto& self) {
        (void)self.setHookPriority("PlayLayer::destroyPlayer", HOOK_PRIORITY);
        (void)self.setHookPriority("PlayLayer::playEndAnimationToPos", HOOK_PRIORITY);
    }

    void destroyPlayer(PlayerObject* player, GameObject* object) {
        // (v3.0 test 9: the game "kills" you with its hidden anticheat spike in the first ticks of
        // every attempt; that isn't a death, and it made autoplay learn nothing)
        // test 76: player 2 dying in a dual part is a death too (tests 27-75 didn't see it: no log, and a
        // macro that died there was never planned around)
        bool p2Dual = player == m_player2 && m_gameState.m_isDualMode && m_player1 && !m_player1->m_isDead;
        if (autoplay.active && !buf.simulating && (player == m_player1 || p2Dual) && !player->m_isDead && object != m_anticheatSpike &&
            !autoplay.diedThisAttempt) {
            int at = autoplay.attemptTick;
            bool replaying = !m_isPracticeMode && autoplay.knownUntil > 0 && at <= autoplay.knownUntil;
            bool expected = autoplay.planSafeUntil > at + 2;
            say("autoplay died at {:.1f}% as {} (tick {}): {}", getCurrentPercent(), MODE_NAMES[modeIndex(m_player1, m_gameState.m_isDualMode ? m_player2 : nullptr)], at,
                replaying ? "while replaying what it learned (the replay went differently)"
                          : expected ? fmt::format("the ghost expected to survive {} more ticks (prediction was wrong here)",
                                           autoplay.planSafeUntil - at)
                                     : "no safe route was found in time (search)");
            // v3.0 test 18: what killed you, next to what the ghost thought would
            say("  you hit object {} (type {}) at x {:.0f} y {:.0f} when at x {:.0f} y {:.0f} | {}", object ? object->m_objectID : 0,
                object ? static_cast<int>(object->m_objectType) : -1, object ? object->getPositionX() : 0.f, object ? object->getPositionY() : 0.f,
                player->getPositionX(), player->getPositionY(), autoplay.killer.empty() ? "no ghost report" : autoplay.killer);
            if (p2Dual) {
                say("  (that was player 2, in a dual part)");
            }
            // alpha.24: what the object is: moved from where the level puts it, which groups, high
            // detail, hidden (Heliopolis: two bots passed this exact spot; here the spike kills)
            if (object) {
                std::string groups;
                if (object->m_groups) {
                    for (int g = 0; g < std::min<int>(object->m_groupCount, 10); ++g) {
                        if ((*object->m_groups)[g] > 0) {
                            groups += fmt::format("{}{}", groups.empty() ? "" : ",", (*object->m_groups)[g]);
                        }
                    }
                }
                say("  the object: placed at x {:.0f} y {:.0f} (now {:.0f},{:.0f}), groups [{}], high detail {}, hidden {}, group off {}, visible {}, opacity {}",
                    object->m_startPosition.x, object->m_startPosition.y, object->getPositionX(), object->getPositionY(), groups,
                    object->m_isHighDetail, object->m_isHide, object->m_isGroupDisabled, object->isVisible(), static_cast<int>(object->getOpacity()));
            }
            if (route.active) {
                say("  route: planned {} ticks past you{}; {} tests so far, {:.2f} ms each", route.end - tickCount,
                    route.stuck ? " (stuck)"
                    : route.sprinting ? fmt::format(" (looking for a way past tick {}: at {}, down to {}{})", route.death - route.origin,
                                            route.cand - route.origin, route.lower - route.origin, route.fine ? ", tick by tick" : "")
                                      : "",
                    routeSimCount, routeSimCount ? routeSimMs / routeSimCount : 0.0);
                say("  off-screen objects the route's ghost was given: {} ({} of them after the check)", buf.offscreenAdded,
                    buf.offscreenFallback);
            }
            // v3.0 test 26: the ghost didn't see what killed you: where you died is avoided from now on
            if (expected && !replaying) {
                auto r = player->getObjectRect();
                float w = r.size.width * 0.6f, h = r.size.height * 0.6f;
                // (test 59 also marked what killed you and grew a spot you died at again: in
                // Aquamarine's 48% wave two grown marks closed the gap between them, and every route
                // there was "stuck". Test 60: back to the spot where you died only)
                buf.autoDangers.push_back(CCRect(r.getMidX() - w / 2.f, r.getMidY() - h / 2.f, w, h));
                say("  the ghost didn't see this: the spot is marked, autoplay keeps out of it ({} marked)", buf.autoDangers.size());
            }
            autoplay.diedThisAttempt = true;
        }
        if (buf.simulating && isGhost(player)) {
            buf.ghostDied = true;
            // v2.30 diagnostics: remember the first thing that kills the press-now ghost
            if (buf.captureKiller && !buf.killerSeen) {
                buf.killerSeen = true;
                buf.killerTick = buf.simTick;
                buf.ghostDeathRect = player->getObjectRect();
                if (object) {
                    buf.killerId = object->m_objectID;
                    buf.killerType = static_cast<int>(object->m_objectType);
                    buf.killerNoTouch = object->m_isNoTouch;
                    buf.killerDisabled = object->m_isDisabled;
                    buf.killerRect = object->getObjectRect();
                }
                else {
                    buf.killerId = 0;
                    buf.killerType = -1;
                }
            }
            return;
        }
        // v2.34: let the game (and noclip / other mods) decide first, and only reset if you
        // really died. With noclip this is called every tick you're inside a spike; resetting
        // before it marked the button released each tick, the next tick pressed it again to
        // match you, and a held button turned into 100-200 clicks a second.
        bool wasDead = player && player->m_isDead;
        PlayLayer::destroyPlayer(player, object);
        if (!player || wasDead || !player->m_isDead) {
            return;
        }
        // v2.84: if the game still has the button down (a delayed release that hadn't fired
        // yet, or an auto press), let go now. Dropping the queue alone left the game holding,
        // so the next attempt started with the icon holding by itself until you clicked.
        // If you really are still holding, it's pressed again once the attempt starts.
        if (buf.appliedInput && buf.owner == this) {
            sendButton(this, false);
        }
        resetPending();
        buf.appliedInput = false;
        buf.autoReleased = false;
        buf.autoClicked = false;
        buf.rapid = 0;
        forgetPress();
        memoryDeath();
        benchFinish();
        refreshHeatmap();
        endAttempt();
        finishDrift(" (died)");
    }

    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        // v2.99.6: the last level was replaced without being quit (seen with Versus, which
        // opens levels on its own). Its nodes are already freed, and the new level can even
        // be created at the same address, so `buf.owner == this` matched during loading and
        // resetLevel touched the old level's freed nodes (crash reading 0xFFFFFFFFFFFFFFFF).
        // Forget all of it before the game starts loading, without touching any of it.
        if (buf.owner) {
            forgetLeftLevel();
        }
        if (!PlayLayer::init(level, useReplay, dontCreateObjects)) {
            return false;
        }
        dropGhosts();
        readSettings();
        applyStage();
        exportLines.clear();
        logClock = 0;
        loadMemory(level);
        if (statsKey != memoryKey) {
            statsKey = memoryKey;
            sessionStats = {};
        }
        attemptStats = {};
        benchNeedStart = true;
        benchRecording = false;
        benchBest = {};
        benchBestDirty = false;
        benchBestTicks = benchSavedTicks();
        loadProfile();
        buf.ghost1 = makeGhost(this);
        buf.ghost2 = makeGhost(this);
        buf.ghostLayerBase = buf.ghostLayer->getChildrenCount();

        // In-game console (F9): hidden unless the player opens it.
        constexpr float W = PANEL_W;
        constexpr float H = PANEL_H;
        auto win = CCDirector::get()->getWinSize();
        buf.panel = CCNode::create();
        buf.panel->setPosition({ 8.f, win.height - H - 8.f });
        buf.panel->setZOrder(1000);

        // Background: the game's own blue background art, cropped to the panel and
        // darkened from top to bottom so the text stays readable.
        auto* stencil = CCLayerColor::create({ 255, 255, 255, 255 }, W, H);
        auto* clip = CCClippingNode::create(stencil);
        if (auto* art = CCSprite::create("game_bg_01_001.png")) {
            auto size = art->getContentSize();
            art->setScale(std::max(W / size.width, H / size.height));
            art->setPosition({ W / 2.f, H / 2.f });
            art->setColor({ 40, 120, 255 });
            art->setOpacity(150);
            clip->addChild(art);
        }
        auto* shade = CCLayerGradient::create({ 0, 0, 0, 50 }, { 0, 0, 0, 140 }, { 0.f, -1.f });
        shade->setContentSize({ W, H });
        clip->addChild(shade);
        auto* header = CCLayerColor::create({ 0, 0, 0, 80 }, W, 28.f);
        header->setPosition({ 0.f, H - 28.f });
        clip->addChild(header);
        auto* rule = CCLayerColor::create({ 255, 255, 255, 45 }, W - 16.f, 1.f);
        rule->setPosition({ 8.f, H - 46.f });
        clip->addChild(rule);
        buf.panel->addChild(clip);

        if (auto* frame = CCScale9Sprite::create("GJ_square07.png")) {
            frame->setContentSize({ W, H });
            frame->setPosition({ W / 2.f, H / 2.f });
            frame->setOpacity(190);
            buf.panel->addChild(frame);
        }

        auto* title = CCLabelBMFont::create("Click Before Frames", "goldFont.fnt");
        title->setScale(0.55f);
        title->setAnchorPoint({ 0.f, 0.5f });
        title->setPosition({ 10.f, H - 14.f });
        buf.panel->addChild(title);

        auto* version = CCLabelBMFont::create(Mod::get()->getVersion().toVString().c_str(), "chatFont.fnt");
        version->setScale(0.45f);
        version->setOpacity(150);
        version->setAnchorPoint({ 1.f, 0.5f });
        version->setPosition({ W - 10.f, H - 14.f });
        buf.panel->addChild(version);

        buf.panelTitle = CCLabelBMFont::create("", "bigFont.fnt");
        buf.panelTitle->setScale(0.4f);
        buf.panelTitle->setAnchorPoint({ 0.f, 0.5f });
        buf.panelTitle->setPosition({ 10.f, H - 37.f });
        buf.panel->addChild(buf.panelTitle);

        buf.panelLines.clear();
        for (size_t i = 0; i < PANEL_LINES; ++i) {
            auto* line = CCLabelBMFont::create("", "chatFont.fnt");
            line->setScale(0.55f);
            line->setAnchorPoint({ 0.f, 0.5f });
            line->setPosition({ 10.f, H - 57.f - i * 13.f });
            buf.panel->addChild(line);
            buf.panelLines.push_back(line);
        }

        // v2.79: the hint shows the keys you set
        auto* mod = Mod::get();
        auto keyName = [mod](char const* key) {
            auto binds = mod->getSettingValue<std::vector<Keybind>>(key);
            return binds.empty() ? std::string("-") : binds.front().toString();
        };
        auto hintText = fmt::format("{} stage  {} menu  {} copy log  {} console  {} path  {} forget", keyName("bind-stage"),
            keyName("bind-menu"), keyName("bind-export"), keyName("bind-console"), keyName("bind-path"), keyName("bind-forget"));
        auto* hint = CCLabelBMFont::create(hintText.c_str(), "chatFont.fnt");
        hint->setScale(0.45f);
        hint->setColor({ 160, 190, 255 });
        hint->setAnchorPoint({ 0.5f, 0.5f });
        hint->setPosition({ W / 2.f, 9.f });
        buf.panel->addChild(hint);

        this->addChild(buf.panel);
        refreshPanel();

        // v2.95: the watermark is back (v2.92 had dropped it), shorter and smaller than before;
        // on phones smaller still, so it doesn't sit on the touch controls
#ifdef GEODE_IS_MOBILE
        constexpr float WATERMARK_SCALE = 0.18f;
#else
        constexpr float WATERMARK_SCALE = 0.22f;
#endif
        buf.watermark = CCLabelBMFont::create("", "bigFont.fnt");
        buf.watermark->setScale(WATERMARK_SCALE);
        buf.watermark->setColor({ 150, 205, 255 });
        buf.watermark->setOpacity(150);
        buf.watermark->setAnchorPoint({ 0.f, 0.f });
        buf.watermark->setPosition({ 4.f, 3.f });
        buf.watermark->setZOrder(1000);
        this->addChild(buf.watermark);
        buf.usedThisAttempt = false;
        refreshWatermark();

        buf.autoTag = CCLabelBMFont::create("AUTO INPUTS ON", "bigFont.fnt");
        buf.autoTag->setScale(0.24f);
        buf.autoTag->setColor({ 255, 120, 120 });
        buf.autoTag->setOpacity(153);  // 60%, like the console; still readable in a recording
        buf.autoTag->setAnchorPoint({ 1.f, 0.f });
        buf.autoTag->setPosition({ win.width - 6.f, 4.f });
        buf.autoTag->setZOrder(1000);
        this->addChild(buf.autoTag);
        refreshAutoTag();

        buf.hintLabel = CCLabelBMFont::create("", "bigFont.fnt");
        buf.hintLabel->setColor({ 255, 170, 215 });
        buf.hintLabel->setOpacity(0);
        buf.hintLabel->setPosition({ win.width / 2.f, win.height - 40.f });
        buf.hintLabel->setZOrder(1000);
        this->addChild(buf.hintLabel);

        buf.heatNode = nullptr;
        if (m_progressBar) {
            buf.heatNode = CCDrawNode::create();
            m_progressBar->addChild(buf.heatNode, 5);
            refreshHeatmap();
        }

        // Path view lives in the object layer, so it scrolls with the level.
        buf.pathNode = CCDrawNode::create();
        // v2.48: above the level's shader effects when it has that layer; inside the shader
        // layer, dark or glow shaders could hide the path completely.
        (m_aboveShaderObjectLayer ? m_aboveShaderObjectLayer : m_objectLayer)->addChild(buf.pathNode, 9999);

        buf.realInput = false;
        buf.appliedInput = false;
        buf.isPlayer1 = true;
        buf.sinceInput = 999;
        buf.rapid = 0;
        forgetPress();
        buf.owner = this;
        return true;
    }

    // v2.99.6: a level that went away without onQuit. Its nodes may be freed, so they're only
    // forgotten (never touched). The ghosts are let go of the same way (like Android): they
    // still point at the old level, so deleting them isn't safe.
    static void forgetLeftLevel() {
        benchFinish();
        benchRecording = false;
        saveMemory();
        restoreGlobals();
        memoryKey.clear();
        buf.hintLabel = nullptr;
        buf.heatNode = nullptr;
        buf.ghost1 = nullptr;
        buf.ghost2 = nullptr;
        buf.ghostLayer = nullptr;
        dropGhosts();  // with the ghosts gone this only clears pointers and state
        forgetAutoplay();
    }

    void onQuit() {
        benchFinish();
        benchFlush();
        benchRecording = false;
        saveMemory();
        restoreGlobals();
        buf.hintLabel = nullptr;
        buf.heatNode = nullptr;
        memoryKey.clear();
        stopAutoplay("");
        dropSnaps(true);
        dropGhosts();
        PlayLayer::onQuit();
    }

    void levelComplete() {
        bool wasGenerating = autoplay.active && autoplay.layer == this && autoplay.generating;
        autoplayFinished(this);
        if (wasGenerating) {
            return;  // the hidden run reached the end: no end screen, the shown run starts next frame
        }
        PlayLayer::levelComplete();
    }

    // v3.0 test 25: while the path is made a death restarts at once (the game's own restart a
    // second later would cut into the next attempt)
    void delayedResetLevel() {
        if (autoplay.active && autoplay.layer == this) {
            double since = std::chrono::duration<double>(std::chrono::steady_clock::now() - autoplay.genEnded).count();
            bool justEnded = autoplay.genEnded.time_since_epoch().count() != 0 && since < 1.5 && !(m_player1 && m_player1->m_isDead);
            if (autoplay.generating || justEnded) {
                return;
            }
        }
        PlayLayer::delayedResetLevel();
    }

    void resetLevel() {
        // v3.0 test 9: an attempt that ended in a real death (the game restarts with you dead)
        // test 81: or player 2 died in a dual part (player 1 isn't dead then)
        bool p2Died = m_gameState.m_isDualMode && m_player2 && m_player2->m_isDead;
        if (autoplay.active && autoplay.layer == this && ((m_player1 && m_player1->m_isDead) || p2Died || autoplay.diedThisAttempt)) {
            autoplayDied(this);
        }
        triggerChecks.clear();  // v2.84
        attemptStartTick = tickCount;  // v2.84.3
        benchFinish();
        benchNeedStart = true;
        endAttempt();
        buf.usedThisAttempt = false;
        // v2.84: same on a restart (R, checkpoints, practice): never start an attempt with the
        // game holding a button you already let go of
        // v2.86: the game also restarts the attempt while the level is still loading; sending
        // a button then (v2.84/v2.85) crashed when you left a level and opened one again
        if (buf.owner != this) {
            PlayLayer::resetLevel();
            return;
        }
        if (buf.appliedInput && !buf.realInput && !(autoplay.active && m_isPracticeMode)) {
            sendButton(this, false);
        }
        resetPending();
        forgetPress();
        refreshAutoTag();
        if (buf.pathNode) {
            buf.pathNode->clear();
        }
        buf.autoReleased = false;
        buf.autoClicked = false;
        drift.active = false;
        PlayLayer::resetLevel();
        // v2.85: start every attempt from the game's own button state. A release you made while
        // your icon was dead never reached the mod, so it still thought you were holding and
        // pressed for you as the attempt started (the cube jumped by itself until you clicked).
        // Now the mod only presses when you press.
        syncHeld(this);
        if (autoplay.active) {
            autoplayAttemptStart(this);
        }
    }

    // v2.99.4: see addPauseButton (iOS never ran the PauseLayer::customSetup hook)
    void pauseGame(bool unfocused) {
        PlayLayer::pauseGame(unfocused);
        if (auto* scene = CCDirector::get()->getRunningScene()) {
            for (auto* child : CCArrayExt<CCNode*>(scene->getChildren())) {
                if (typeinfo_cast<PauseLayer*>(child)) {
                    addPauseButton(child);
                }
            }
        }
    }

    void togglePracticeMode(bool practiceMode) {
        PlayLayer::togglePracticeMode(practiceMode);
        refreshWatermark();
        refreshAutoTag();
    }

    void playEndAnimationToPos(CCPoint position) {
        if (buf.simulating) {
            return;
        }
        PlayLayer::playEndAnimationToPos(position);
    }

    // v2.78.3: the level's own circle effect (orb / pad hits) is never spawned by a ghost
    void spawnCircle() {
        if (buf.simulating) {
            return;
        }
        PlayLayer::spawnCircle();
    }
};
