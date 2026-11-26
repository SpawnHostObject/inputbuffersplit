//@ autoplay/brain_fetch.hpp
//@ Part of main.cpp (single translation unit). Not a standalone file: do not add it to the build.
    // test 90: the run of this level finished the most times by other games' autoplay
    // v3.2: forceTps: the brain's run at another TPS (it's played at that TPS, Macro Plays At Its TPS)
    void brainFetch(int levelId, std::string print, int forceTps) {
        int tps = forceTps > 0 ? forceTps : static_cast<int>(settingTps());
        say("autoplay: asking the global brain for a run of this level...");
        std::string skip;
        for (auto const& k : autoplay.brainSkip) {
            skip += (skip.empty() ? "" : ",") + k;
        }
        bool autoTps = Mod::get()->getSettingValue<bool>("macro-auto-tps");
        std::thread([levelId, print, tps, skip, forceTps, autoTps] {
            auto res = web::WebRequest().timeout(std::chrono::seconds(15)).userAgent("ClickBeforeFrames")
                .getSync(fmt::format("{}/api/runs?level={}&hash={}&tps={}{}", online::URL, levelId, print, tps, skip.empty() ? "" : "&skip=" + skip));
            std::optional<Macro> got;
            int confirms = 0, replays = 0, fails = 0;
            std::string key;
            int tried = 0;
            int otherTps = 0;
            std::string why;
            if (res.ok()) {
                if (auto j = res.json(); j.isOk()) {
                    auto v = j.unwrap();
                    // v3.0.2: why there's no run for you (another TPS, another version of the level)
                    if (v.contains("others") && v["others"].isObject()) {
                        auto o = v["others"];
                        std::string tpsList;
                        if (o.contains("tps") && o["tps"].isArray()) {
                            for (auto const& t : o["tps"]) {
                                int tt = static_cast<int>(t.asInt().unwrapOr(0));
                                if (tt > 0 && tt != tps) {
                                    tpsList += fmt::format("{}{}", tpsList.empty() ? "" : ", ", tt);
                                    if (!otherTps && tt >= 240 && tt <= 1000) {
                                        otherTps = tt;
                                    }
                                }
                            }
                        }
                        int otherVersions = o.contains("versions") ? static_cast<int>(o["versions"].asInt().unwrapOr(0)) : 0;
                        tried = o.contains("tried") ? static_cast<int>(o["tried"].asInt().unwrapOr(0)) : 0;
                        if (!tpsList.empty()) {
                            why = fmt::format(" (it has runs of this level at {} TPS; you play at {})", tpsList, tps);
                        }
                        else if (otherVersions > 0) {
                            why = " (it has runs only for another version of this level: it was updated, or your copy is older)";
                        }
                    }
                    if (v["found"].asBool().unwrapOr(false) && v["inputs"].isArray()) {
                        Macro m;
                        m.file = "global brain run";
                        m.bot = "Click Before Frames";
                        m.fps = tps;
                        m.levelId = levelId;
                        confirms = static_cast<int>(v["confirms"].asInt().unwrapOr(1));
                        replays = static_cast<int>(v["replays"].asInt().unwrapOr(0));
                        fails = static_cast<int>(v["fails"].asInt().unwrapOr(0));
                        key = v["key"].asString().unwrapOr("");
                        for (auto const& in : v["inputs"]) {
                            if (in.isArray() && in.size() == 2) {
                                int f = static_cast<int>(in[0].asInt().unwrapOr(0));
                                m.inputs.push_back({ f, in[1].asInt().unwrapOr(0) != 0 });
                                m.lastFrame = std::max(m.lastFrame, f);
                            }
                        }
                        // v3.1.1: its position records (runs shared from alpha.17 on)
                        if (v.contains("fixes") && v["fixes"].isArray()) {
                            for (auto const& a : v["fixes"]) {
                                if (!a.isArray() || (a.size() != 4 && a.size() != 6)) {
                                    continue;
                                }
                                MacroFix fx;
                                fx.frame = static_cast<int>(a[0].asInt().unwrapOr(-1));
                                fx.x = static_cast<float>(a[1].asDouble().unwrapOr(0.0));
                                fx.y = static_cast<float>(a[2].asDouble().unwrapOr(0.0));
                                fx.r = static_cast<float>(a[3].asDouble().unwrapOr(0.0));
                                if (a.size() == 6) {
                                    fx.p2 = true;
                                    fx.x2 = fx.x;
                                    fx.y2 = static_cast<float>(a[4].asDouble().unwrapOr(0.0));
                                    fx.r2 = static_cast<float>(a[5].asDouble().unwrapOr(0.0));
                                }
                                m.fixes.push_back({ fx.frame, fx.x });
                                m.fixFull.push_back(fx);
                            }
                            std::sort(m.fixes.begin(), m.fixes.end());
                            std::stable_sort(m.fixFull.begin(), m.fixFull.end(), [](MacroFix const& a, MacroFix const& b) { return a.frame < b.frame; });
                        }
                        if (!m.inputs.empty()) {
                            got = std::move(m);
                        }
                    }
                }
            }
            queueInMainThread([got, confirms, replays, fails, key, tried, levelId, why, otherTps, forceTps, autoTps, print] {
                auto* layer = GameManager::get()->m_playLayer;
                bool here = autoplay.active && layer && layer->m_level && layer->m_level->m_levelID.value() == levelId;
                // v3.2: the brain has this level only at another TPS: that run, played at its TPS
                if (!got && tried == 0 && otherTps > 0 && forceTps == 0 && autoTps && here) {
                    say("autoplay: the global brain has this level at {} TPS: autoplay plays it at {} TPS", otherTps, otherTps);
                    brainFetch(levelId, print, otherTps);
                    return;
                }
                if (!got) {
                    if (tried > 0) {
                        say("autoplay: the global brain has no other run of this level ({} failed here)", tried);
                    }
                    else {
                        say("autoplay: the global brain has no run of this level for you yet{}", why);
                    }
                    if (here) {
                        loadLocalMacro(layer);  // v3.0.2: then your macros, then Hyperbolus
                    }
                    return;
                }
                if (here && !autoplay.fromMacro.empty() && !autoplay.macroIsBrain) {
                    say("autoplay: the global brain's run comes first: {} isn't used", autoplay.fromMacro);
                }
                if (!here) {
                    return;
                }
                // v3.1.1: the run others replayed to the end most comes first
                auto credit = replays > 0 ? fmt::format("global brain run (replayed to the end {} time{}{})", replays, replays == 1 ? "" : "s",
                                                        fails ? fmt::format(", failed {}", fails) : "")
                                          : fmt::format("global brain run (not replayed by anyone yet{})", fails ? fmt::format(", failed {}", fails) : "");
                if (applyMacro(*got, credit)) {
                    // recorded by the same counter it's played by: no shift first
                    autoplay.macroIsBrain = true;
                    autoplay.brainKey = key;
                    autoplay.brainReported = false;
                    if (!autoplay.macroTuned) {
                        autoplay.macroTrials = { { 0, false }, { 1, false }, { -1, false } };
                        autoplay.macroTrialReach.assign(3, -1);
                        autoplay.macroAutoOffset = 0;
                    }
                }
            });
        }).detach();
    }
