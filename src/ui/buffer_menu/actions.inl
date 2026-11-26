//@ ui/buffer_menu/actions.inl
//@ Part of main.cpp (single translation unit). Not a standalone file: do not add it to the build.
    // ---------- actions ----------

    void onCloseButton(CCObject*) {
        keyBackClicked();
    }

    void onTab(CCObject* sender) {
        s_tab = sender->getTag();
        s_back = s_tab;
        rebuildPage();
    }

    void onChip(CCObject*) {
        s_tab = s_back = 1;
        rebuildPage();
    }

    void onDone(CCObject*) {
        s_tab = s_back;
        rebuildPage();
    }

    void onTuning(CCObject*) {
        s_back = s_tab < TAB_TUNING ? s_tab : s_back;
        s_tab = TAB_TUNING;
        rebuildPage();
    }

    void onSettings(CCObject*) {
        if (s_tab < TAB_TUNING) {
            s_back = s_tab;
        }
        s_tab = TAB_SETTINGS;
        rebuildPage();
    }

    void onLight(CCObject*) {
        Mod::get()->setSavedValue("ui-light", !Mod::get()->getSavedValue<bool>("ui-light", false));
        wipe();
    }

    void onAccent(CCObject* sender) {
        int i = sender->getTag();
        if (i == 6) {
            // the + dot: a colour of your own, each tap turns the hue a bit further
            int hue = Mod::get()->getSavedValue<int>("ui-accent-hue", 262);
            if (Mod::get()->getSavedValue<int>("ui-accent", 0) == 6) {
                hue = (hue + 24) % 360;
            }
            Mod::get()->setSavedValue("ui-accent-hue", hue);
        }
        Mod::get()->setSavedValue("ui-accent", i);
        wipe();
    }

    void onBack(CCObject* sender) {
        Mod::get()->setSavedValue("ui-bg", sender->getTag());
        wipe();
    }

    void onToggle(CCObject* sender) {
        auto const& o = *m_options[sender->getTag()];
        bool value = !optionValue(o);
        if (o.key) {
            Mod::get()->setSettingValue<bool>(o.key, value);
            readSettings();
            saveProfile();
            refreshHeatmap();
            refreshWatermark();
            refreshAutoTag();
        }
        else {
            buf.showPath = value;
            if (buf.pathNode) {
                buf.pathNode->clear();
            }
        }
        say("{} {}", o.label, value ? "on" : "off");
        refresh();
    }

    void onPauseButton(CCObject*) {
        bool on = !Mod::get()->getSavedValue<bool>("pause-button", true);
        Mod::get()->setSavedValue("pause-button", on);
        say("pause menu button {}", on ? "on" : "off (open the menu with its key)");
        rebuildPage();
    }

    void onStageButton(CCObject* sender) {
        int i = sender->getTag();
        if (i == static_cast<int>(std::size(STAGE_BTN))) {
            buf.perMode = true;
            saveModeTicks();
            refreshWatermark();
            refreshPanel();
            say("stage -> Custom (per-mode frames)");
        }
        else {
            if (buf.perMode) {
                buf.perMode = false;
                saveModeTicks();
            }
            buf.stage = STAGE_BTN[i];
            cycleStage(0);
        }
        rebuildPage();
    }

    void onModeSize(CCObject* sender) {
        int bit = sender->getTag();
        buf.modeOff ^= 1u << bit;
        saveModeTicks();
        resetPending();
        say("{} {}: {}", MODE_NAMES[bit / 2], bit % 2 ? "mini" : "normal", buf.modeOff & (1u << bit) ? "raw clicks" : "buffered");
        rebuildPage();
    }

    void onAllModes(CCObject* sender) {
        buf.modeOff = sender->getTag() ? 0u : 0xffffu;
        saveModeTicks();
        resetPending();
        say("all modes {}", sender->getTag() ? "buffered" : "off (raw clicks)");
        rebuildPage();
    }

    void onModeStep(CCObject* sender) {
        // tags: mode * 10 - 1 for "-", mode * 10 + 1 for "+"
        int tag = sender->getTag();
        int mode = (tag + 5) / 10;
        int dir = tag - mode * 10;
        if (mode < 0 || mode > 7 || (dir != 1 && dir != -1)) {
            return;
        }
        buf.modeTicks[mode] = std::clamp(buf.modeTicks[mode] + dir * 2, 0, MODE_TICKS_MAX);  // half a frame
        saveModeTicks();
        say("{}: {:g} frames", MODE_NAMES[mode], buf.modeTicks[mode] * TICK);
        refreshModes();
    }

    void onTps(CCObject* sender) {
        int tps = static_cast<int>(Mod::get()->getSettingValue<int64_t>("tps"));
        tps = std::clamp(tps + (sender->getTag() > 0 ? 20 : -20), TPS_MIN, TPS_MAX);
        Mod::get()->setSettingValue<int64_t>("tps", tps);
        refresh();
    }

    void onScale(CCObject* sender) {
        float s = Mod::get()->getSavedValue<float>("ui-scale", 1.f) + (sender->getTag() > 0 ? 0.1f : -0.1f);
        s = std::clamp(std::round(s * 10.f) / 10.f, 0.4f, 1.3f);
        Mod::get()->setSavedValue("ui-scale", s);
        m_mainLayer->setScale(s);
        refresh();
    }

    void onOpacity(CCObject* sender) {
        float o = Mod::get()->getSavedValue<float>("ui-opacity", 1.f) + (sender->getTag() > 0 ? 0.1f : -0.1f);
        o = std::clamp(std::round(o * 10.f) / 10.f, 0.5f, 1.f);
        Mod::get()->setSavedValue("ui-opacity", o);
        buildFrame();
        refresh();
    }

    void onAllSettings(CCObject*) {
        geode::openSettingsPopup(Mod::get());
    }

    void onCopyLog(CCObject*) {
        exportConsole();
    }

    void onConsole(CCObject*) {
        buf.panelOpen = !buf.panelOpen;
        refreshPanel();
        say("console {}", buf.panelOpen ? "shown" : "hidden");
        rebuildPage();
    }

    // test 91: account
    void onLinkAccount(CCObject*) {
        account::startLink();
        rebuildPage();
    }

    void onAccountRefresh(CCObject*) {
        account::status.clear();
        account::refresh();
    }

    void onOpenSite(CCObject*) {
        web::openLinkInBrowser(std::string(online::URL));
    }

    // test 90: check for / install the newest version
    void onUpdates(CCObject*) {
        if (!account::isLinked()) {
            s_tab = s_back = TAB_ACCOUNT;  // test 92: link first
            rebuildPage();
            return;
        }
        manualUpdate();
        rebuildPage();
    }

    // v3.0.2: deletes this level's macro (the downloaded one, or yours named after the level) and its
    // remembered shift; autoplay asks the brain / Hyperbolus again next time it starts
    void onForgetMacro(CCObject*) {
        auto* layer = GameManager::get()->m_playLayer;
        if (!layer || !layer->m_level) {
            return;
        }
        int id = layer->m_level->m_levelID.value();
        std::error_code ec;
        int removed = 0;
        for (auto const& name : { fmt::format("{}.gdr", id), fmt::format("{}.gdr.json", id), fmt::format("{}.json", id) }) {
            if (std::filesystem::remove(macroDir() / name, ec)) {
                ++removed;
            }
        }
        Mod::get()->setSavedValue(fmt::format("macro-src-{}", id), std::string());
        Mod::get()->setSavedValue(fmt::format("macro-shift-{}", id), std::string());
        Mod::get()->setSavedValue(fmt::format("macro-bad2-{}", id), std::string());
        Mod::get()->setSavedValue(fmt::format("macro-best-{}", id), std::string());
        Mod::get()->setSavedValue(fmt::format("macro-final-{}", id), false);
        Mod::get()->setSavedValue(fmt::format("macro-pick-{}", id), std::string());  // alpha.31
        std::filesystem::remove(macroDir() / fmt::format("{}.best.gdr", id), ec);
        std::filesystem::remove(macroDir() / fmt::format("{}.best.gdr.json", id), ec);
        say("autoplay: {} for this level: looking again (brain, then Hyperbolus)",
            removed ? "the macro is forgotten" : "no macro file named after this level (one with another name stays: delete it in the Macros folder)");
        // v3.1.1: right away (alpha.2 kept playing the macro it had loaded)
        if (autoplay.active && autoplay.layer == layer) {
            onlineMacroTried.erase(id);
            autoplay.macroDeaths.clear();
            loadMacro(layer);
        }
    }

    // alpha.31
    void onChooseMacro(CCObject*) {
        if (auto* p = MacroPicker::create()) {
            p->show();
        }
    }

    // test 67: where to put a level's GDR macro
    void onMacros(CCObject*) {
        std::error_code ec;
        std::filesystem::create_directories(macroDir(), ec);
        file::openFolder(macroDir());
    }

    void onTakeOver(CCObject*) {
        bool on = !Mod::get()->getSavedValue<bool>("autoplay-takeover", false);
        Mod::get()->setSavedValue("autoplay-takeover", on);
        if (autoplay.active) {
            autoplay.takeOver = on;
            if (autoplay.tag) {
                autoplay.tag->setString(on ? "AUTOPLAY - TAKE OVER" : "AUTOPLAY");
            }
        }
        say("autoplay take over {}", on ? "on (nothing is saved, the label stays on screen)" : "off");
        rebuildPage();
    }

    void onAutoplay(CCObject*) {
        if (autoplay.active) {
            stopAutoplay("");
            rebuildPage();
            return;
        }
        std::string why;
        if (!startAutoplay(&why)) {
            if (!account::isLinked()) {
                s_tab = s_back = TAB_ACCOUNT;  // test 92: straight to where you link
                rebuildPage();
            }
            FLAlertLayer::create("Autoplay", why, "OK")->show();
            return;
        }
        rebuildPage();
    }

    void onChangelog(CCObject*) {
        web::openLinkInBrowser("https://github.com/raresdalas1009/-predictive-input-buffer/releases");
    }

    void onRunCheck(CCObject*) {
        benchPending = true;
        say("run check: runs at the start of your next attempt from the beginning (restart the level)");
        FLAlertLayer::create("Run check",
            "Restart the level from the beginning: your longest recorded run is replayed on the hidden player and the "
            "result shows in the Log and in Statistics.",
            "OK")->show();
    }

    void onStats(CCObject*) {
        FLAlertLayer::create("Statistics", statsText(), "OK")->show();
    }

    void onForget(CCObject*) {
        memory.clear();
        pending.active = false;
        memoryDirty = true;
        saveMemory();
        refreshHeatmap();
        say("memory for this level cleared");
        rebuildPage();
    }
