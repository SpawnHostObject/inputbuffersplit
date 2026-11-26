//@ autoplay/persistence.hpp
//@ Part of main.cpp (single translation unit). Not a standalone file: do not add it to the build.
    Autoplay autoplay;

    // v3.0 test 24: what autoplay learned on a level is saved, so it plays the known part right
    // from the first attempt next time. A level that changed (new version) starts fresh.
    int trailEvery() {
        return st(4);
    }

    std::filesystem::path autoplayFile() {
        return Mod::get()->getSaveDir() / "autoplay" / (autoplay.saveKey + ".txt");
    }

    void autoplayKey(PlayLayer* layer) {
        auto* level = layer->m_level;
        autoplay.saveKey.clear();
        autoplay.levelHash = 0;
        if (!level) {
            return;
        }
        int id = level->m_levelID.value();
        if (id > 0) {
            autoplay.saveKey = fmt::format("level_{}", id);
        }
        else {
            std::string name = level->m_levelName;
            for (auto& ch : name) {
                if (!std::isalnum(static_cast<unsigned char>(ch))) {
                    ch = '_';
                }
            }
            autoplay.saveKey = "local_" + name;
        }
        // v3.0 test 26: a start position is a different run: its own save
        if (layer->m_startPosObject) {
            autoplay.saveKey += fmt::format("_start{}", static_cast<int>(layer->m_startPosObject->getPositionX()));
        }
        autoplay.levelHash = std::hash<std::string> {}(std::string(level->m_levelString)) ^ static_cast<size_t>(level->m_levelVersion);
    }

    void saveAutoplay() {
        if (autoplay.saveKey.empty() || (autoplay.knownUntil <= 0 && buf.autoDangers.empty())) {
            return;
        }
        std::error_code ec;
        std::filesystem::create_directories(autoplayFile().parent_path(), ec);
        std::ofstream out(autoplayFile(), std::ios::trunc);
        out << "cbf-autoplay 1 " << autoplay.levelHash << ' ' << st(240) << '\n';
        out << autoplay.knownUntil << ' ' << (autoplay.knowsWholeLevel ? 1 : 0) << '\n';
        for (auto const& [t, down] : autoplay.known) {
            out << "f " << t << ' ' << (down ? 1 : 0) << '\n';
        }
        for (auto const& pt : autoplay.knownTrail) {
            out << "p " << pt.x << ' ' << pt.y << '\n';
        }
        for (auto const& d : buf.autoDangers) {
            out << "d " << d.origin.x << ' ' << d.origin.y << ' ' << d.size.width << ' ' << d.size.height << '\n';
        }
    }

    void drawTrail() {
        auto* d = autoplay.trailNode;
        if (!d) {
            return;
        }
        d->clear();
        auto const& pts = autoplay.knownTrail;
        ccColor4F line { 0.35f, 1.f, 0.65f, 1.f };
        for (size_t i = 1; i < pts.size(); ++i) {
            // a teleport portal jumps far in one step: no line across it
            if (std::abs(pts[i].x - pts[i - 1].x) < 150.f && std::abs(pts[i].y - pts[i - 1].y) < 150.f) {
                d->drawSegment(pts[i - 1], pts[i], 1.2f, line);
            }
        }
        // the presses on the trail, as dots
        int every = std::max(1, trailEvery());
        for (auto const& [t, down] : autoplay.known) {
            size_t i = static_cast<size_t>(t / every);
            if (down && i < pts.size()) {
                d->drawDot(pts[i], 3.f, { 1.f, 0.85f, 0.3f, 1.f });
            }
        }
    }
