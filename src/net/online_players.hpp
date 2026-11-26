//@ net/online_players.hpp
//@ Part of main.cpp (single translation unit). Not a standalone file: do not add it to the build.
// test 66: online players. Every 90 s (setting "Show Online Players") the game sends the CBF server
// an anonymous random id, its platform and the mod's version, nothing else; the server answers with
// how many are online and offline, shown in the window's footer. The server's code is in web/.
namespace online {
    // the worker's address once it's deployed (web/README.md); empty = nothing is sent
    constexpr char const* URL = "https://clickbeforeframes.j6m7hh99sg.workers.dev";
    std::atomic<int> nowOnline { -1 };
    std::atomic<int> nowOffline { -1 };
    bool started = false;

    std::string id() {
        auto saved = Mod::get()->getSavedValue<std::string>("online-id", "");
        if (saved.size() == 32) {
            return saved;
        }
        std::random_device rd;
        std::string made;
        for (int i = 0; i < 32; ++i) {
            made += "0123456789abcdef"[rd() % 16];
        }
        Mod::get()->setSavedValue("online-id", made);
        return made;
    }

    char const* platform() {
#if defined(GEODE_IS_WINDOWS)
        return "Windows";
#elif defined(GEODE_IS_ANDROID)
        return "Android";
#elif defined(GEODE_IS_IOS)
        return "iOS";
#elif defined(GEODE_IS_MACOS)
        return "macOS";
#else
        return "Other";
#endif
    }

    void start() {
        if (started || std::string_view(URL).empty()) {
            return;
        }
        // v3.0.2: test builds (a version like v3.0.2-alpha.1) aren't counted on the site
        if (Mod::get()->getVersion().toVString().find('-') != std::string::npos) {
            return;
        }
        started = true;
        std::string body = fmt::format(R"({{"id":"{}","platform":"{}","version":"{}"}})", id(), platform(),
            Mod::get()->getVersion().toVString());
        std::thread([body] {
            while (true) {
                if (Mod::get()->getSettingValue<bool>("online-players")) {
                    auto res = web::WebRequest()
                        .timeout(std::chrono::seconds(10))
                        .userAgent("ClickBeforeFrames")
                        .header("Content-Type", "application/json")
                        .bodyString(body)
                        .postSync(std::string(URL) + "/api/ping");
                    if (res.ok()) {
                        if (auto json = res.json(); json.isOk()) {
                            nowOnline = static_cast<int>(json.unwrap()["online"].asInt().unwrapOr(-1));
                            nowOffline = static_cast<int>(json.unwrap()["offline"].asInt().unwrapOr(-1));
                        }
                    }
                }
                else {
                    nowOnline = nowOffline = -1;
                }
                std::this_thread::sleep_for(std::chrono::seconds(90));
            }
        }).detach();
    }

    std::string text() {
        int on = nowOnline, off = nowOffline;
        if (on < 0) {
            return "";
        }
        return fmt::format("{} online  ·  {} offline", on, off);
    }
}
