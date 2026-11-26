//@ autoplay/macro_apply.hpp
//@ Part of main.cpp (single translation unit). Not a standalone file: do not add it to the build.
    bool applyMacro(Macro const& m, std::string const& credit) {
        // test 71: like Nova, only a macro made at your TPS by time; v3.1.1: one made at another TPS with
        // the bot's position records is played by x instead (each input where the bot pressed it)
        // alpha.18: made at another TPS (240 to 1000): autoplay plays at that TPS while it plays the macro
        tpsOverride = 0;
        bool autoTps = Mod::get()->getSettingValue<bool>("macro-auto-tps") && m.fps >= 239.5 && m.fps <= 1000.5;
        double baseTps = static_cast<double>(std::max<int64_t>(240, Mod::get()->getSettingValue<int64_t>("tps")));
        if (autoTps && std::abs(m.fps - baseTps) > 0.5) {
            tpsOverride = static_cast<int>(std::lround(m.fps));
            autoplay.needRestart = true;  // the attempt starts again at that TPS
            say("autoplay: the macro was made at {} TPS: autoplay plays at {} TPS while it plays it (setting: Macro Plays At Its TPS)", tpsOverride, tpsOverride);
        }
        bool otherTps = std::abs(m.fps - physicsTps()) > 0.5;
        if (otherTps && m.fixes.size() < 100) {
            say("autoplay: the macro {} was made at {:.0f} TPS, the game runs {:.0f}, and it has no position records to play it by: it can't be used",
                m.file, m.fps, physicsTps());
            return false;
        }
        auto* plNow = GameManager::get()->m_playLayer;
        double k = otherTps ? 1.0 : physicsTps() / m.fps;
        autoplay.macroForceX = otherTps;
        {
            std::string b = m.bot;
            std::transform(b.begin(), b.end(), b.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            autoplay.macroXd = !otherTps && b.find("xdbot") != std::string::npos && Mod::get()->getSettingValue<bool>("macro-xdbot-mode");
        }
        autoplay.xdFixIdx = 0;
        autoplay.xdLastFrame = -1;
        autoplay.xdFrameSteps = 1;
        autoplay.xdFrameTry = 0;
        autoplay.xdFrameReach.assign(3, -1);
        if (autoplay.macroXd) {
            // the FPS it was made at, from the file name ("60fps", "60 fps")
            std::string n = m.file + " " + credit;
            std::transform(n.begin(), n.end(), n.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            for (size_t at = n.find("fps"); at != std::string::npos; at = n.find("fps", at + 3)) {
                size_t e = at;
                while (e > 0 && n[e - 1] == ' ') {
                    --e;
                }
                size_t b = e;
                while (b > 0 && std::isdigit(static_cast<unsigned char>(n[b - 1]))) {
                    --b;
                }
                if (b < e) {
                    int fps = std::atoi(n.substr(b, e - b).c_str());
                    if (fps >= 30 && fps < 240) {
                        autoplay.xdFrameSteps = std::max(1, static_cast<int>(std::lround(240.0 / fps)));
                        autoplay.xdFrameSteps = 1;  // alpha.28: only noted
                        say("autoplay: the macro says it was made at {} FPS", fps);
                    }
                    break;
                }
            }
        }
        autoplay.macroAheadTry = 0;
        autoplay.macroAheadReach.assign(5, -1);
        autoplay.macroNudges.clear();
        if (otherTps) {
            say("autoplay: the macro was made at {:.0f} TPS and the game runs {:.0f}: it's played by position (where the bot pressed), not by time",
                m.fps, physicsTps());
        }
        // test 70: outside 2-player levels the "player 2" clicks are yours too: the button is down
        // while either is
        auto* pl = GameManager::get()->m_playLayer;
        bool twoPlayer = pl && pl->m_level && pl->m_level->m_twoPlayerMode;
        std::vector<std::tuple<int, bool, bool>> all;  // frame, down, which
        for (auto const& [f, d] : m.inputs) {
            all.push_back({ f, d, false });
        }
        if (!twoPlayer) {
            for (auto const& [f, d] : m.inputs2) {
                all.push_back({ f, d, true });
            }
        }
        std::stable_sort(all.begin(), all.end(), [](auto const& a, auto const& b) { return std::get<0>(a) < std::get<0>(b); });
        // test 80: each press or release counts as it comes, from either side (test 70 held while
        // either side held: a "player 1" release inside a "player 2" hold, which the game lets go on,
        // was skipped: Zodiac's robot at 9.6% held from step 5984 to 6039 instead of 5984-6011, 6031-6039)
        std::vector<std::pair<int, bool>> merged;
        for (auto const& [f, d, which] : all) {
            (void)which;
            merged.push_back({ f, d });
        }
        auto toTicks = [k](std::vector<std::pair<int, bool>> const& in) {
            std::vector<std::pair<int, bool>> out;
            int lastTick = -1;
            bool held = false;
            for (auto const& [frame, down] : in) {
                if (down == held) {
                    continue;  // a repeat of the same state changes nothing
                }
                int t = static_cast<int>(std::llround(frame * k));
                // two changes on one tick (a 1-tick tap): the second goes on the next tick
                t = std::max(t, lastTick + 1);
                out.push_back({ t, down });
                lastTick = t;
                held = down;
            }
            return out;
        };
        std::vector<std::pair<int, bool>> ticks = toTicks(merged);
        autoplay.macroTwoPlayer = twoPlayer;
        autoplay.macroA.clear();
        autoplay.macroB.clear();
        autoplay.macroP2.clear();
        autoplay.macroSwap = false;
        if (twoPlayer) {
            // test 84: player 1's and player 2's clicks each to their own player
            autoplay.macroA = toTicks(m.inputs);
            autoplay.macroB = toTicks(m.inputs2);
            ticks = autoplay.macroA;
            autoplay.macroP2 = autoplay.macroB;
        }
        int until = static_cast<int>(std::llround(m.lastFrame * k)) + st(60);
        if (until <= autoplay.knownUntil) {
            say("autoplay: the macro {} is shorter than what it already learned here, it keeps its own", m.file);
            return false;
        }
        // test 70: the macro is kept apart from what autoplay learns (test 69 put it in its memory,
        // and the first death replaced it with autoplay's own attempt). It's played by the game's step
        // counter, the way bots play it, from the start of an attempt
        autoplay.macro = ticks;
        // test 71: each input's x, from the bot's position records (for playing from a start position)
        auto xsOf = [&](std::vector<std::pair<int, bool>> const& list) {
            std::vector<float> xs;
            if (m.fixes.size() >= 2) {
                for (auto const& [t, d] : list) {
                    double f = t / k;
                    auto it = std::lower_bound(m.fixes.begin(), m.fixes.end(), std::pair<int, float> { static_cast<int>(f), -1e9f });
                    if (it == m.fixes.end()) {
                        it = std::prev(m.fixes.end());
                    }
                    xs.push_back(it->second);
                }
            }
            return xs;
        };
        autoplay.macroX = xsOf(ticks);
        autoplay.macroXA = twoPlayer ? xsOf(autoplay.macroA) : autoplay.macroX;
        autoplay.macroXB = twoPlayer ? xsOf(autoplay.macroB) : std::vector<float> {};
        autoplay.macroX2 = autoplay.macroXB;
        autoplay.fixPtr = 0;
        autoplay.macroByX = autoplay.macroForceX || (plNow && plNow->m_startPosObject && !autoplay.macroX.empty());
        autoplay.macroFixes.clear();
        for (auto f : m.fixFull) {
            f.frame = static_cast<int>(std::llround(f.frame * k));
            autoplay.macroFixes.push_back(f);
        }
        if (!autoplay.macroFixes.empty()) {
            say("autoplay: the macro has {} position records: you're kept on its path (setting: Macro Position Fixes)", autoplay.macroFixes.size());
        }
        autoplay.macroSyncedFor = -1;
        autoplay.macroUntil = until;
        autoplay.macroEnd = until;
        autoplay.macroIdx = 0;
        autoplay.macroArmed = autoplay.attemptTick <= 1;
        // alpha.20: a macro that arrives during an attempt starts it again from the beginning (it ran
        // from partway before: an attempt that didn't count, and a death you watched for nothing)
        if (plNow && autoplay.active && !plNow->m_startPosObject && autoplay.attemptTick > st(60)) {
            autoplay.needRestart = true;
        }
        autoplay.macroDeaths.clear();
        autoplay.macroGaps.clear();
        autoplay.macroInGap = false;
        autoplay.macroTry = 0;
        autoplay.macroTuned = false;
        // test 83: +1 first; test 84: in 2-player levels each shift also with the players swapped
        autoplay.macroTrials.clear();
        for (int sh : { 1, 0, -1, 2, -2 }) {
            autoplay.macroTrials.push_back({ sh, false });
            if (twoPlayer) {
                autoplay.macroTrials.push_back({ sh, true });
            }
        }
        if (twoPlayer) {
            autoplay.macroTrials.resize(6);  // +1, +1 swapped, 0, 0 swapped, -1, -1 swapped
        }
        if (autoplay.macroXd) {
            // alpha.26: xdBot's own frame count: no shift to find
            autoplay.macroTuned = true;
            autoplay.macroTrials = { { 0, false } };
            say("autoplay: an xdBot macro: it's played exactly the way xdBot plays it (setting: Play xdBot Macros Like xdBot)");
        }
        if (autoplay.macroForceX) {
            // alpha.7: played by position no shift matters, only which player is which. The player
            // with the most clicks goes first (Grief: the file's "player 2" has 1359, "player 1" 18,
            // the wave at the start is the visible player's)
            bool guess = twoPlayer && m.inputs.size() * 4 < m.inputs2.size();
            autoplay.macroTrials.clear();
            if (twoPlayer) {
                autoplay.macroTrials = { { 0, guess }, { 0, !guess } };
            }
            else {
                autoplay.macroTuned = true;
                autoplay.macroTrials = { { 0, false } };
            }
        }
        autoplay.macroTrialReach.assign(autoplay.macroTrials.size(), -1);
        autoplay.macroAutoOffset = autoplay.macroTrials[0].first;
        if (autoplay.macroTrials[0].second) {
            macroSetSwap(true);
            say("autoplay: the file's player 2 has most of the clicks ({} to {}): it's played as player 1 first", m.inputs2.size(), m.inputs.size());
        }
        autoplay.macroIdx2 = 0;
        autoplay.held2 = false;
        autoplay.known.clear();
        autoplay.knownUntil = 0;
        autoplay.knownTrail.clear();
        autoplay.knowsWholeLevel = false;
        autoplay.fromMacro = credit.empty() ? m.file : credit;
        cbfOffForMacro();
        // test 81: the shift found for this macro before
        if (plNow && plNow->m_level) {
            auto saved = Mod::get()->getSavedValue<std::string>(fmt::format("macro-shift-{}", plNow->m_level->m_levelID.value()), "");
            auto bar = saved.find('|');
            // alpha.18: found for the same way of playing it ('t': by time; none: by position)
            bool sameMode = bar != std::string::npos && (saved.substr(0, bar).find('t') != std::string::npos) == !autoplay.macroForceX;
            if (bar != std::string::npos && sameMode && saved.substr(bar + 1) == autoplay.fromMacro) {
                auto head = saved.substr(0, bar);
                autoplay.macroAutoOffset = std::atoi(head.c_str());
                autoplay.macroTuned = true;
                // alpha.12: either way (alpha.11 left the first guess's swap on when the saved one had none:
                // Grief's best, unswapped to 36%, came back swapped)
                macroSetSwap(head.find('s') != std::string::npos);
                say("autoplay: plays it shifted {:+d} step(s){}, as found before", autoplay.macroAutoOffset, autoplay.macroSwap ? ", players swapped" : "");
            }
        }
        // alpha.23: Low Detail Mode changes some levels' gameplay (a high-detail trigger that hides a
        // spike is gone in LDM): a macro only works with it set the way the macro was made (Heliopolis:
        // made with it off, played with it on, dead at 7.1% every time, on the bot's exact path)
        autoplay.macroLdmMismatch = false;
        if (plNow && plNow->m_level) {
            say("autoplay: Low Detail Mode: the level {} it, yours is {}, the macro's {}", plNow->m_level->m_lowDetailMode ? "has" : "doesn't have",
                plNow->m_level->m_lowDetailModeToggled ? "on" : "off", m.ldm ? "on" : "off");
        }
        if (plNow && plNow->m_level && plNow->m_level->m_lowDetailMode) {
            bool mine = plNow->m_level->m_lowDetailModeToggled;
            if (mine != m.ldm) {
                autoplay.macroLdmMismatch = true;
                say("autoplay: WARNING: this macro was made with Low Detail Mode {} and you have it {}: it dies where the two differ. Turn LDM {} for this level (the level's page, before you play) and enter it again",
                    m.ldm ? "ON" : "OFF", mine ? "ON" : "OFF", m.ldm ? "on" : "off");
            }
        }
        else if (m.ldm) {
            say("autoplay: this macro was made with Low Detail Mode on: turn it on for this level if it dies where the two differ");
        }
        say("autoplay: plays the macro {}{}{} ({} inputs, {:.1f} s{})", m.file, m.author.empty() ? "" : " by " + m.author,
            m.bot.empty() ? "" : " (" + m.bot + ")", ticks.size(), until / static_cast<double>(st(240)),
            std::abs(m.fps - st(240)) > 0.5 ? fmt::format(", made at {:.0f} TPS, you're at {}", m.fps, st(240)) : "");
        return true;
    }
