//@ autoplay/load_saved.hpp
//@ Part of main.cpp (single translation unit). Not a standalone file: do not add it to the build.
    void loadAutoplaySaved(PlayLayer* layer);
    void loadAutoplay(PlayLayer* layer) {
        loadAutoplaySaved(layer);
        loadMacro(layer);
    }

    void loadAutoplaySaved(PlayLayer* layer) {
        autoplayKey(layer);
        if (autoplay.saveKey.empty()) {
            return;
        }
        std::ifstream in(autoplayFile());
        std::string magic;
        int version = 0, rate = 0;
        size_t hash = 0;
        if (!(in >> magic >> version >> hash >> rate) || magic != "cbf-autoplay" || version != 1) {
            return;
        }
        if (hash != autoplay.levelHash || rate != st(240)) {
            say("autoplay: the level (or the TPS) changed since it was learned, it learns it again");
            return;
        }
        int until = 0, whole = 0;
        if (!(in >> until >> whole)) {
            return;
        }
        std::string kind;
        while (in >> kind) {
            if (kind == "f") {
                int t = 0, down = 0;
                if (in >> t >> down) {
                    autoplay.known.push_back({ t, down != 0 });
                }
            }
            else if (kind == "d") {
                float x = 0.f, y = 0.f, w = 0.f, h = 0.f;
                // test 60: test 59's big marks (what killed you, grown) are left out when loaded
                if ((in >> x >> y >> w >> h) && w <= 30.f && h <= 30.f) {
                    buf.autoDangers.push_back(CCRect(x, y, w, h));
                }
            }
            else if (kind == "p") {
                float x = 0.f, y = 0.f;
                if (in >> x >> y) {
                    autoplay.knownTrail.push_back({ x, y });
                }
            }
        }
        autoplay.knownUntil = until;
        autoplay.knowsWholeLevel = whole != 0;
        say("autoplay: loaded what it learned here ({} inputs, {}){}", autoplay.known.size(),
            autoplay.knowsWholeLevel ? "the whole level" : fmt::format("{:.1f} s of the level", until / static_cast<double>(st(240))),
            ", it follows the trail from the next attempt");
    }
