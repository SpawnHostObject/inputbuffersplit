//@ net/update_check.hpp
//@ Part of main.cpp (single translation unit). Not a standalone file: do not add it to the build.
    // v2.81 update check: once per game start, reads the version in the public repo's mod.json
    // (in the background) and says so on the main menu if a newer one is out.
    constexpr char const* REPO_URL = "https://github.com/raresdalas1009/-predictive-input-buffer";
    std::string latestVersion;
    bool updateChecked = false;
    bool updateShown = false;

    // v2.91 auto update: "Update now" downloads the .geode from the latest GitHub release and
    // puts it in place of this one; the new version loads on the next start. The file is only
    // installed if it really is this mod and a newer version than the one running.
    constexpr bool INSTALL_UPDATES = true;
    constexpr char const* RELEASE_API = "https://api.github.com/repos/raresdalas1009/-predictive-input-buffer/releases/latest";
    bool updating = false;
    std::string latestAssetUrl;  // v2.98: the .geode of the release the popup is about
    std::string latestNotes;     // test 90: what's new, from the CBF site
    bool updateAnywhere = false; // test 90: a check from the window shows its popup wherever you are

    // v2.98: the newest GitHub release and its .geode download (empty url if it has none)
    std::string releaseAsset(matjson::Value const& release) {
        auto assets = release["assets"];
        if (assets.isArray()) {
            for (auto const& asset : assets) {
                auto name = asset["name"].asString().unwrapOr("");
                if (name.size() > 6 && name.ends_with(".geode")) {
                    return asset["browser_download_url"].asString().unwrapOr("");
                }
            }
        }
        return "";
    }

    Result<VersionInfo> packageVersion(ByteVector const& data) {
        GEODE_UNWRAP_INTO(auto unzip, file::Unzip::create(ByteSpan(data)));
        GEODE_UNWRAP_INTO(auto bytes, unzip.extract("mod.json"));
        auto json = matjson::parse(std::string_view(reinterpret_cast<char const*>(bytes.data()), bytes.size()));
        if (json.isErr()) {
            return Err("the download has no readable mod.json");
        }
        auto info = json.unwrap();
        if (info["id"].asString().unwrapOr("") != Mod::get()->getID()) {
            return Err("the download is a different mod");
        }
        GEODE_UNWRAP_INTO(auto version, info["version"].asString());
        return VersionInfo::parse(version);
    }

    void updateDone(bool ok, std::string message) {
        updating = false;
        say("update: {}", message);
        if (!ok) {
            createQuickPopup("Update failed", message + "\nYou can still get it from the GitHub page.", "OK", "Open page",
                [](FLAlertLayer*, bool open) {
                    if (open) {
                        web::openLinkInBrowser(std::string(REPO_URL));
                    }
                });
            return;
        }
        createQuickPopup("Updated!", message + "\nRestart the game to use it.", "Later", "Restart", [](FLAlertLayer*, bool restart) {
            if (restart) {
                game::restart(true);
            }
        });
    }

    void installUpdate() {
        if (updating) {
            return;
        }
        updating = true;
        say("update: downloading...");
        std::thread([url = latestAssetUrl]() mutable {
            auto fail = [](std::string why) {
                queueInMainThread([why] { updateDone(false, why); });
            };
            if (url.empty()) {
                auto res = web::WebRequest()
                    .timeout(std::chrono::seconds(15))
                    .userAgent("ClickBeforeFrames")
                    .header("Accept", "application/vnd.github+json")
                    .getSync(RELEASE_API);
                if (!res.ok()) {
                    return fail(fmt::format("Couldn't reach GitHub (error {}).", res.code()));
                }
                auto json = res.json();
                if (json.isErr()) {
                    return fail("GitHub sent something unexpected.");
                }
                url = releaseAsset(json.unwrap());
            }
            if (url.empty()) {
                return fail("The latest release has no .geode file yet.");
            }
            auto file = web::WebRequest()
                .timeout(std::chrono::seconds(120))
                .userAgent("ClickBeforeFrames")
                .followRedirects(true)
                .getSync(url);
            if (!file.ok()) {
                return fail(fmt::format("The download failed (error {}).", file.code()));
            }
            auto data = std::move(file).data();
            auto version = packageVersion(data);
            if (version.isErr()) {
                return fail("The downloaded file isn't a valid Click Before Frames build.");
            }
            auto newVersion = version.unwrap();
            if (!(newVersion > Mod::get()->getVersion())) {
                return fail(fmt::format("The newest download is {} and you already have {}: it isn't published yet.",
                    newVersion.toVString(), Mod::get()->getVersion().toVString()));
            }
            auto written = file::writeBinarySafe(Mod::get()->getPackagePath(), ByteSpan(data));
            if (written.isErr()) {
                return fail("Couldn't write the new file into the mods folder.");
            }
            std::string done = fmt::format("Installed {}.", newVersion.toVString());
            queueInMainThread([done] { updateDone(true, done); });
        }).detach();
    }

    void showUpdatePopup() {
        if (updateShown || latestVersion.empty() || !CCScene::get()) {
            return;
        }
        updateShown = true;
        say("update: {} is out (you have {})", latestVersion, Mod::get()->getVersion().toVString());
        createQuickPopup("Click Before Frames", fmt::format("<cp>{}</c> is out! You have {}.{}", latestVersion,
            Mod::get()->getVersion().toVString(), latestNotes.empty() ? std::string() : "\n\n" + latestNotes), "Later", INSTALL_UPDATES ? "Update now" : "Open page", [](FLAlertLayer*, bool yes) {
                if (!yes) {
                    return;
                }
                if (INSTALL_UPDATES) {
                    installUpdate();
                }
                else {
                    web::openLinkInBrowser(std::string(REPO_URL));
                }
            });
    }

    void checkForUpdate();

    // test 90: the window's Updates button
    bool updateAvailable() {
        return !latestVersion.empty() && !latestAssetUrl.empty();
    }

    std::string updateButtonText() {
        if (updating) {
            return "Updating...";
        }
        if (!account::isLinked()) {
            return "Updates: Link Account";
        }
        return updateAvailable() ? "Update to " + latestVersion : std::string("Check for Updates");
    }

    void manualUpdate() {
        if (updating) {
            return;
        }
        if (!account::isLinked()) {
            say("update: link this game to your Discord first (Account tab)");
            return;
        }
        if (updateAvailable()) {
            installUpdate();
            return;
        }
        updateAnywhere = true;
        updateChecked = false;
        updateShown = false;
        say("update: checking...");
        checkForUpdate();
    }

    void checkForUpdate() {
        if (updateChecked || (!updateAnywhere && !Mod::get()->getSettingValue<bool>("check-updates"))) {
            return;
        }
        if (!account::isLinked()) {
            return;  // test 92: updates are for linked games
        }
        updateChecked = true;
        // v2.98: the check reads the newest GitHub release, the same file "Update now" installs
        // (it used to read the code's version, which could be ahead of the newest release: the
        // popup said "v2.97 is out" and the install then found only v2.95)
        std::thread([] {
            // test 90: the version uploaded on the CBF site's admin page comes first (GitHub otherwise)
            if (!std::string_view(online::URL).empty()) {
                auto site = web::WebRequest().timeout(std::chrono::seconds(10)).userAgent("ClickBeforeFrames")
                    .getSync(std::string(online::URL) + "/api/latest");
                if (site.ok()) {
                    if (auto j = site.json(); j.isOk()) {
                        auto v = j.unwrap();
                        auto ver = VersionInfo::parse(v["version"].asString().unwrapOr(""));
                        if (ver.isOk() && ver.unwrap() > Mod::get()->getVersion()) {
                            std::string name = ver.unwrap().toVString();
                            std::string url = std::string(online::URL) + "/download/latest";
                            std::string notes = v["notes"].asString().unwrapOr("");
                            queueInMainThread([name, url, notes] {
                                latestVersion = name;
                                latestAssetUrl = url;
                                latestNotes = notes;
                                say("update: {} is on the CBF site{}", name, notes.empty() ? "" : ": " + notes);
                                auto* scene = CCScene::get();
                                if (updateAnywhere || (scene && scene->getChildByID("MenuLayer"))) {
                                    showUpdatePopup();
                                }
                            });
                            return;
                        }
                        if (ver.isOk()) {
                            if (updateAnywhere) {
                                queueInMainThread([] { say("update: you have the newest version ({})", Mod::get()->getVersion().toVString()); });
                            }
                            return;  // the site has nothing newer: that's the newest
                        }
                    }
                }
            }
            auto res = web::WebRequest()
                .timeout(std::chrono::seconds(10))
                .userAgent("ClickBeforeFrames")
                .header("Accept", "application/vnd.github+json")
                .getSync(RELEASE_API);
            if (!res.ok()) {
                return;
            }
            auto json = res.json();
            if (json.isErr()) {
                return;
            }
            auto release = json.unwrap();
            std::string url = releaseAsset(release);
            if (url.empty()) {
                return;
            }
            auto latest = VersionInfo::parse(release["tag_name"].asString().unwrapOr(""));
            if (latest.isErr()) {
                latest = VersionInfo::parse(release["name"].asString().unwrapOr(""));
            }
            if (latest.isErr()) {
                // a release named something else: the version inside its .geode decides
                auto file = web::WebRequest()
                    .timeout(std::chrono::seconds(60))
                    .userAgent("ClickBeforeFrames")
                    .followRedirects(true)
                    .getSync(url);
                if (!file.ok()) {
                    return;
                }
                auto data = std::move(file).data();
                latest = packageVersion(data);
            }
            if (latest.isErr() || !(latest.unwrap() > Mod::get()->getVersion())) {
                return;
            }
            std::string name = latest.unwrap().toVString();
            queueInMainThread([name, url] {
                latestVersion = name;
                latestAssetUrl = url;
                auto* scene = CCScene::get();
                if (scene && scene->getChildByID("MenuLayer")) {
                    showUpdatePopup();
                }
            });
        }).detach();
    }
