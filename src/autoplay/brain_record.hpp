//@ autoplay/brain_record.hpp
//@ Part of main.cpp (single translation unit). Not a standalone file: do not add it to the build.
    // ===== test 90: the global brain =====
    // A run autoplay finishes from the level's start (normal mode, nothing pressed by you) is sent to
    // the CBF server: the level id, a fingerprint of the level's exact contents, the TPS, the mod
    // version, the level name and its button changes by the game's step. Nothing about you. Another
    // game starting autoplay there gets the run finished the most times and plays it like a macro.
    // The same run finished again counts as a confirmation. Setting: Global Brain.
    int progressPerStepNow();
    void brainRecordPos(PlayLayer* layer, int step);
    void brainRecord(PlayLayer* layer, bool down) {
        int per = progressPerStepNow();
        int step = static_cast<int>(layer->m_gameState.m_currentProgress) / std::max(1, per);
        if (!autoplay.runFlips.empty() && autoplay.runFlips.back().second == down) {
            return;
        }
        autoplay.runFlips.push_back({ step, down });
        brainRecordPos(layer, step);
    }

    // v3.1.1: where you are at this step, for the run's position records (bots keep these so a run
    // stays on its path in another game; brain runs had only clicks and drifted off: Thinking Space 12%)
    void brainRecordPos(PlayLayer* layer, int step) {
        auto* p1 = layer->m_player1;
        if (!p1 || autoplay.runFixes.size() >= 40000) {
            return;
        }
        if (!autoplay.runFixes.empty() && autoplay.runFixes.back().frame == step) {
            return;
        }
        MacroFix f;
        f.frame = step;
        f.x = p1->getPositionX();
        f.y = p1->getPositionY();
        f.r = p1->getRotation();
        auto* p2 = layer->m_player2;
        if (layer->m_gameState.m_isDualMode && p2 && !p2->m_isDead) {
            f.p2 = true;
            f.x2 = p2->getPositionX();
            f.y2 = p2->getPositionY();
            f.r2 = p2->getRotation();
        }
        autoplay.runFixes.push_back(f);
    }

    std::string levelFingerprint(GJGameLevel* level) {
        // FNV-1a 64 of the level's contents and version (the same on every platform)
        uint64_t h = 1469598103934665603ull;
        auto mix = [&](std::string const& t) {
            for (unsigned char c : t) {
                h ^= c;
                h *= 1099511628211ull;
            }
        };
        mix(std::string(level->m_levelString));
        mix(fmt::format("|{}", static_cast<int>(level->m_levelVersion)));
        return fmt::format("{:016x}", h);
    }

    // v3.1.1: tells the brain how its run went here: to the end, or it kept dying (it ranks runs by it)
    void brainResult(bool ok) {
        if (autoplay.brainKey.empty() || autoplay.brainReported || std::string_view(online::URL).empty()) {
            return;
        }
        autoplay.brainReported = true;
        std::string body = fmt::format(R"({{"id":"{}","key":"{}","ok":{}}})", online::id(), autoplay.brainKey, ok ? "true" : "false");
        std::thread([body] {
            (void)web::WebRequest().timeout(std::chrono::seconds(15)).userAgent("ClickBeforeFrames").header("Content-Type", "application/json")
                .bodyString(body).postSync(std::string(online::URL) + "/api/runs/result");
        }).detach();
    }

    void brainUpload(PlayLayer* layer) {
        if (std::string_view(online::URL).empty() || !Mod::get()->getSettingValue<bool>("global-brain") || !layer->m_level ||
            layer->m_level->m_levelID.value() <= 0 || layer->m_level->m_twoPlayerMode || autoplay.runFlips.size() < 1 ||
            autoplay.macroIsBrain) {
            return;
        }
        // v3.1.1: a run is shared with where you were along it (every 3 steps and at each click), so a
        // run a macro's position records carried is shared too: the brain keeps the next game on the
        // same path. Without them (shouldn't happen) clicks alone don't finish, so it isn't shared
        if (autoplay.runFixes.size() < 10) {
            say("autoplay: the run isn't shared with the global brain: no positions were recorded along it");
            return;
        }
        std::string inputs;
        for (auto const& [t, d] : autoplay.runFlips) {
            inputs += fmt::format("{}[{},{}]", inputs.empty() ? "" : ",", t, d ? 1 : 0);
        }
        auto name = matjson::Value(std::string(layer->m_level->m_levelName)).dump();
        std::string fixes;
        fixes.reserve(autoplay.runFixes.size() * 32);
        for (auto const& f : autoplay.runFixes) {
            fixes += fixes.empty() ? "" : ",";
            fixes += f.p2 ? fmt::format("[{},{:.2f},{:.2f},{:.1f},{:.2f},{:.1f}]", f.frame, f.x, f.y, f.r, f.y2, f.r2)
                          : fmt::format("[{},{:.2f},{:.2f},{:.1f}]", f.frame, f.x, f.y, f.r);
        }
        std::string body = fmt::format(R"({{"id":"{}","level":{},"hash":"{}","tps":{},"name":{},"version":"{}","inputs":[{}],"fixes":[{}]}})", online::id(), layer->m_level->m_levelID.value(),
            levelFingerprint(layer->m_level), static_cast<int>(settingTps()), name, Mod::get()->getVersion().toVString(), inputs, fixes);
        size_t n = autoplay.runFlips.size();
        size_t pos = autoplay.runFixes.size();
        std::thread([body, n, pos] {
            auto res = web::WebRequest().timeout(std::chrono::seconds(20)).userAgent("ClickBeforeFrames").header("Content-Type", "application/json")
                .bodyString(body).postSync(std::string(online::URL) + "/api/runs");
            bool confirmed = res.ok() && res.string().unwrapOr("").find("\"confirmed\":true") != std::string::npos;
            queueInMainThread([ok = res.ok(), confirmed, n, pos] {
                say("autoplay: {}", !ok ? std::string("couldn't share the run with the global brain") :
                    confirmed ? fmt::format("the run ({} inputs) was already in the global brain: it counts as one more finish", n)
                              : fmt::format("the run ({} inputs, {} positions) is shared with the global brain: every game's autoplay can play it now", n, pos));
            });
        }).detach();
    }
