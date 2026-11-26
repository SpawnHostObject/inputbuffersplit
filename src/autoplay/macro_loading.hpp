//@ autoplay/macro_loading.hpp
//@ Part of main.cpp (single translation unit). Not a standalone file: do not add it to the build.
    void fetchOnlineMacro(int levelId);
    bool applyMacro(Macro const& m, std::string const& credit);

    // the macro for this level: a file named after the level id (76962930.gdr), or any file whose
    // level id inside matches
    void loadLocalMacro(PlayLayer* layer);
    void macroRestoreBest(int lid);
    // v3.0.2: a run in the global brain comes first, over a macro you have (yours or a downloaded one):
    // the brain is asked, and only without a run there does autoplay look in your Macros Folder, then
    // on Hyperbolus
    void loadMacro(PlayLayer* layer) {
        tpsOverride = 0;
        autoplay.fromMacro.clear();
        autoplay.macroIsBrain = false;
        autoplay.macroUrl.clear();
        autoplay.macro.clear();
        autoplay.macroUntil = autoplay.macroEnd = 0;
        autoplay.macroLogged = false;
        if (!layer->m_level) {
            return;
        }
        int id = layer->m_level->m_levelID.value();
        if (autoplay.brainSkipLevel != id) {
            // alpha.20: the brain runs that failed in this game before, kept between sessions
            autoplay.brainSkip.clear();
            autoplay.brainSkipLevel = id;
            auto saved = Mod::get()->getSavedValue<std::string>(fmt::format("brain-skip-{}", id), "");
            for (size_t a = 0; a < saved.size();) {
                size_t b = saved.find(',', a);
                if (b == std::string::npos) {
                    b = saved.size();
                }
                if (b - a == 64) {
                    autoplay.brainSkip.push_back(saved.substr(a, 64));
                }
                a = b + 1;
            }
            if (autoplay.brainSkip.size() > 20) {
                autoplay.brainSkip.erase(autoplay.brainSkip.begin(), autoplay.brainSkip.end() - 20);
            }
        }
        autoplay.brainKey.clear();
        autoplay.brainReported = false;
        // alpha.31: a macro the player picked (not the brain's run): straight to it
        // v3.2: a level the global brain has a run of always plays that run, even over a macro you
        // picked (Choose Macro); your pick is played when the brain has none, or its run fails here
        auto pick = Mod::get()->getSavedValue<std::string>(fmt::format("macro-pick-{}", id), "");
        if (!pick.empty() && pick != "brain" && (std::string_view(online::URL).empty() || !Mod::get()->getSettingValue<bool>("global-brain"))) {
            loadLocalMacro(layer);
            return;
        }
        if (id > 0 && !layer->m_level->m_twoPlayerMode && !std::string_view(online::URL).empty() &&
            Mod::get()->getSettingValue<bool>("global-brain")) {
            brainFetch(id, levelFingerprint(layer->m_level));  // without a run there: loadLocalMacro
            return;
        }
        loadLocalMacro(layer);
    }

    void loadLocalMacro(PlayLayer* layer) {
        if (!layer->m_level) {
            return;
        }
        int id = layer->m_level->m_levelID.value();
        std::error_code ec;
        std::filesystem::create_directories(macroDir(), ec);
        std::optional<Macro> found;
        std::string why;
        for (auto const& e : std::filesystem::directory_iterator(macroDir(), ec)) {
            auto name = e.path().filename().string();
            if (name.find(".best.") != std::string::npos) {
                continue;  // v3.1.1: the best one kept aside while others are tried
            }
            bool named = id > 0 && name.rfind(std::to_string(id) + ".", 0) == 0;
            auto ext = e.path().extension().string();
            if (!e.is_regular_file() || (ext != ".gdr" && ext != ".json")) {
                continue;
            }
            std::string err;
            auto m = readMacro(e.path(), err);
            if (!m) {
                if (named) {
                    why = fmt::format("{}: {}", name, err);
                }
                continue;
            }
            if (named || (id > 0 && m->levelId == id)) {
                found = std::move(m);
                break;
            }
        }
        if (!found) {
            if (!why.empty()) {
                say("autoplay: couldn't use the macro {}", why);
            }
            fetchOnlineMacro(id);  // (the brain was asked first)
            return;
        }
        // test 77: one downloaded from Hyperbolus keeps its credit
        std::string credit;
        auto src = Mod::get()->getSavedValue<std::string>(fmt::format("macro-src-{}", id), "");
        std::string url;
        if (auto nl = src.find('\n'); nl != std::string::npos && found->file.rfind(std::to_string(id) + ".", 0) == 0) {
            credit = src.substr(nl + 1);
            url = src.substr(0, nl);
        }
        if (applyMacro(*found, credit)) {
            autoplay.macroUrl = url;
        }
    }

    void cbfOffForMacro();
    // test 87: your Physics TPS setting (240 = the normal game)
    double settingTps() {
        if (tpsOverride > 0) {
            return tpsOverride;
        }
        return static_cast<double>(std::max<int64_t>(240, Mod::get()->getSettingValue<int64_t>("tps")));
    }

    // v3.1.1: the steps a second the game really runs (the TPS setting doesn't change it on 2.2081:
    // Grief's 360 TPS macro was played at 240 by time and died at once)
    double physicsTps() {
        if (tpsOverride > 0) {
            return tpsOverride;
        }
        return std::round(st(240) / 10.0) * 10.0;
    }

    // test 84: which of the file's two click lists goes to player 1 in a 2-player level
    void macroSetSwap(bool swap) {
        autoplay.macroSwap = swap;
        if (!autoplay.macroTwoPlayer) {
            return;
        }
        autoplay.macro = swap ? autoplay.macroB : autoplay.macroA;
        autoplay.macroP2 = swap ? autoplay.macroA : autoplay.macroB;
        autoplay.macroX = swap ? autoplay.macroXB : autoplay.macroXA;
        autoplay.macroX2 = swap ? autoplay.macroXA : autoplay.macroXB;
        autoplay.macroSyncedFor = -1;
    }
