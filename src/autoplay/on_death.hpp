//@ autoplay/on_death.hpp
//@ Part of main.cpp (single translation unit). Not a standalone file: do not add it to the build.
    // alpha.11: where in a step an input of a position-played macro goes in: the step whose x is
    // nearest the bot's (0.5 of a step ahead), then slightly later / earlier ones
    constexpr float MACRO_AHEAD[5] = { 0.5f, 0.25f, 0.75f, 0.f, 1.f };
    constexpr float MACRO_NUDGE[4] = { 0.f, 0.5f, 0.25f, 0.75f };

    // test 79; test 83: +1 first (Zodiac, Tidal Wave (iCreate) and Thinking Space II (xdBot) all lined up at +1)
    constexpr int MACRO_OFFSETS[5] = { 1, 0, -1, 2, -2 };
    int macroNow(PlayLayer* layer);
    void autoplayDied(PlayLayer* layer) {
        if (layer->m_isPracticeMode || (autoplay.userTouched && autoplay.macro.empty())) {
            return;
        }
        // test 70: with a macro, the macro stays. Dying twice at the same spot in it: it's played up
        // to 0.5 s before there, and the route plans the rest
        if (!autoplay.macro.empty()) {
            int reached = macroNow(layer);
            if (autoplay.macroFixCount > 0) {
                if (autoplay.macroLdmMismatch) {
                    say("autoplay: (the macro's Low Detail Mode isn't yours: set LDM the way the macro was made, it's likely why it died)");
                }
                say("autoplay: position records used {} times this attempt (most {:.2f} units){}", autoplay.macroFixCount, autoplay.macroFixMax,
                    autoplay.macroFixSkipped ? fmt::format(", {} left out (they'd have put you inside a block or spike)", autoplay.macroFixSkipped) : "");
                autoplay.macroFixCount = 0;
                autoplay.macroFixMax = 0.f;
                autoplay.macroFixSkipped = 0;
            }
            // alpha.9: an attempt the macro joined partway (it finished downloading mid-attempt) says
            // nothing about it (Grief: the swapped try started at x 149, died at once, and lost to the other)
            if (autoplay.macroMidStart) {
                autoplay.macroMidStart = false;
                say("autoplay: the macro started partway into that attempt: it doesn't count, it plays from the start now");
                return;
            }
            // test 79: first find the step the macro lines up on: each shift gets an attempt
            if (!autoplay.macroTuned && (!autoplay.macroByX || autoplay.macroForceX) && reached < autoplay.macroEnd && !autoplay.macroTrials.empty()) {
                int raw = reached - autoplay.macroAutoOffset;
                int n = static_cast<int>(autoplay.macroTrials.size());
                autoplay.macroTrialReach[autoplay.macroTry] = raw;
                say("autoplay: the macro shifted {:+d} step(s){} got to step {}", autoplay.macroAutoOffset, autoplay.macroSwap ? ", players swapped," : "", raw);
                if (autoplay.macroTry + 1 < n) {
                    ++autoplay.macroTry;
                    autoplay.macroAutoOffset = autoplay.macroTrials[autoplay.macroTry].first;
                    macroSetSwap(autoplay.macroTrials[autoplay.macroTry].second);
                    say("autoplay: trying the macro shifted {:+d} step(s){}", autoplay.macroAutoOffset, autoplay.macroSwap ? ", players swapped" : "");
                    return;
                }
                int best = 0;
                for (int i = 1; i < n; ++i) {
                    if (autoplay.macroTrialReach[i] > autoplay.macroTrialReach[best]) {
                        best = i;
                    }
                }
                autoplay.macroTuned = true;
                autoplay.macroAutoOffset = autoplay.macroTrials[best].first;
                macroSetSwap(autoplay.macroTrials[best].second);
                say("autoplay: the macro goes furthest shifted {:+d} step(s){} (to step {}): it's played that way", autoplay.macroAutoOffset,
                    autoplay.macroSwap ? ", players swapped" : "", autoplay.macroTrialReach[best]);
                // test 81: remembered for this macro, so the next session doesn't try them all again
                if (layer->m_level) {
                    Mod::get()->setSavedValue(fmt::format("macro-shift-{}", layer->m_level->m_levelID.value()),
                        fmt::format("{}{}{}|{}", autoplay.macroAutoOffset, autoplay.macroSwap ? "s" : "", autoplay.macroForceX ? "" : "t", autoplay.fromMacro));
                }
                autoplay.macroDeaths.clear();
                autoplay.macroDeaths.push_back(autoplay.macroTrialReach[best] + autoplay.macroAutoOffset);
                return;
            }
            if (reached < autoplay.macroUntil + st(60)) {
                autoplay.macroDeaths.push_back(reached);
                int here = 0;
                for (int d : autoplay.macroDeaths) {
                    if (std::abs(d - reached) < st(30)) {
                        ++here;
                    }
                }
                // v3.1.1: the brain's run dying twice at one spot here: it's reported (runs that fail go
                // last) and the brain's next best run is played
                if (autoplay.macroIsBrain && here >= 2 && !autoplay.brainKey.empty() && layer->m_level) {
                    say("autoplay: the global brain's run died twice at step {} here: it's reported, asking the brain for its next best run", reached);
                    brainResult(false);
                    autoplay.brainSkip.push_back(autoplay.brainKey);
                    {
                        std::string list;
                        for (auto const& k : autoplay.brainSkip) {
                            list += (list.empty() ? "" : ",") + k;
                        }
                        Mod::get()->setSavedValue(fmt::format("brain-skip-{}", layer->m_level->m_levelID.value()), list);
                    }
                    autoplay.brainKey.clear();
                    autoplay.macro.clear();
                    autoplay.fromMacro.clear();
                    autoplay.macroIsBrain = false;
                    autoplay.macroDeaths.clear();
                    brainFetch(layer->m_level->m_levelID.value(), levelFingerprint(layer->m_level));
                    return;
                }
                // test 77: a downloaded macro dying twice in its first 5%: it doesn't work here (an
                // older GD, or a broken one): the next best one is downloaded
                auto* lvl = layer->m_level;
                int lid = lvl ? lvl->m_levelID.value() : 0;
                auto src = lid > 0 ? Mod::get()->getSavedValue<std::string>(fmt::format("macro-src-{}", lid), "") : std::string();
                auto nl = src.find('\n');
                bool fromSite = !autoplay.macroUrl.empty() || (nl != std::string::npos && autoplay.fromMacro == src.substr(nl + 1));
                if (!autoplay.macroUrl.empty()) {
                    src = autoplay.macroUrl + "\n" + autoplay.fromMacro;  // v3.1.1: known even after its file is gone
                    nl = src.find('\n');
                }
                // (one downloaded by tests 69-76 has no record of where it's from: it's downloaded again)
                bool oldDownload = src.empty() && lid > 0 && autoplay.fromMacro.rfind(std::to_string(lid) + ".", 0) == 0;
                if (here >= 2 && reached < autoplay.macroEnd / 20 && (fromSite || oldDownload) && !autoplay.macroForceX) {
                    if (fromSite) {
                        auto key = fmt::format("macro-bad2-{}", lid);
                        auto badList = Mod::get()->getSavedValue<std::string>(key, "");
                        Mod::get()->setSavedValue(key, badList.empty() ? src.substr(0, nl) : badList + "|" + src.substr(0, nl));
                    }
                    Mod::get()->setSavedValue(fmt::format("macro-src-{}", lid), std::string());
                    std::error_code ec;
                    std::filesystem::remove(macroDir() / fmt::format("{}.gdr", lid), ec);
                    std::filesystem::remove(macroDir() / fmt::format("{}.gdr.json", lid), ec);
                    say("autoplay: the macro {} died twice at step {} ({:.0f}% into it): it's dropped, looking for another one", autoplay.fromMacro, reached,
                        100.0 * reached / std::max(1, autoplay.macroEnd));
                    autoplay.macro.clear();
                    autoplay.fromMacro.clear();
                    onlineMacroTried.erase(lid);
                    fetchOnlineMacro(lid);
                    return;
                }
                // v3.1.1: a downloaded macro still dying at one spot after the guard and autoplay's own planning
                // there (4 deaths): the next Hyperbolus macro is tried; the best one so far is kept aside
                // and comes back if no other gets further
                // alpha.11: a position-played macro dying at one spot again: each press timing gets a try
                // first (Grief: 4 deaths at 36.1%, 0.3 units off the bot's path), then the best is kept
                // alpha.13: no: a timing for the whole level broke the start (0.25: dead at 0.6%). At the spot
                // only, you're moved along the bot's own path by part of a step, so the game checks hits at
                // other points of it (Grief 36.1%: you were on the bot's path, 0.3 units off, and still hit
                // the spike between the points it was checked at)
                if (autoplay.macroForceX && here >= 2 && layer->m_player1) {
                    int dxAt = static_cast<int>(layer->m_player1->getPositionX());
                    int key = dxAt;
                    for (auto const& [k, v] : autoplay.macroNudges) {
                        if (std::abs(k - dxAt) <= 12) {
                            key = k;
                        }
                    }
                    int& n = autoplay.macroNudges[key];
                    if (n < 3) {
                        ++n;
                        autoplay.macroDeaths.pop_back();
                        say("autoplay: the macro died at x {} again: just before it you're moved {:.2f} of a step along the bot's path (try {} of 3)",
                            key, MACRO_NUDGE[n], n);
                        return;
                    }
                }
                if (false) {
                    auto& rs = autoplay.macroAheadReach;
                    rs[autoplay.macroAheadTry] = std::max(rs[autoplay.macroAheadTry], reached);
                    if (autoplay.macroAheadTry < 4) {
                        ++autoplay.macroAheadTry;
                        autoplay.macroDeaths.pop_back();
                        say("autoplay: the macro died at step {} again: its inputs go in {:.2f} of a step ahead now (was {:.2f})", reached,
                            MACRO_AHEAD[autoplay.macroAheadTry], MACRO_AHEAD[autoplay.macroAheadTry - 1]);
                        return;
                    }
                    int best = 0;
                    for (int i = 1; i < 5; ++i) {
                        if (rs[i] > rs[best]) {
                            best = i;
                        }
                    }
                    autoplay.macroAheadTry = 5 + best;  // done: played with the best from now on
                    say("autoplay: every press timing tried: {:.2f} of a step ahead went furthest (to step {})", MACRO_AHEAD[best], rs[best]);
                }
                bool finalPick = lid > 0 && Mod::get()->getSavedValue<bool>(fmt::format("macro-final-{}", lid), false);
                // alpha.28: an xdBot macro played like xdBot plays the same every time: dying twice at one
                // spot, it won't get past it: the next one is tried
                if ((here >= 4 || (autoplay.macroXd && here >= 2)) && fromSite && !finalPick) {
                    macroKeepBest(lid, reached, src);
                    auto key = fmt::format("macro-bad2-{}", lid);
                    auto badList = Mod::get()->getSavedValue<std::string>(key, "");
                    Mod::get()->setSavedValue(key, badList.empty() ? src.substr(0, nl) : badList + "|" + src.substr(0, nl));
                    Mod::get()->setSavedValue(fmt::format("macro-src-{}", lid), std::string());
                    std::error_code ec;
                    std::filesystem::remove(macroDir() / fmt::format("{}.gdr", lid), ec);
                    std::filesystem::remove(macroDir() / fmt::format("{}.gdr.json", lid), ec);
                    say("autoplay: the macro keeps dying at step {} ({:.0f}% into it): trying another Hyperbolus macro (this one is kept in case it's the best)",
                        reached, 100.0 * reached / std::max(1, autoplay.macroEnd));
                    autoplay.macro.clear();
                    autoplay.fromMacro.clear();
                    onlineMacroTried.erase(lid);
                    fetchOnlineMacro(lid);
                    return;
                }
                if (here >= 2 && autoplay.macroTwoPlayer) {
                    say("autoplay: the macro died at step {} again (2-player level: autoplay can't plan player 2, it plays the macro again)", reached);
                }
                else if (false && here >= 2 && autoplay.macroXd && autoplay.xdFrameTry < 3) {
                    // alpha.27: the game's FPS while it plays: the one found first, then 60 and 240 FPS
                    static constexpr int ORDER[3] = { 0, 4, 1 };
                    auto& rs = autoplay.xdFrameReach;
                    rs[autoplay.xdFrameTry] = std::max(rs[autoplay.xdFrameTry], reached);
                    int first = autoplay.xdFrameTry == 0 ? autoplay.xdFrameSteps : -1;
                    (void)first;
                    while (++autoplay.xdFrameTry < 3) {
                        int c = ORDER[autoplay.xdFrameTry];
                        if (c != autoplay.xdFrameSteps) {
                            autoplay.xdFrameSteps = c;
                            autoplay.macroDeaths.pop_back();
                            say("autoplay: the xdBot macro died at step {} again: trying it with the game at {} FPS (moving objects follow the game's frames)", reached,
                                240 / c);
                            return;
                        }
                    }
                    say("autoplay: the xdBot macro died at step {} with every FPS tried: it's played as it is, the next macro is tried after 4 deaths here", reached);
                }
                else if (here >= 2 && (autoplay.macroXd || (autoplay.macroIsBrain && !autoplay.macroFixes.empty()))) {
                    say("autoplay: the xdBot macro died at step {} again: it's played as it is (no planning mixed in), the next macro is tried after 4 deaths here", reached);
                }
                else if (here >= 2) {
                    // test 75 (like Nova): autoplay plans a stretch around it, the macro plays on after it
                    bool grown = false;
                    for (auto& g : autoplay.macroGaps) {
                        if (reached >= g.first - st(60) && reached < g.second + st(60)) {
                            g.first = std::max(0, std::min(g.first, reached - st(180)));
                            g.second = std::max(g.second + st(120), reached + st(240));
                            grown = true;
                            say("autoplay: died in the stretch it plans itself (step {}): it's now steps {}-{}", reached, g.first, g.second);
                        }
                    }
                    if (!grown) {
                        // test 82: from 1.5 s before (0.5 s was too little time to see the death coming)
                        autoplay.macroGaps.push_back({ std::max(0, reached - st(360)), reached + st(240) });
                        say("autoplay: the macro died at step {} again: autoplay plans steps {}-{}, the macro plays the rest", reached,
                            std::max(0, reached - st(360)), reached + st(240));
                    }
                }
                else {
                    say("autoplay: the macro died at step {} ({:.0f}% of it), it plays it again", reached,
                        100.0 * reached / std::max(1, autoplay.macroEnd));
                }
            }
            return;
        }
        int reached = autoplay.attemptTick;
        if (autoplay.knowsWholeLevel && reached < autoplay.knownUntil) {
            // the saved run didn't play the same this time: learn from here like any other death
            autoplay.knowsWholeLevel = false;
        }
        // v3.0 test 24: deaths near this one in this session (test 23 counted only deaths in a row
        // at the same spot, so dying somewhere else once reset it and it went back and forth)
        autoplay.deathTicks.push_back(reached);
        int here = 0;
        for (int d : autoplay.deathTicks) {
            if (std::abs(d - reached) < st(30)) {
                ++here;
            }
        }
        autoplay.sameDeaths = here - 1;
        autoplay.lastDeath = reached;
        // v3.0 test 17: the last flip before the crash isn't made there again, so the next attempt
        // takes a different route instead of repeating the same one
        for (auto it = autoplay.attemptFlips.rbegin(); it != autoplay.attemptFlips.rend(); ++it) {
            if (it->first >= autoplay.knownUntil) {
                autoplay.banned.insert(it->first);
                break;
            }
        }
        // v3.0 test 51: the best attempt so far is what gets replayed (an attempt that dies sooner
        // doesn't replace it), up to 0.5 s before where it ended; each death there frees 1 s more
        // (at most 6). Test 43 kept 3 s for the search, and the search came out differently every
        // time: a route that had worked wasn't followed again and it died sooner.
        if (reached > autoplay.bestReached) {
            autoplay.bestReached = reached;
            autoplay.bestFlips = autoplay.attemptFlips;
            autoplay.bestTrail = autoplay.attemptTrail;
        }
        int atBest = 0;
        for (int d : autoplay.deathTicks) {
            if (std::abs(d - autoplay.bestReached) < st(45)) {
                ++atBest;
            }
        }
        // v3.0 test 59: a second death there doesn't throw 1 s of the best run away any more (it went
        // from replaying up to tick 322 to only up to 82, and died sooner): the first 3 deaths keep
        // it up to 0.5 s before; after that, 0.25 s more each time
        int keepUntil = std::max(0, autoplay.bestReached - st(120) - std::min(std::max(atBest - 3, 0), 16) * st(60));
        autoplay.knownUntil = keepUntil;
        autoplay.known.clear();
        for (auto const& f : autoplay.bestFlips) {
            if (f.first < keepUntil) {
                autoplay.known.push_back(f);
            }
        }
        autoplay.knownTrail.clear();
        for (size_t i = 0; i < autoplay.bestTrail.size() && static_cast<int>(i) * trailEvery() < keepUntil; ++i) {
            autoplay.knownTrail.push_back(autoplay.bestTrail[i]);
        }
        say("autoplay replays its best attempt ({} ticks) up to tick {} ({} inputs)", autoplay.bestReached, keepUntil, autoplay.known.size());
        drawTrail();
        saveAutoplay();
    }
