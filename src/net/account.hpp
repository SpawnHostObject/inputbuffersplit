//@ net/account.hpp
//@ Part of main.cpp (single translation unit). Not a standalone file: do not add it to the build.
    // ===== test 91: your account (a game linked to your Discord on the CBF site) =====
    namespace account {
        struct Stats {
            bool loaded = false;
            bool linked = false;
            std::string name;
            int games = 0, levelsBeaten = 0, finishes = 0, runsShared = 0, othersUsed = 0;
            std::vector<std::string> recent;
        };
        Stats stats;
        bool dirty = false;      // the window shows it again
        bool linking = false;
        std::string code;        // the code the browser shows while linking
        std::string status;

        // test 92: autoplay and updates need this game linked to Discord on the CBF site (checked with
        // the site at start; the last answer is kept for when it can't be reached)
        bool isLinked() {
            if (stats.loaded) {
                return stats.linked;
            }
            return !Mod::get()->getSavedValue<std::string>("account-name", "").empty();
        }

        void refresh() {
            if (std::string_view(online::URL).empty()) {
                return;
            }
            std::string url = fmt::format("{}/api/account?id={}", online::URL, online::id());
            std::thread([url] {
                auto res = web::WebRequest().timeout(std::chrono::seconds(15)).userAgent("ClickBeforeFrames").getSync(url);
                Stats st;
                if (res.ok()) {
                    if (auto j = res.json(); j.isOk()) {
                        auto v = j.unwrap();
                        st.loaded = true;
                        st.linked = v["linked"].asBool().unwrapOr(false);
                        st.name = v["name"].asString().unwrapOr("");
                        st.games = static_cast<int>(v["games"].asInt().unwrapOr(1));
                        st.levelsBeaten = static_cast<int>(v["levelsBeaten"].asInt().unwrapOr(0));
                        st.finishes = static_cast<int>(v["finishes"].asInt().unwrapOr(0));
                        st.runsShared = static_cast<int>(v["runsShared"].asInt().unwrapOr(0));
                        st.othersUsed = static_cast<int>(v["othersFinishedWithYours"].asInt().unwrapOr(0));
                        if (v["recent"].isArray()) {
                            for (auto const& r : v["recent"]) {
                                st.recent.push_back(r["name"].asString().unwrapOr("?"));
                            }
                        }
                    }
                }
                queueInMainThread([st] {
                    if (st.loaded) {
                        stats = st;
                        Mod::get()->setSavedValue("account-name", st.linked ? st.name : std::string());
                    }
                    else {
                        status = "Couldn't reach the CBF site.";
                    }
                    dirty = true;
                });
            }).detach();
        }

        // the game gets a code, the browser opens the site's link page, and the game asks every 3 s
        // whether you pressed Link there (for 10 minutes)
        void startLink() {
            if (linking || std::string_view(online::URL).empty()) {
                return;
            }
            linking = true;
            status = "Opening the CBF site...";
            dirty = true;
            std::string body = fmt::format(R"({{"id":"{}"}})", online::id());
            std::thread([body] {
                auto res = web::WebRequest().timeout(std::chrono::seconds(15)).userAgent("ClickBeforeFrames").header("Content-Type", "application/json")
                    .bodyString(body).postSync(std::string(online::URL) + "/api/link/start");
                std::string c, url;
                if (res.ok()) {
                    if (auto j = res.json(); j.isOk()) {
                        c = j.unwrap()["code"].asString().unwrapOr("");
                        url = j.unwrap()["url"].asString().unwrapOr("");
                    }
                }
                if (c.empty()) {
                    queueInMainThread([] {
                        linking = false;
                        status = "Couldn't reach the CBF site.";
                        dirty = true;
                    });
                    return;
                }
                queueInMainThread([c, url] {
                    code = c;
                    status = fmt::format("Log in with Discord in the browser and press Link (code {}).", c);
                    dirty = true;
                    web::openLinkInBrowser(url);
                });
                for (int i = 0; i < 200; ++i) {
                    std::this_thread::sleep_for(std::chrono::seconds(3));
                    auto p = web::WebRequest().timeout(std::chrono::seconds(10)).userAgent("ClickBeforeFrames")
                        .getSync(fmt::format("{}/api/link/poll?code={}", online::URL, c));
                    if (!p.ok()) {
                        continue;
                    }
                    auto j = p.json();
                    if (j.isErr()) {
                        continue;
                    }
                    auto v = j.unwrap();
                    if (v["linked"].asBool().unwrapOr(false)) {
                        std::string name = v["name"].asString().unwrapOr("");
                        queueInMainThread([name] {
                            linking = false;
                            code.clear();
                            status = "Linked to " + name + ".";
                            say("account: this game is linked to {} on the CBF site", name);
                            refresh();
                        });
                        return;
                    }
                    if (v["expired"].asBool().unwrapOr(false)) {
                        break;
                    }
                }
                queueInMainThread([] {
                    linking = false;
                    code.clear();
                    status = "The code ran out. Press Link with Discord again.";
                    dirty = true;
                });
            }).detach();
        }
    }
