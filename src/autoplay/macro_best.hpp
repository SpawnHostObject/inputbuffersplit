//@ autoplay/macro_best.hpp
//@ Part of main.cpp (single translation unit). Not a standalone file: do not add it to the build.
    // a death: keep this attempt's inputs up to a bit before it (normal mode, bot-only attempts)
    // v3.1.1: the macro that got furthest on a level, kept aside as <id>.best.gdr while others are tried
    void macroKeepBest(int lid, int reached, std::string const& src) {
        auto key = fmt::format("macro-best-{}", lid);
        auto saved = Mod::get()->getSavedValue<std::string>(key, "");
        int bestReach = saved.empty() ? -1 : std::atoi(saved.c_str());
        if (reached <= bestReach) {
            return;
        }
        std::error_code ec;
        for (auto ext : { ".gdr", ".gdr.json" }) {
            auto from = macroDir() / fmt::format("{}{}", lid, ext);
            if (std::filesystem::exists(from, ec)) {
                std::filesystem::remove(macroDir() / fmt::format("{}.best.gdr", lid), ec);
                std::filesystem::remove(macroDir() / fmt::format("{}.best.gdr.json", lid), ec);
                std::filesystem::copy_file(from, macroDir() / fmt::format("{}.best{}", lid, ext), ec);
            }
        }
        Mod::get()->setSavedValue(key, fmt::format("{}\n{}", reached, src));
    }

    // no other macro to try: the best one comes back and stays
    void macroRestoreBest(int lid) {
        auto saved = Mod::get()->getSavedValue<std::string>(fmt::format("macro-best-{}", lid), "");
        auto nl = saved.find('\n');
        if (nl == std::string::npos) {
            return;
        }
        std::error_code ec;
        bool back = false;
        for (auto ext : { ".gdr", ".gdr.json" }) {
            auto from = macroDir() / fmt::format("{}.best{}", lid, ext);
            if (std::filesystem::exists(from, ec)) {
                std::filesystem::rename(from, macroDir() / fmt::format("{}{}", lid, ext), ec);
                back = !ec;
            }
        }
        if (!back) {
            return;
        }
        Mod::get()->setSavedValue(fmt::format("macro-src-{}", lid), saved.substr(nl + 1));
        Mod::get()->setSavedValue(fmt::format("macro-final-{}", lid), true);
        say("autoplay: no other macro gets further: back to the best one (to step {}), and it's kept from now on", std::atoi(saved.c_str()));
        auto* layer = GameManager::get()->m_playLayer;
        if (autoplay.active && layer && layer->m_level && layer->m_level->m_levelID.value() == lid) {
            loadLocalMacro(layer);
        }
    }
