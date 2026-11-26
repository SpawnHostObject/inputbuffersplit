//@ core/export_log.hpp
//@ Part of main.cpp (single translation unit). Not a standalone file: do not add it to the build.
    size_t memorySpots();

    // v2.29 F8: copy the console to the clipboard (and save it to console-export.txt in the
    // mod's save folder), with the version, level, physics rate and settings on top, so it
    // can be pasted somewhere in one go.
    void exportConsole() {
        auto* mod = Mod::get();
        auto* layer = GameManager::get()->m_playLayer;
        std::string text;
        text += fmt::format("Click Before Frames {} console export\n", mod->getVersion().toVString());
        if (layer && layer->m_level) {
            text += fmt::format("Level: {} (id {}){}\n", std::string(layer->m_level->m_levelName), layer->m_level->m_levelID.value(),
                layer->m_isPracticeMode ? ", practice" : "");
        }
        float dt = buf.tickLength > 0.001f ? buf.tickLength : TICK;
        text += fmt::format("{} | physics ~{:.0f} steps/s\n", stageText(), 60.f / dt);
        text += fmt::format("budget {:g} ms | smart {} | full state {} | robust {} | late crash {} | practice only {}\n", buf.budgetMs,
            buf.smart, buf.fullState, buf.robustSaves, buf.lateCrash, buf.practiceOnly);
        text += fmt::format("orbs: align {} (ground {}, {}t) timing {} assume {} both {} | memory {} ({} spots)\n", buf.orbAlign,
            buf.orbAlignGround, buf.orbWindow, buf.orbTiming, buf.assumeOrbs, buf.tryBothOrbs, buf.learn, memorySpots());
        text += fmt::format("wave: v4.6 {} margin {} horizon {} spam gap {} | robot smart {} | spider smart {} | ship {} (max {}t)\n",
            buf.waveV46, buf.waveMargin, buf.waveHorizon, buf.spamGap, buf.robotSmart, buf.spiderSmart, buf.shipBuffer, buf.shipMaxDelay);
        text += fmt::format("auto-release: wave {} robot {} dash {} | auto-click {} | normal runs {} window {} | movers: avoid {} predict {}\n",
            buf.autoRelease, buf.robotAutoRelease, buf.dashAutoRelease, buf.autoClick, buf.autoNormalRuns, buf.autoWindow, buf.avoidMovers,
            buf.predictMovers);
        text += fmt::format("debug: drift check {} | path view {} | perf log {} | death log {} | ignore touching {}\n", buf.driftCheck,
            buf.showPath, buf.perfLog, buf.deathLog, buf.ignoreTouching);
        text += fmt::format("explain inputs {} | low latency {} | cbf sync {}{}\n", buf.explain, buf.lowLatency, buf.cbfSync,
            Loader::get()->isModLoaded("syzzi.click_between_frames") ? " (CBF loaded)" : "");
        // test 78: CBF's own settings (Physics Bypass changes physics even with CBF disabled)
        if (auto* cbf = Loader::get()->getLoadedMod("syzzi.click_between_frames")) {
            auto b = [&](char const* k) -> std::string {
                return cbf->hasSetting(k) ? (cbf->getSettingValue<bool>(k) ? "on" : "off") : "?";
            };
            text += fmt::format("CBF: disabled {} | click on steps {} | physics bypass {} | precision fix {}\n", b("soft-toggle"),
                b("click-on-steps"), b("physics-bypass"), b("precision-fix"));
        }
        // v2.39: CBF Extrapolate only changes what's drawn (a hidden player run ahead to "now"),
        // never the real player's physics, so predictions aren't affected by it.
        if (Loader::get()->isModLoaded("square3ang.cbfextrapolate")) {
            text += "CBF Extrapolate loaded (visual only)\n";
        }
        text += "----\n";
        for (auto const& line : exportLines) {
            text += line;
            text += '\n';
        }

        bool copied = geode::utils::clipboard::write(text);
        std::error_code ec;
        std::filesystem::create_directories(mod->getSaveDir(), ec);
        std::ofstream(mod->getSaveDir() / "console-export.txt", std::ios::trunc) << text;
        say("console {} ({} lines) + saved to console-export.txt", copied ? "copied to clipboard" : "NOT copied", exportLines.size());
    }

    std::string stageText() {
        if (!buf.enabled) {
            return "Buffer: OFF";
        }
        if (buf.perMode) {
            return fmt::format("Buffer: per mode{}", buf.train ? " [training]" : "");
        }
        return fmt::format("Buffer{}: {:g}f / wave {:g}f{}", STAGES[buf.stage].custom ? " (custom)" : "", buf.ticksAll * TICK,
            buf.ticksWave * TICK, buf.train ? " [training]" : "");
    }
