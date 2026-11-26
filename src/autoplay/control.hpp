//@ autoplay/control.hpp
//@ Part of main.cpp (single translation unit). Not a standalone file: do not add it to the build.
    void stopAutoplay(char const* why) {
        tpsOverride = 0;
        if (cbfOverridden) {
            cbfRestore();
        }
        if (!autoplay.active) {
            return;
        }
        auto* layer = GameManager::get()->m_playLayer;
        if (layer && layer == autoplay.layer) {
            endGenerating("stopped");
            if (autoplay.tag) {
                autoplay.tag->removeFromParent();
            }
            if (autoplay.trailNode) {
                autoplay.trailNode->removeFromParent();
            }
            if (buf.appliedInput) {
                // autoplay pressed on the players directly, so it lets go the same way
                buf.appliedInput = false;
                layer->m_player1->releaseButton(PlayerButton::Jump);
                if (layer->m_player2) {
                    layer->m_player2->releaseButton(PlayerButton::Jump);
                }
            }
        }
        say("autoplay off{} ({} inputs)", why, autoplay.inputs);
        routeReset();
        buf.objIndex.clear();
        autoplay = {};
        buf.autoDangers.clear();
        onlineMacroTried.clear();  // test 86: looked up again next start (after changing TPS, say)
    }

    // a level that was left without quitting: its label went with it, so it's only forgotten
    void forgetAutoplay() {
        routeReset();
        dropSnaps(false);  // the level they belonged to is gone
        buf.objIndex.clear();
        autoplay = {};
        buf.autoDangers.clear();
    }

    bool startAutoplay(std::string* why) {
        auto* layer = GameManager::get()->m_playLayer;
        if (!layer) {
            *why = "Open a level first.";
            return false;
        }
        if (layer->m_isPlatformer) {
            *why = "Autoplay doesn't do platformer levels.";
            return false;
        }
        if (!account::isLinked()) {
            *why = "Autoplay needs this game linked to your Discord on the Click Before Frames site: open the menu, go to Account and press Link with Discord.";
            return false;
        }
        // test 85: 2-player levels are allowed: a macro plays both players (test 84); without one,
        // autoplay plays player 1 only
        if (layer->m_level && layer->m_level->m_twoPlayerMode) {
            say("autoplay: a 2-player level: a macro plays both players; without one, autoplay plays player 1 only");
        }
        if (!buf.ghost1 || buf.owner != layer) {
            *why = "The level isn't ready yet, try again in a moment.";
            return false;
        }
        // the game itself saves nothing (progress, completion, stars) in test mode
        layer->m_isTestMode = true;
        autoplay = {};
        buf.autoDangers.clear();
        autoplay.active = true;
        autoplay.layer = layer;
        // started mid-run: this attempt's ticks don't count from the level start, so it isn't learned
        autoplay.userTouched = true;
        resetPending();
        auto win = CCDirector::get()->getWinSize();
        autoplay.takeOver = Mod::get()->getSavedValue<bool>("autoplay-takeover", false);
        // the label stays the whole time autoplay is on (also while you play with Take Over),
        // so a recording always shows the run wasn't played by hand
        autoplay.tag = CCLabelBMFont::create(autoplay.takeOver ? "AUTOPLAY - TAKE OVER" : "AUTOPLAY", "bigFont.fnt");
        // v3.0.1: the label always shows (viewers have to be able to tell it isn't a real run), but
        // it can be small, in the bottom corner, for showcases
        if (Mod::get()->getSettingValue<std::string>("autoplay-label") == "Small") {
            autoplay.tag->setScale(0.32f);
            autoplay.tag->setColor({ 255, 255, 255 });
            autoplay.tag->setOpacity(150);
            autoplay.tag->setAnchorPoint({ 1.f, 0.f });
            autoplay.tag->setPosition({ win.width - 6.f, 4.f });
        }
        else {
            autoplay.tag->setScale(0.45f);
            autoplay.tag->setColor({ 255, 215, 90 });
            autoplay.tag->setOpacity(220);
            autoplay.tag->setPosition({ win.width / 2.f, win.height - 26.f });
        }
        autoplay.tag->setID("autoplay-label"_spr);
        layer->addChild(autoplay.tag, 1000);
        say("autoplay on: the level is in test mode, nothing is saved as progress");
        autoplay.trailNode = CCDrawNode::create();
        autoplay.trailNode->setID("autoplay-trail"_spr);
        autoplay.trailNode->setVisible(Mod::get()->getSettingValue<bool>("autoplay-trail"));  // v3.0.1: hidden for showcases
        (layer->m_aboveShaderObjectLayer ? layer->m_aboveShaderObjectLayer : layer->m_objectLayer)->addChild(autoplay.trailNode, 9998);
        loadAutoplay(layer);
        drawTrail();
        // v3.0 test 27: no "making the path" screen any more: the route is planned while it plays
        makeSnaps(layer);
        buildObjIndex(layer);
        routeReset();
        routeCountersReset();
        say("autoplay: the route is planned up to 10 s ahead while it plays ({} level objects indexed)", [] {
            size_t n = 0;
            for (auto const& [k, v] : buf.objIndex) {
                n += v.size();
            }
            return n;
        }());
        return true;
    }
