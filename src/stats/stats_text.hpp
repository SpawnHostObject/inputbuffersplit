//@ stats/stats_text.hpp
//@ Part of main.cpp (single translation unit). Not a standalone file: do not add it to the build.
    // v2.51 stats popup text
    std::string statsText() {
        auto const& s = sessionStats;
        std::string text;
        if (s.saves == 0) {
            text += "No early clicks yet this session.\n";
        }
        else {
            text += fmt::format("<cp>{}</c> {} in <cp>{}</c> attempts, on average <cp>{:.1f}t</c> ({:.2f}f) early.\n",
                s.saves, buf.train ? "early clicks" : "saves", s.attempts, static_cast<double>(s.earlySum) / s.saves, s.earlySum * TICK / s.saves);
            std::vector<std::pair<int, int>> spots(s.savesAt.begin(), s.savesAt.end());
            std::sort(spots.begin(), spots.end(), [](auto const& a, auto const& b) { return a.second > b.second; });
            text += "Most early clicks at:";
            for (size_t i = 0; i < std::min<size_t>(3, spots.size()); ++i) {
                text += fmt::format(" <cy>{}%</c> ({}x)", spots[i].first, spots[i].second);
            }
            text += "\n";
        }
        auto* layer = GameManager::get()->m_playLayer;
        if (layer && layer->m_levelLength > 0.f) {
            std::map<int, uint32_t> deathsAt;
            for (auto const& [slice, spot] : memory) {
                if (spot.deaths > 0) {
                    deathsAt[std::clamp(static_cast<int>((slice + 0.5f) * SLICE / layer->m_levelLength * 100.f), 0, 100)] += spot.deaths;
                }
            }
            std::vector<std::pair<int, uint32_t>> spots(deathsAt.begin(), deathsAt.end());
            std::sort(spots.begin(), spots.end(), [](auto const& a, auto const& b) { return a.second > b.second; });
            if (!spots.empty()) {
                text += "Most deaths at:";
                for (size_t i = 0; i < std::min<size_t>(3, spots.size()); ++i) {
                    text += fmt::format(" <cr>{}%</c> ({}x)", spots[i].first, spots[i].second);
                }
                text += "\n";
            }
        }
        if (!lastBench.empty()) {
            text += "Run check: " + lastBench + "\n";
        }
        return text;
    }
