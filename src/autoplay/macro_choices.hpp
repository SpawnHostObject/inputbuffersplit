//@ autoplay/macro_choices.hpp
//@ Part of main.cpp (single translation unit). Not a standalone file: do not add it to the build.
    // alpha.31: every macro of a level the sites have, for the player to pick one (Choose Macro). Only
    // what the lists say is fetched here; the file itself is downloaded when one is picked
    struct MacroChoice {
        OnlineMacro o;
        int tps = 0;          // as the site says (0: not known before it's downloaded)
        bool brain = false;   // the global brain's best run
        int replays = 0, fails = 0, inputs = 0;
    };

    std::vector<MacroChoice> listMacroChoices(int levelId, std::string const& levelName, std::string const& print, int myTps) {
        std::vector<MacroChoice> out;
        auto get = [](std::string const& url) {
            return web::WebRequest().timeout(std::chrono::seconds(15)).userAgent("ClickBeforeFrames").followRedirects(true).getSync(url);
        };
        // the global brain
        if (!std::string_view(online::URL).empty() && !print.empty()) {
            auto r = get(fmt::format("{}/api/runs?level={}&hash={}&tps={}", online::URL, levelId, print, myTps));
            if (r.ok()) {
                if (auto j = r.json(); j.isOk() && j.unwrap()["found"].asBool().unwrapOr(false)) {
                    auto v = j.unwrap();
                    MacroChoice c;
                    c.brain = true;
                    c.o.source = "Global brain";
                    c.o.file = "best shared run";
                    c.tps = myTps;
                    c.replays = static_cast<int>(v["replays"].asInt().unwrapOr(0));
                    c.fails = static_cast<int>(v["fails"].asInt().unwrapOr(0));
                    c.inputs = v["inputs"].isArray() ? static_cast<int>(v["inputs"].size()) : 0;
                    out.push_back(std::move(c));
                }
            }
        }
        // Hyperbolus
        for (int page = 1; page <= 3; ++page) {
            auto r = get(fmt::format("https://hyperbolus.net/api/macros?level_id={}&format=gdr&page={}", levelId, page));
            if (!r.ok()) {
                break;
            }
            auto j = r.json();
            if (!j.isOk() || !j.unwrap()["data"].isArray()) {
                break;
            }
            auto v = j.unwrap();
            for (auto const& it : v["data"]) {
                if (!it["files"].isArray() || it["files"].size() == 0) {
                    continue;
                }
                auto const& f = it["files"][0];
                MacroChoice c;
                c.o.file = f["filename"].asString().unwrapOr("");
                c.o.url = f["url"].asString().unwrapOr("");
                c.o.downloads = static_cast<int>(f["downloads"].asInt().unwrapOr(0));
                c.o.author = it["author"].isObject() ? it["author"]["name"].asString().unwrapOr("") : "";
                c.o.notes = it["notes"].isString() ? it["notes"].asString().unwrapOr("") : "";
                c.o.approved = !it["approved_at"].isNull();
                auto name = lower(c.o.file);
                if ((name.ends_with(".gdr") || name.ends_with(".json")) && c.o.url.rfind("https://hyperbolus.net/", 0) == 0) {
                    out.push_back(std::move(c));
                }
            }
            if (page >= static_cast<int>(v["last_page"].asInt().unwrapOr(1))) {
                break;
            }
        }
        auto norm = [](std::string const& v) {
            std::string r;
            for (unsigned char ch : v) {
                if (std::isalnum(ch)) {
                    r += static_cast<char>(std::tolower(ch));
                }
            }
            return r;
        };
        // MacroDash
        if (!levelName.empty()) {
            std::string base = "https://raw.githubusercontent.com/downupstudio/macrodash/main/";
            auto r = get(base + "macros-data.js");
            if (r.ok()) {
                std::string js = r.string().unwrapOr("");
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
                    std::string path = field(chunk, "gdr");
                    if (norm(field(chunk, "title")) == norm(levelName) && path.rfind("macros/", 0) == 0 && path.find("..") == std::string::npos) {
                        MacroChoice c;
                        c.o.source = "MacroDash";
                        c.o.file = path.substr(7);
                        c.o.url = base + path;
                        out.push_back(std::move(c));
                    }
                    at = next;
                }
            }
        }
        // Astral
        if (!levelName.empty()) {
            std::string q;
            for (unsigned char ch : levelName) {
                q += (std::isalnum(ch) || ch == '-' || ch == '_' || ch == '.') ? std::string(1, static_cast<char>(ch)) : fmt::format("%{:02X}", ch);
            }
            auto r = get(fmt::format("https://astral-api.gwddos.workers.dev/api/macros?page=1&limit=50&search={}", q));
            if (r.ok()) {
                if (auto j = r.json(); j.isOk() && j.unwrap()["macros"].isArray()) {
                    for (auto const& it : j.unwrap()["macros"]) {
                        std::string id = it["level_id"].isString() ? it["level_id"].asString().unwrapOr("") : std::to_string(it["level_id"].asInt().unwrapOr(0));
                        std::string fileName = it["file_name"].asString().unwrapOr("");
                        bool okName = !fileName.empty() && fileName.ends_with(".ast") && fileName.size() < 120 && fileName.find("..") == std::string::npos &&
                            std::all_of(fileName.begin(), fileName.end(), [](unsigned char ch) { return std::isalnum(ch) || ch == '_' || ch == '.' || ch == '-'; });
                        if (id != std::to_string(levelId) || !okName) {
                            continue;
                        }
                        MacroChoice c;
                        c.o.source = "Astral";
                        c.o.file = it["macro_name"].isString() ? it["macro_name"].asString().unwrapOr(fileName) : fileName;
                        c.o.url = std::string(ASTRAL_CONVERTER) + "/convert?file=" + fileName;
                        c.o.author = it["macro_author"].isString() ? it["macro_author"].asString().unwrapOr("") : "";
                        c.o.notes = it["description"].isString() ? it["description"].asString().unwrapOr("") : "";
                        c.o.gameVersion = std::atof(it["game_version"].isString() ? it["game_version"].asString().unwrapOr("0").c_str() : "0");
                        c.tps = it["tps"].isNumber() ? static_cast<int>(it["tps"].asInt().unwrapOr(0)) : 0;
                        out.push_back(std::move(c));
                    }
                }
            }
        }
        return out;
    }

    // alpha.31: the player picked one: it's downloaded, kept as this level's macro (autoplay doesn't
    // swap it for another or for the brain's run), and played from the next attempt
    void useMacroChoice(int levelId, MacroChoice const& c) {
        if (c.brain) {
            Mod::get()->setSavedValue(fmt::format("macro-pick-{}", levelId), std::string("brain"));
            say("autoplay: you picked the global brain's run for this level");
            auto* layer = GameManager::get()->m_playLayer;
            if (autoplay.active && layer && layer->m_level && layer->m_level->m_levelID.value() == levelId) {
                autoplay.brainSkip.clear();
                Mod::get()->setSavedValue(fmt::format("brain-skip-{}", levelId), std::string());
                brainFetch(levelId, levelFingerprint(layer->m_level));
            }
            return;
        }
        say("autoplay: downloading {} from {}...", c.o.file, c.o.source);
        std::filesystem::path dir = macroDir();
        OnlineMacro o = c.o;
        std::thread([levelId, o, dir] {
            auto file = web::WebRequest().timeout(std::chrono::seconds(60)).userAgent("ClickBeforeFrames").followRedirects(true).getSync(o.url);
            if (!file.ok()) {
                queueInMainThread([o] { say("autoplay: couldn't download {} from {}", o.file, o.source); });
                return;
            }
            auto bytes = std::move(file).data();
            std::vector<uint8_t> data(bytes.begin(), bytes.end());
            std::string why;
            auto m = parseMacro(data, o.file, why);
            if (m && o.source == "Astral") {
                m->levelId = levelId;
                if (o.gameVersion > 0.0) {
                    m->gameVersion = o.gameVersion;
                }
                m->author = o.author;
                m->bot = "Astral";
            }
            if (!m || (m->levelId != 0 && m->levelId != levelId)) {
                queueInMainThread([o, why] { say("autoplay: {} couldn't be read{}", o.file, why.empty() ? "" : ": " + why); });
                return;
            }
            bool json = !data.empty() && data[0] == '{';
            std::error_code ec;
            std::filesystem::create_directories(dir, ec);
            std::filesystem::remove(dir / fmt::format("{}.gdr", levelId), ec);
            std::filesystem::remove(dir / fmt::format("{}.gdr.json", levelId), ec);
            (void)file::writeBinarySafe(dir / fmt::format("{}{}", levelId, json ? ".gdr.json" : ".gdr"), ByteSpan(data));
            std::string credit = fmt::format("{} by {} ({}, GD {:.4f})", o.file, o.author.empty() ? m->author : o.author, o.source, m->gameVersion);
            Macro got = std::move(*m);
            queueInMainThread([got, credit, levelId, o] {
                Mod::get()->setSavedValue(fmt::format("macro-src-{}", levelId), o.url + "\n" + credit);
                Mod::get()->setSavedValue(fmt::format("macro-pick-{}", levelId), o.source);
                Mod::get()->setSavedValue(fmt::format("macro-final-{}", levelId), true);
                Mod::get()->setSavedValue(fmt::format("macro-shift-{}", levelId), std::string());
                say("autoplay: you picked {}: it's this level's macro from now on (Forget Macro goes back to automatic)", credit);
                auto* layer = GameManager::get()->m_playLayer;
                if (!autoplay.active || !layer || !layer->m_level || layer->m_level->m_levelID.value() != levelId) {
                    return;
                }
                autoplay.macroIsBrain = false;
                if (applyMacro(got, credit)) {
                    autoplay.macroUrl = o.url;
                    autoplay.needRestart = true;
                }
            });
        }).detach();
    }
