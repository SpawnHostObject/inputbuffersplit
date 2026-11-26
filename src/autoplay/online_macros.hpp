//@ autoplay/online_macros.hpp
//@ Part of main.cpp (single translation unit). Not a standalone file: do not add it to the build.
    // ===== test 69: online macros from Hyperbolus (hyperbolus.net, used with its owner's OK) =====
    // With no macro of your own for the level, the level's page on Hyperbolus is read once: its GDR
    // macros (the archive lists 12 a page), approved ones first, then the most downloaded, skipping
    // ones whose notes say they need noclip. The first that reads as a GDR macro for this level is
    // saved in the macros folder (so it's there next time, and you can delete it) and replayed from
    // the next attempt. Off with the "Online Macros" setting.
    std::set<int> onlineMacroTried;

    // alpha.30: Click Before Frames' Astral converter (web/astral-converter)
    constexpr char const* ASTRAL_CONVERTER = "https://cbf-astral.j6m7hh99sg.workers.dev";

    struct OnlineMacro {
        std::string file, url, author, notes;
        std::string source = "Hyperbolus";  // alpha.29: or MacroDash; alpha.30: or Astral
        double gameVersion = 0.0;  // alpha.30: as the site says (Astral's converted files don't carry it)
        bool approved = false;
        int downloads = 0;
    };

    std::string lower(std::string s) {
        for (auto& c : s) {
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        return s;
    }

    // the page's data: <script data-page="app" type="application/json">{...}</script>
    std::optional<matjson::Value> hyperbolusPage(std::string const& url) {
        auto res = web::WebRequest().timeout(std::chrono::seconds(20)).userAgent("ClickBeforeFrames").getSync(url);
        if (!res.ok()) {
            return std::nullopt;
        }
        auto html = res.string().unwrapOr("");
        std::string_view open = R"(<script data-page="app" type="application/json">)";
        auto a = html.find(open);
        if (a == std::string::npos) {
            return std::nullopt;
        }
        a += open.size();
        auto b = html.find("</script>", a);
        if (b == std::string::npos) {
            return std::nullopt;
        }
        auto parsed = matjson::parse(std::string_view(html).substr(a, b - a));
        if (parsed.isErr()) {
            return std::nullopt;
        }
        return parsed.unwrap();
    }

    void fetchOnlineMacro(int levelId) {
        if (levelId <= 0 || !Mod::get()->getSettingValue<bool>("online-macros") || onlineMacroTried.contains(levelId)) {
            return;
        }
        onlineMacroTried.insert(levelId);
        say("autoplay: looking for a macro of this level on Hyperbolus, MacroDash and Astral...");
        std::filesystem::path dir = macroDir();
        std::string levelName;
        if (auto* pl = GameManager::get()->m_playLayer; pl && pl->m_level && pl->m_level->m_levelID.value() == levelId) {
            levelName = pl->m_level->m_levelName;
        }
        // alpha.18: your TPS (the setting); a macro at another one is played at its own (Macro Plays At Its TPS)
        double tps = static_cast<double>(std::max<int64_t>(240, Mod::get()->getSettingValue<int64_t>("tps")));
        bool autoTps = Mod::get()->getSettingValue<bool>("macro-auto-tps");
        std::string bad = "|" + Mod::get()->getSavedValue<std::string>(fmt::format("macro-bad2-{}", levelId), "") + "|";  // alpha.6: a new list (macros dropped before for another TPS are tried again)
        std::thread([levelId, dir, tps, bad, autoTps, levelName] {
            auto tell = [](std::string msg) { queueInMainThread([msg] { say("{}", msg); }); };
            std::vector<OnlineMacro> list;
            int last = 1;
            for (int page = 1; page <= std::min(last, 8); ++page) {
                // test 71: Hyperbolus's own API (the level page's data before)
                std::optional<matjson::Value> data;
                auto api = web::WebRequest().timeout(std::chrono::seconds(20)).userAgent("ClickBeforeFrames")
                    .getSync(fmt::format("https://hyperbolus.net/api/macros?level_id={}&format=gdr&page={}", levelId, page));
                if (api.ok()) {
                    if (auto j = api.json(); j.isOk()) {
                        auto wrapped = matjson::Value::object();
                        wrapped["props"] = matjson::Value::object();
                        wrapped["props"]["replays"] = j.unwrap();
                        data = wrapped;
                    }
                }
                if (!data) {
                    data = hyperbolusPage(fmt::format("https://hyperbolus.net/level/{}/replays?page={}", levelId, page));
                }
                if (!data) {
                    if (page == 1) {
                        tell("autoplay: Hyperbolus has no macros of this level (or couldn't be reached)");
                    }
                    break;
                }
                auto& replays = (*data)["props"]["replays"];
                last = static_cast<int>(replays["last_page"].asInt().unwrapOr(1));
                if (!replays["data"].isArray()) {
                    break;
                }
                for (auto const& r : replays["data"]) {
                    if (r["format"].asString().unwrapOr("") != "gdr" || !r["files"].isArray() || r["files"].size() == 0) {
                        continue;
                    }
                    auto const& f = r["files"][0];
                    OnlineMacro o;
                    o.file = f["filename"].asString().unwrapOr("");
                    o.url = f["url"].asString().unwrapOr("");
                    o.downloads = static_cast<int>(f["downloads"].asInt().unwrapOr(0));
                    o.author = r["author"].isObject() ? r["author"]["name"].asString().unwrapOr("") : "";
                    o.notes = r["notes"].isString() ? r["notes"].asString().unwrapOr("") : "";
                    o.approved = !r["approved_at"].isNull();
                    auto name = lower(o.file);
                    bool gdrFile = name.ends_with(".gdr") || name.ends_with(".gdr.json") || name.ends_with(".json");
                    auto notes = lower(o.notes);
                    bool noclip = notes.find("noclip") != std::string::npos && notes.find("no noclip") == std::string::npos &&
                        notes.find("no need noclip") == std::string::npos && notes.find("without noclip") == std::string::npos &&
                        notes.find("no need for noclip") == std::string::npos;
                    if (gdrFile && !noclip && o.url.rfind("https://hyperbolus.net/", 0) == 0) {
                        list.push_back(std::move(o));
                    }
                }
            }
            // alpha.29: MacroDash (a collection of hard demons' macros on GitHub), by the level's name; the
            // file's own level id is checked when it's read
            if (!levelName.empty()) {
                auto norm = [](std::string const& v) {
                    std::string out;
                    for (unsigned char c : v) {
                        if (std::isalnum(c)) {
                            out += static_cast<char>(std::tolower(c));
                        }
                    }
                    return out;
                };
                std::string want = norm(levelName);
                std::string base = "https://raw.githubusercontent.com/downupstudio/macrodash/main/";
                auto idx = web::WebRequest().timeout(std::chrono::seconds(15)).userAgent("ClickBeforeFrames").getSync(base + "macros-data.js");
                if (idx.ok() && !want.empty()) {
                    std::string js = idx.string().unwrapOr("");
                    auto field = [](std::string const& chunk, std::string const& key) -> std::string {
                        auto k = chunk.find(key + ":");
                        if (k == std::string::npos) {
                            return "";
                        }
                        auto q1 = chunk.find('"', k);
                        auto q2 = q1 == std::string::npos ? q1 : chunk.find('"', q1 + 1);
                        return q2 == std::string::npos ? "" : chunk.substr(q1 + 1, q2 - q1 - 1);
                    };
                    for (size_t at = js.find("title:"); at != std::string::npos;) {
                        size_t next = js.find("title:", at + 6);
                        std::string chunk = js.substr(at, next == std::string::npos ? std::string::npos : next - at);
                        std::string title = field(chunk, "title"), path = field(chunk, "gdr");
                        if (norm(title) == want && path.rfind("macros/", 0) == 0 && path.find("..") == std::string::npos) {
                            OnlineMacro o;
                            o.source = "MacroDash";
                            o.file = path.substr(7);
                            o.url = base + path;
                            list.push_back(std::move(o));
                        }
                        at = next;
                    }
                }
            }
            // alpha.30: Astral's macro database (its own .ast format: converted to GDR by Click Before
            // Frames' converter, with Astral's own converter, used with the Astral developer's permission)
            if (!levelName.empty()) {
                std::string q;
                for (unsigned char c : levelName) {
                    if (std::isalnum(c) || c == '-' || c == '_' || c == '.') {
                        q += static_cast<char>(c);
                    }
                    else {
                        q += fmt::format("%{:02X}", c);
                    }
                }
                auto res = web::WebRequest().timeout(std::chrono::seconds(15)).userAgent("ClickBeforeFrames")
                    .getSync(fmt::format("https://astral-api.gwddos.workers.dev/api/macros?page=1&limit=50&search={}", q));
                if (res.ok()) {
                    if (auto j = res.json(); j.isOk() && j.unwrap()["macros"].isArray()) {
                        for (auto const& it : j.unwrap()["macros"]) {
                            std::string id = it["level_id"].isString() ? it["level_id"].asString().unwrapOr("") : std::to_string(it["level_id"].asInt().unwrapOr(0));
                            std::string fileName = it["file_name"].asString().unwrapOr("");
                            if (id != std::to_string(levelId) || fileName.empty() || !fileName.ends_with(".ast")) {
                                continue;
                            }
                            bool okName = fileName.size() < 120 && fileName.find("..") == std::string::npos &&
                                std::all_of(fileName.begin(), fileName.end(), [](unsigned char c) { return std::isalnum(c) || c == '_' || c == '.' || c == '-'; });
                            if (!okName) {
                                continue;
                            }
                            OnlineMacro o;
                            o.source = "Astral";
                            o.file = fileName;
                            o.url = std::string(ASTRAL_CONVERTER) + "/convert?file=" + fileName;
                            o.author = it["macro_author"].isString() ? it["macro_author"].asString().unwrapOr("") : "";
                            o.notes = it["description"].isString() ? it["description"].asString().unwrapOr("") : "";
                            o.downloads = it["downloads"].isNumber() ? static_cast<int>(it["downloads"].asInt().unwrapOr(0)) : 0;
                            o.gameVersion = std::atof(it["game_version"].asString().unwrapOr("0").c_str());
                            list.push_back(std::move(o));
                        }
                    }
                }
            }
            if (list.empty()) {
                return tell("autoplay: Hyperbolus, MacroDash and Astral have no macro of this level it can use");
            }
            std::stable_sort(list.begin(), list.end(), [](OnlineMacro const& a, OnlineMacro const& b) {
                if (a.approved != b.approved) {
                    return a.approved;
                }
                return a.downloads > b.downloads;
            });
            // test 77: up to 8 are downloaded and read; the one made at your TPS on the newest GD
            // (2.207+ first: macros from older 2.2 versions can go wrong in 2.2081), then approved, then
            // the most downloaded. Ones that failed you before are skipped.
            struct Pick {
                OnlineMacro o;
                Macro m;
                std::vector<uint8_t> data;
            };
            std::vector<Pick> picks;
            int skippedBad = 0;
            std::map<int, int> otherTps;  // test 86: macros made at another TPS (which, how many)
            bool anyLdm = false;
            for (size_t i = 0; i < list.size() && i < 8; ++i) {
                auto const& o = list[i];
                if (bad.find("|" + o.url + "|") != std::string::npos) {
                    ++skippedBad;
                    continue;
                }
                auto file = web::WebRequest().timeout(std::chrono::seconds(60)).userAgent("ClickBeforeFrames").followRedirects(true).getSync(o.url);
                if (!file.ok()) {
                    continue;
                }
                auto bytes = std::move(file).data();
                std::vector<uint8_t> data(bytes.begin(), bytes.end());
                std::string why;
                auto m = parseMacro(data, o.file, why);
                if (m && o.source == "Astral") {
                    // its converted file has a placeholder level id and GD version: the site's are used
                    m->levelId = levelId;
                    if (o.gameVersion > 0.0) {
                        m->gameVersion = o.gameVersion;
                    }
                    m->author = o.author;
                    m->bot = "Astral";
                }
                if (m && (m->levelId == 0 || m->levelId == levelId) && std::abs(m->fps - tps) > 0.5 && m->fixes.size() < 100 &&
                    !(autoTps && m->fps >= 239.5 && m->fps <= 1000.5)) {
                    ++otherTps[static_cast<int>(std::lround(m->fps))];
                    continue;
                }
                if (!m || (m->levelId != 0 && m->levelId != levelId)) {
                    continue;
                }
                picks.push_back({ o, std::move(*m), std::move(data) });
            }
            if (!picks.empty()) {
                // test 88: 2.206+ first (2.2069 left out 2.206 macros, which work: Thinking Space II;
                // Silent Acropolis's only working one), then ones with the bot's position records
                std::stable_sort(picks.begin(), picks.end(), [tps](Pick const& a, Pick const& b) {
                    // v3.1.1: one made at your TPS first (another TPS plays by position: less exact)
                    bool ta = std::abs(a.m.fps - tps) <= 0.5, tb = std::abs(b.m.fps - tps) <= 0.5;
                    if (ta != tb) {
                        return ta;
                    }
                    bool na = a.m.gameVersion >= 2.2055, nb = b.m.gameVersion >= 2.2055;
                    if (na != nb) {
                        return na;
                    }
                    bool fa = a.m.fixes.size() > 100, fb = b.m.fixes.size() > 100;
                    if (fa != fb) {
                        return fa;
                    }
                    if (a.o.approved != b.o.approved) {
                        return a.o.approved;
                    }
                    return a.o.downloads > b.o.downloads;
                });
                auto& best = picks.front();
                bool json = !best.data.empty() && best.data[0] == '{';
                auto path = dir / fmt::format("{}{}", levelId, json ? ".gdr.json" : ".gdr");
                std::error_code ec;
                std::filesystem::create_directories(dir, ec);
                std::filesystem::remove(dir / fmt::format("{}.gdr", levelId), ec);
                std::filesystem::remove(dir / fmt::format("{}.gdr.json", levelId), ec);
                (void)file::writeBinarySafe(path, ByteSpan(best.data));
                if (best.m.author.empty()) {
                    best.m.author = best.o.author;
                }
                std::string credit = fmt::format("{} by {} ({}, GD {:.4f})", best.o.file, best.o.author.empty() ? best.m.author : best.o.author,
                    best.o.source, best.m.gameVersion);
                std::string source = best.o.source;
                std::string url = best.o.url;
                size_t usable = picks.size();
                Macro got = std::move(best.m);
                queueInMainThread([got, credit, levelId, url, usable, source] {
                    Mod::get()->setSavedValue(fmt::format("macro-src-{}", levelId), url + "\n" + credit);
                    auto* layer = GameManager::get()->m_playLayer;
                    say("autoplay: downloaded {} from {} ({} macro{} found fit your TPS)", credit, source, usable, usable == 1 ? "" : "s");
                    if (!autoplay.active || !layer || !layer->m_level || layer->m_level->m_levelID.value() != levelId) {
                        return;
                    }
                    if (applyMacro(got, credit)) {
                        autoplay.macroUrl = url;
                    }
                });
                return;
            }
            if (!otherTps.empty()) {
                // test 86: say which TPS to set (Grief's only macro is made at 360)
                std::string which;
                for (auto const& [t, n] : otherTps) {
                    which += fmt::format("{}{} at {} TPS", which.empty() ? "" : ", ", n, t);
                }
                return tell(fmt::format("autoplay: Hyperbolus has macros of this level made at another TPS ({}); you play at {:.0f}. "
                    "Set the mod's TPS setting to {} and start autoplay again to use one", which, tps, otherTps.rbegin()->first));
            }
            if (skippedBad > 0) {
                queueInMainThread([levelId] { macroRestoreBest(levelId); });  // v3.1.1
                return tell(fmt::format("autoplay: no other Hyperbolus macro of this level to try ({} failed here before)", skippedBad));
            }
            tell("autoplay: none of the Hyperbolus macros of this level could be used (made at another TPS, or GDR2, which isn't supported yet)");
        }).detach();
    }
