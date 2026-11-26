//@ autoplay/macro_format.hpp
//@ Part of main.cpp (single translation unit). Not a standalone file: do not add it to the build.
    // ===== test 67: macros =====
    // A GD bot macro (GDR, the format most 2.2 bots save: .gdr is MessagePack, .gdr.json / .json is
    // JSON) in the mod's "macros" folder, made for this level, becomes what autoplay replays: it plays
    // it input by input from the start, and where it dies it learns from there like after any death
    // (the replay is kept up to just before, the route plans the rest). Player 1's jump only; not from
    // a start position (the macro's ticks count from the level's start).

    // a small MessagePack reader (only what GDR files use) into matjson
    struct MsgPack {
        std::vector<uint8_t> const& d;
        size_t p = 0;
        bool bad = false;

        uint64_t be(int n) {
            if (p + n > d.size()) {
                bad = true;
                return 0;
            }
            uint64_t v = 0;
            for (int i = 0; i < n; ++i) {
                v = (v << 8) | d[p++];
            }
            return v;
        }

        std::string str(size_t n) {
            if (p + n > d.size()) {
                bad = true;
                return "";
            }
            std::string s(reinterpret_cast<char const*>(d.data() + p), n);
            p += n;
            return s;
        }

        matjson::Value arr(size_t n, int depth) {
            auto a = matjson::Value::array();
            for (size_t i = 0; i < n && !bad; ++i) {
                a.push(read(depth + 1));
            }
            return a;
        }

        matjson::Value map(size_t n, int depth) {
            auto o = matjson::Value::object();
            for (size_t i = 0; i < n && !bad; ++i) {
                auto k = read(depth + 1);
                auto v = read(depth + 1);
                o[k.isString() ? k.asString().unwrapOr("") : k.dump()] = v;
            }
            return o;
        }

        matjson::Value read(int depth = 0) {
            if (bad || depth > 64 || p >= d.size()) {
                bad = true;
                return nullptr;
            }
            uint8_t t = d[p++];
            if (t <= 0x7f) return static_cast<int64_t>(t);
            if (t >= 0xe0) return static_cast<int64_t>(static_cast<int8_t>(t));
            if ((t & 0xf0) == 0x80) return map(t & 0x0f, depth);
            if ((t & 0xf0) == 0x90) return arr(t & 0x0f, depth);
            if ((t & 0xe0) == 0xa0) return str(t & 0x1f);
            switch (t) {
                case 0xc0: return nullptr;
                case 0xc2: return false;
                case 0xc3: return true;
                case 0xc4: p += be(1); return nullptr;
                case 0xc5: p += be(2); return nullptr;
                case 0xc6: p += be(4); return nullptr;
                case 0xca: { uint32_t u = static_cast<uint32_t>(be(4)); float f; std::memcpy(&f, &u, 4); return static_cast<double>(f); }
                case 0xcb: { uint64_t u = be(8); double f; std::memcpy(&f, &u, 8); return f; }
                case 0xcc: return static_cast<int64_t>(be(1));
                case 0xcd: return static_cast<int64_t>(be(2));
                case 0xce: return static_cast<int64_t>(be(4));
                case 0xcf: return static_cast<int64_t>(be(8));
                case 0xd0: return static_cast<int64_t>(static_cast<int8_t>(be(1)));
                case 0xd1: return static_cast<int64_t>(static_cast<int16_t>(be(2)));
                case 0xd2: return static_cast<int64_t>(static_cast<int32_t>(be(4)));
                case 0xd3: return static_cast<int64_t>(be(8));
                case 0xd9: return str(be(1));
                case 0xda: return str(be(2));
                case 0xdb: return str(be(4));
                case 0xdc: return arr(be(2), depth);
                case 0xdd: return arr(be(4), depth);
                case 0xde: return map(be(2), depth);
                case 0xdf: return map(be(4), depth);
                case 0xd4: p += 2; return nullptr;
                case 0xd5: p += 3; return nullptr;
                case 0xd6: p += 5; return nullptr;
                case 0xd7: p += 9; return nullptr;
                case 0xd8: p += 17; return nullptr;
                case 0xc7: { auto n = be(1); p += 1 + n; return nullptr; }
                case 0xc8: { auto n = be(2); p += 1 + n; return nullptr; }
                case 0xc9: { auto n = be(4); p += 1 + n; return nullptr; }
                default: bad = true; return nullptr;
            }
        }
    };

    std::filesystem::path macroDir() {
        return Mod::get()->getConfigDir() / "macros";
    }

    struct Macro {
        std::string file, author, bot;
        std::vector<MacroFix> fixFull;  // test 89
        double fps = 240.0;
        int levelId = 0;
        std::vector<std::pair<int, bool>> inputs;  // (macro frame, down) for player 1's jump
        std::vector<std::pair<int, bool>> inputs2;  // test 70: the same, marked as player 2's
        std::vector<std::pair<int, float>> fixes;  // test 71: (frame, player 1's x) where the bot recorded it
        int lastFrame = 0;
        double gameVersion = 0.0;  // test 77
        bool ldm = false;  // test 86
    };

    std::optional<Macro> parseMacro(std::vector<uint8_t> const& data, std::string const& fileName, std::string& why);
    std::optional<Macro> readMacro(std::filesystem::path const& path, std::string& why) {
        std::ifstream f(path, std::ios::binary);
        std::vector<uint8_t> data((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
        return parseMacro(data, path.filename().string(), why);
    }

    std::optional<Macro> parseMacro(std::vector<uint8_t> const& data, std::string const& fileName, std::string& why) {
        if (data.empty()) {
            why = "empty file";
            return std::nullopt;
        }
        matjson::Value j;
        if (data[0] == '{') {
            auto parsed = matjson::parse(std::string_view(reinterpret_cast<char const*>(data.data()), data.size()));
            if (parsed.isErr()) {
                why = "not valid JSON";
                return std::nullopt;
            }
            j = parsed.unwrap();
        }
        else if (data.size() > 3 && data[0] == 'G' && data[1] == 'D' && data[2] == 'R') {
            why = "a GDR2 file (not supported yet: save it as GDR / .gdr.json)";
            return std::nullopt;
        }
        else {
            MsgPack mp { data };
            j = mp.read();
            if (mp.bad || !j.isObject()) {
                why = "not a GDR macro";
                return std::nullopt;
            }
        }
        Macro m;
        m.file = fileName;
        m.author = j["author"].asString().unwrapOr("");
        // test 69: the GDR files people upload say "bot" / "level" (older writers: "botInfo" / "levelInfo")
        auto sub = [&](char const* a, char const* b) -> matjson::Value {
            if (j.contains(a) && j[a].isObject()) {
                return j[a];
            }
            if (j.contains(b) && j[b].isObject()) {
                return j[b];
            }
            return matjson::Value::object();
        };
        auto botInfo = sub("bot", "botInfo");
        auto levelInfo = sub("level", "levelInfo");
        m.bot = botInfo.contains("name") ? botInfo["name"].asString().unwrapOr("") : "";
        m.fps = j["framerate"].asDouble().unwrapOr(240.0);
        m.gameVersion = j.contains("gameVersion") ? j["gameVersion"].asDouble().unwrapOr(0.0) : 0.0;
        m.ldm = j.contains("ldm") && j["ldm"].asBool().unwrapOr(false);
        if (!(m.fps >= 30.0 && m.fps <= 100000.0)) {
            m.fps = 240.0;
        }
        m.levelId = levelInfo.contains("id") ? static_cast<int>(levelInfo["id"].asInt().unwrapOr(0)) : 0;
        if (j["platformer"].asBool().unwrapOr(false)) {
            why = "a platformer macro";
            return std::nullopt;
        }
        if (!j["inputs"].isArray()) {
            why = "no inputs in it";
            return std::nullopt;
        }
        for (auto const& in : j["inputs"]) {
            int btn = static_cast<int>(in["btn"].asInt().unwrapOr(1));
            bool p2 = in["2p"].asBool().unwrapOr(false);
            if (btn != 1) {
                continue;  // left / right (platformer)
            }
            int frame = static_cast<int>(in["frame"].asInt().unwrapOr(-1));
            if (frame < 0) {
                continue;
            }
            // test 70: iCreate saves most of player 1's clicks as player 2's (Zodiac: 1078 of 1209), and
            // outside 2-player levels both are your clicks: both are kept, joined when it's played
            (p2 ? m.inputs2 : m.inputs).push_back({ frame, in["down"].asBool().unwrapOr(false) });
            m.lastFrame = std::max(m.lastFrame, frame);
        }
        std::stable_sort(m.inputs2.begin(), m.inputs2.end(), [](auto const& a, auto const& b) { return a.first < b.first; });
        if (j.contains("frameFixes") && j["frameFixes"].isArray()) {
            for (auto const& f : j["frameFixes"]) {
                if (f.contains("p1") && f["p1"].isObject() && f["p1"].contains("x")) {
                    m.fixes.push_back({ static_cast<int>(f["frame"].asInt().unwrapOr(-1)), static_cast<float>(f["p1"]["x"].asDouble().unwrapOr(0.0)) });
                    MacroFix fx;
                    fx.frame = static_cast<int>(f["frame"].asInt().unwrapOr(-1));
                    auto const& a = f["p1"];
                    fx.x = static_cast<float>(a["x"].asDouble().unwrapOr(0.0));
                    fx.y = a.contains("y") ? static_cast<float>(a["y"].asDouble().unwrapOr(0.0)) : 0.f;
                    fx.r = a.contains("r") ? static_cast<float>(a["r"].asDouble().unwrapOr(0.0)) : 0.f;
                    if (f.contains("p2") && f["p2"].isObject() && f["p2"].contains("y")) {
                        auto const& b = f["p2"];
                        fx.p2 = true;
                        fx.x2 = b.contains("x") ? static_cast<float>(b["x"].asDouble().unwrapOr(0.0)) : fx.x;
                        fx.y2 = static_cast<float>(b["y"].asDouble().unwrapOr(0.0));
                        fx.r2 = b.contains("r") ? static_cast<float>(b["r"].asDouble().unwrapOr(0.0)) : 0.f;
                    }
                    m.fixFull.push_back(fx);
                }
            }
            std::sort(m.fixes.begin(), m.fixes.end());
            std::sort(m.fixFull.begin(), m.fixFull.end(), [](MacroFix const& a, MacroFix const& b) { return a.frame < b.frame; });
        }
        if (m.inputs.empty() && !m.inputs2.empty()) {
            // nothing for player 1 at all: player 2's are the clicks
            m.inputs.swap(m.inputs2);
        }
        if (m.inputs.empty()) {
            why = "no player 1 jump inputs in it";
            return std::nullopt;
        }
        // test 69: inputs aren't always in order in the file (a release listed before the one before it)
        std::stable_sort(m.inputs.begin(), m.inputs.end(), [](auto const& a, auto const& b) { return a.first < b.first; });
        return m;
    }
