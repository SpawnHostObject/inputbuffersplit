//@ ui/buffer_menu/pages.inl
//@ Part of main.cpp (single translation unit). Not a standalone file: do not add it to the build.
    void buildTabs() {
        if (m_tabsNode) {
            m_tabsNode->removeFromParent();
        }
        m_tabsNode = CCNode::create();
        m_mainLayer->addChild(m_tabsNode, 4);
        auto* menu = CCMenu::create();
        menu->setPosition({ 0.f, 0.f });
        m_tabsNode->addChild(menu);
        for (int i = 0; i < static_cast<int>(std::size(TABS)); ++i) {
            // test 91: 5 tabs between the logo and the theme box (the 4th one's highlight went over the box)
            float th = 19.f;
            float y = H - 63.f - i * 21.5f;
            bool active = i == s_tab || (s_tab >= TAB_TUNING && i == s_back);
            float tw = SIDE - 22.f;
            auto* node = CCNode::create();
            node->setContentSize({ tw, th });
            auto* d = CCDrawNode::create();
            if (active) {
                roundRect(d, { 0.f, 0.f, tw, th }, 7.f, col(m_t.main, m_light ? 0.25f : 0.22f), 0.6f, col(m_t.main, 0.7f));
            }
            circle(d, { 10.f, th / 2.f }, 2.2f, active ? col(m_t.soft, 1.f) : col(m_muted, 0.7f), 14);
            node->addChild(d);
            auto* l = CCLabelBMFont::create(TABS[i], bodyFont());
            l->setScale(0.56f * fontK());
            l->limitLabelWidth(tw - 26.f, 0.56f * fontK(), 0.2f);
            l->setColor(active ? m_text : m_muted);
            l->setAnchorPoint({ 0.f, 0.5f });
            l->setPosition({ 19.f, th / 2.f + 0.5f });
            node->addChild(l);
            auto* item = CCMenuItemSpriteExtra::create(node, this, menu_selector(BufferMenu::onTab));
            item->m_scaleMultiplier = 1.04f;
            item->setTag(i);
            item->setPosition({ SIDE / 2.f + 1.f, y });
            menu->addChild(item);
        }
    }

    // ---------- pages ----------

    void pageTitle(std::string const& t) {
        label(m_page, t, 0.7f, m_text, { SIDE + 12.f, H - HEAD / 2.f }, { 0.f, 0.5f });
    }

    void pageLog() {
        pageTitle("Log");
        float x = contentX(), w = contentW(), top = contentTop();
        float bottom = FOOT + 30.f;
        card({ x, bottom, w, top - bottom }, "What the buffer did", "newest at the bottom");
        size_t rows = 15;
        size_t n = exportLines.size();
        size_t first = n > rows ? n - rows : 0;
        float y = top - 24.f;
        if (n == 0) {
            label(m_page, "Nothing yet: play a level and your clicks show up here.", 0.42f, m_muted, { x + 10.f, y }, { 0.f, 0.5f });
        }
        for (size_t i = first; i < n; ++i) {
            auto* l = label(m_page, exportLines[i], 0.34f, i + 1 == n ? m_text : m_muted, { x + 10.f, y }, { 0.f, 0.5f });
            l->limitLabelWidth(w - 20.f, 0.34f, 0.15f);
            y -= 11.f;
        }
        float bw = (w - 12.f) / 3.f;
        float by = FOOT + 15.f;
        button(m_pageMenu, "Copy Log", menu_selector(BufferMenu::onCopyLog), { x + bw / 2.f, by }, bw, true);
        button(m_pageMenu, buf.panelOpen ? "Hide On Screen" : "Show On Screen", menu_selector(BufferMenu::onConsole), { x + bw * 1.5f + 6.f, by }, bw);
        button(m_pageMenu, "All Settings", menu_selector(BufferMenu::onAllSettings), { x + bw * 2.5f + 12.f, by }, bw);
    }

    void pageStage() {
        pageTitle("Stage");
        float x = contentX(), w = contentW(), top = contentTop();
        // stage buttons
        float sh = 52.f;
        card({ x, top - sh, w, sh }, "Stage", "how long an early click can be held back");
        size_t count = std::size(STAGE_BTN) + 1;
        float bw = (w - 16.f - (count - 1) * 4.f) / count;
        for (size_t i = 0; i < count; ++i) {
            bool isCustom = i == std::size(STAGE_BTN);
            bool on = isCustom ? buf.perMode : (!buf.perMode && STAGE_BTN[i] == buf.stage);
            button(m_pageMenu, isCustom ? "Custom" : STAGE_LABEL[i], menu_selector(BufferMenu::onStageButton),
                { x + 8.f + bw / 2.f + i * (bw + 4.f), top - 26.f }, bw, on, static_cast<int>(i), 17.f);
        }
        label(m_page, buf.perMode ? std::string("Custom: your own frames for each game mode (Open Tuning)") : stageText(), 0.38f, m_muted,
            { x + w / 2.f, top - 45.f })->limitLabelWidth(w - 20.f, 0.38f, 0.2f);

        float colW = (w - 6.f) / 2.f;
        float y0 = top - sh - 6.f;
        // options (left)
        float oh = 20.f + std::size(STAGE_OPTIONS) * 21.f;
        card({ x, y0 - oh, colW, oh }, "Options");
        for (size_t i = 0; i < std::size(STAGE_OPTIONS); ++i) {
            optionRow(STAGE_OPTIONS[i], x + 5.f, y0 - 30.f - i * 21.f, colW - 10.f, 20.f);
        }
        // quick (left, under options)
        float qTop = y0 - oh - 6.f;
        float qBottom = FOOT + 6.f;
        card({ x, qBottom, colW, qTop - qBottom }, "Quick", "made by BASU");
        // the two rows sit evenly in what's left under the title (v3 test 6: they hung out of the card)
        float inner = qTop - 20.f - qBottom;
        float r1 = qTop - 20.f - inner * 0.3f;
        float r2 = qBottom + inner * 0.28f;
        float qb = (colW - 22.f) / 2.f;
        button(m_pageMenu, "Run Check", menu_selector(BufferMenu::onRunCheck), { x + colW / 2.f, r1 }, colW - 16.f, true);
        button(m_pageMenu, "Changelog", menu_selector(BufferMenu::onChangelog), { x + 8.f + qb / 2.f, r2 }, qb);
        button(m_pageMenu, "Statistics", menu_selector(BufferMenu::onStats), { x + 14.f + qb * 1.5f, r2 }, qb);
        // custom (right)
        float rx = x + colW + 6.f;
        card({ rx, FOOT + 6.f, colW, y0 - FOOT - 6.f }, "Custom", "per gamemode");
        float cellW = (colW - 10.f) / 4.f;
        for (int i = 0; i < 8; ++i) {
            float cx = rx + 5.f + cellW * (i % 4 + 0.5f);
            float cy = y0 - 44.f - (i / 4) * 52.f;
            auto* icon = modeIcon(i, 0.42f);
            icon->setPosition({ cx, cy });
            m_page->addChild(icon, 3);
            bool off = buf.modeOff & (3u << (i * 2));
            m_modeValues[i] = label(m_page, "", 0.42f, m_text, { cx, cy - 18.f });
            if (off) {
                label(m_page, "off", 0.32f, m_muted, { cx, cy - 27.f });
            }
        }
        button(m_pageMenu, "Open Tuning", menu_selector(BufferMenu::onTuning), { rx + colW / 2.f, FOOT + 20.f }, colW - 16.f, true);
        refreshModes();
    }

    void pageModes() {
        pageTitle("Modes");
        float x = contentX(), w = contentW(), top = contentTop();
        float bottom = FOOT + 30.f;
        float cw = (w - 3 * 5.f) / 4.f;
        float ch = (top - bottom - 5.f) / 2.f;
        int on = 0;
        for (int i = 0; i < 8; ++i) {
            float cx = x + (i % 4) * (cw + 5.f);
            float cy = top - (i / 4 + 1) * ch - (i / 4) * 5.f;
            card({ cx, cy, cw, ch });
            bool normal = !(buf.modeOff & (1u << (i * 2)));
            bool mini = !(buf.modeOff & (1u << (i * 2 + 1)));
            on += normal + mini;
            auto* d = CCDrawNode::create();
            ccColor3B lamp = normal && mini ? ccColor3B { 90, 230, 130 } : normal || mini ? ccColor3B { 255, 200, 80 } : ccColor3B { 240, 80, 90 };
            circle(d, { cx + cw - 9.f, cy + ch - 9.f }, 3.4f, col(lamp, 1.f), 18);
            circle(d, { cx + cw - 9.f, cy + ch - 9.f }, 6.f, col(lamp, 0.2f), 18);
            m_page->addChild(d, 2);
            auto* icon = modeIcon(i, 0.62f);
            icon->setPosition({ cx + cw / 2.f, cy + ch - 30.f });
            m_page->addChild(icon, 3);
            label(m_page, MODE_NAMES[i], 0.55f, m_text, { cx + cw / 2.f, cy + ch * 0.36f });
            float pw = (cw - 14.f) / 2.f;
            button(m_pageMenu, "Normal", menu_selector(BufferMenu::onModeSize), { cx + 5.f + pw / 2.f, cy + 12.f }, pw, normal, i * 2, 15.f);
            button(m_pageMenu, "Mini", menu_selector(BufferMenu::onModeSize), { cx + 9.f + pw * 1.5f, cy + 12.f }, pw, mini, i * 2 + 1, 15.f);
        }
        button(m_pageMenu, "All On", menu_selector(BufferMenu::onAllModes), { x + 34.f, FOOT + 14.f }, 62.f, false, 1, 17.f, ccColor3B { 80, 200, 120 });
        button(m_pageMenu, "All Off", menu_selector(BufferMenu::onAllModes), { x + 102.f, FOOT + 14.f }, 62.f, false, 0, 17.f, ccColor3B { 235, 80, 90 });
        label(m_page, fmt::format("{} / 16 buffered  -  off = clicks go through raw", on), 0.38f, m_muted, { x + w - 4.f, FOOT + 14.f }, { 1.f, 0.5f });
    }

    void pageTools() {
        pageTitle("Tools");
        float x = contentX(), w = contentW(), top = contentTop();
        float colW = (w - 6.f) / 2.f;
        float ah = 18.f + std::size(ASSISTS) * 23.f;
        card({ x, top - ah, colW, ah }, "Assists");
        for (size_t i = 0; i < std::size(ASSISTS); ++i) {
            optionRow(ASSISTS[i], x + 5.f, top - 30.f - i * 23.f, colW - 10.f, 22.f);
        }
        float sTop = top - ah - 6.f;
        card({ x, FOOT + 6.f, colW, sTop - FOOT - 6.f }, "Your Stats");
        std::string stats = statsText();
        float y = sTop - 24.f;
        size_t start = 0;
        for (int i = 0; i < 3 && start < stats.size(); ++i) {
            size_t end = stats.find('\n', start);
            std::string line = stats.substr(start, end == std::string::npos ? std::string::npos : end - start);
            label(m_page, line, 0.34f, m_text, { x + 9.f, y }, { 0.f, 0.5f })->limitLabelWidth(colW - 18.f, 0.34f, 0.15f);
            y -= 11.f;
            if (end == std::string::npos) {
                break;
            }
            start = end + 1;
        }
        button(m_pageMenu, "Export Report", menu_selector(BufferMenu::onCopyLog), { x + colW / 2.f, FOOT + 20.f }, colW - 16.f, true);

        float rx = x + colW + 6.f;
        card({ rx, FOOT + 6.f, colW, top - FOOT - 6.f }, "Autoplay", "for showcases");
        label(m_page, "Plays the level for you to record a showcase.", 0.38f, m_text, { rx + 9.f, top - 30.f }, { 0.f, 0.5f })
            ->limitLabelWidth(colW - 18.f, 0.38f, 0.15f);
        label(m_page, "Nothing it plays is saved as progress, and", 0.36f, m_muted, { rx + 9.f, top - 44.f }, { 0.f, 0.5f })
            ->limitLabelWidth(colW - 18.f, 0.36f, 0.15f);
        label(m_page, "AUTOPLAY stays on screen while it runs.", 0.36f, m_muted, { rx + 9.f, top - 55.f }, { 0.f, 0.5f })
            ->limitLabelWidth(colW - 18.f, 0.36f, 0.15f);
        bool inLevel = GameManager::get()->m_playLayer != nullptr;
        button(m_pageMenu, autoplay.active ? "Stop Autoplay" : "Start Autoplay", menu_selector(BufferMenu::onAutoplay),
            { rx + colW / 2.f, top - 82.f }, colW - 16.f, !autoplay.active, 0, 22.f,
            autoplay.active ? std::optional<ccColor3B>(ccColor3B { 235, 80, 90 }) : std::nullopt);
        label(m_page,
            autoplay.active ? fmt::format("Playing - {} inputs so far. Your clicks are ignored.", autoplay.inputs)
                            : inLevel ? std::string("Starts from where you are. Close the menu to watch.")
                                      : std::string("Open a level, then start it from the pause menu."),
            0.36f, m_muted, { rx + colW / 2.f, top - 102.f })->limitLabelWidth(colW - 16.f, 0.36f, 0.15f);
        label(m_page, "Not for platformer levels. 2-player: with a macro.", 0.34f, m_muted, { rx + colW / 2.f, top - 114.f })
            ->limitLabelWidth(colW - 16.f, 0.34f, 0.15f);
        {
            float y = top - 136.f;
            auto* d0 = CCDrawNode::create();
            roundRect(d0, { rx + 5.f, y - 11.f, colW - 10.f, 22.f }, 6.f, m_light ? col({ 255, 255, 255 }, 0.6f) : col({ 255, 255, 255 }, 0.035f), 0.4f,
                col(m_t.main, 0.25f));
            m_page->addChild(d0, 1);
            bool on = Mod::get()->getSavedValue<bool>("autoplay-takeover", false);
            label(m_page, "Take Over", 0.5f, on ? m_text : m_muted, { rx + 12.f, y + 4.f }, { 0.f, 0.5f });
            label(m_page, "Your clicks first, it plays on when you stop", 0.3f, m_muted, { rx + 12.f, y - 5.5f }, { 0.f, 0.5f })
                ->limitLabelWidth(colW - 50.f, 0.3f, 0.15f);
            switchItem({ rx + colW - 23.f, y }, on, menu_selector(BufferMenu::onTakeOver), 0, nullptr);
        }
        float bw = (colW - 22.f) / 2.f;
        button(m_pageMenu, "Forget Level", menu_selector(BufferMenu::onForget), { rx + 8.f + bw / 2.f, FOOT + 20.f }, bw);
        button(m_pageMenu, "Show On Screen", menu_selector(BufferMenu::onConsole), { rx + 14.f + bw * 1.5f, FOOT + 20.f }, bw);
    }

    // test 91: your account: link this game to your Discord on the CBF site, and your autoplay's stats
    void pageAccount() {
        pageTitle("Account");
        float x = contentX(), w = contentW(), top = contentTop();
        float colW = (w - 6.f) / 2.f;
        auto const& st = account::stats;
        if (!st.loaded && account::status.empty()) {
            account::refresh();
        }
        card({ x, FOOT + 6.f, colW, top - FOOT - 6.f }, "Your account", "CBF site");
        std::string savedName = Mod::get()->getSavedValue<std::string>("account-name", "");
        bool linked = st.loaded ? st.linked : !savedName.empty();
        std::string name = st.loaded ? st.name : savedName;
        float y = top - 34.f;
        if (linked) {
            label(m_page, name, 0.75f, m_text, { x + colW / 2.f, y - 6.f })->limitLabelWidth(colW - 20.f, 0.75f, 0.2f);
            label(m_page, "linked with Discord", 0.36f, m_cyan, { x + colW / 2.f, y - 26.f });
            if (st.games > 1) {
                label(m_page, fmt::format("{} games linked to it", st.games), 0.34f, m_muted, { x + colW / 2.f, y - 40.f });
            }
        }
        else {
            label(m_page, "Link this game to your Discord", 0.42f, m_text, { x + colW / 2.f, y }) ->limitLabelWidth(colW - 20.f, 0.42f, 0.15f);
            label(m_page, "to see your stats here and on the site.", 0.36f, m_muted, { x + colW / 2.f, y - 13.f })->limitLabelWidth(colW - 20.f, 0.36f, 0.15f);
            button(m_pageMenu, account::linking ? "Waiting for the browser..." : "Link with Discord", menu_selector(BufferMenu::onLinkAccount),
                { x + colW / 2.f, y - 42.f }, colW - 24.f, !account::linking, 0, 22.f, ccColor3B { 88, 101, 242 });
        }
        if (!account::status.empty()) {
            label(m_page, account::status, 0.32f, m_muted, { x + colW / 2.f, FOOT + 46.f })->limitLabelWidth(colW - 16.f, 0.32f, 0.12f);
        }
        float bw = (colW - 22.f) / 2.f;
        button(m_pageMenu, "Refresh", menu_selector(BufferMenu::onAccountRefresh), { x + 8.f + bw / 2.f, FOOT + 20.f }, bw);
        button(m_pageMenu, "Open Site", menu_selector(BufferMenu::onOpenSite), { x + 14.f + bw * 1.5f, FOOT + 20.f }, bw);

        float rx = x + colW + 6.f;
        card({ rx, FOOT + 6.f, colW, top - FOOT - 6.f }, "Your autoplay", "global brain");
        auto stat = [&](float yy, std::string const& n, std::string const& what) {
            label(m_page, n, 0.7f, m_text, { rx + 12.f, yy }, { 0.f, 0.5f });
            label(m_page, what, 0.34f, m_muted, { rx + 58.f, yy }, { 0.f, 0.5f })->limitLabelWidth(colW - 66.f, 0.34f, 0.12f);
        };
        float sy = top - 36.f;
        stat(sy, fmt::format("{}", st.levelsBeaten), "levels beaten 0-100");
        stat(sy - 24.f, fmt::format("{}", st.runsShared), "runs you shared first");
        stat(sy - 48.f, fmt::format("{}", st.othersUsed), "finishes by others with your runs");
        stat(sy - 72.f, fmt::format("{}", st.finishes), "runs finished in all");
        if (!st.recent.empty()) {
            std::string list;
            for (size_t i = 0; i < st.recent.size() && i < 3; ++i) {
                list += (i ? ", " : "") + st.recent[i];
            }
            label(m_page, "Lately: " + list, 0.32f, m_muted, { rx + colW / 2.f, FOOT + 22.f })->limitLabelWidth(colW - 16.f, 0.32f, 0.12f);
        }
        else if (!st.loaded) {
            label(m_page, "Loading...", 0.34f, m_muted, { rx + colW / 2.f, FOOT + 22.f });
        }
    }

    void pageTuning() {
        pageTitle("Tuning");
        float x = contentX(), w = contentW(), top = contentTop();
        button(m_pageMenu, "Done", menu_selector(BufferMenu::onDone), { W - 98.f - 30.f, H - HEAD / 2.f }, 50.f, true, 0, 16.f);
        float colW = (w - 6.f) / 2.f;
        float rh = (top - FOOT - 30.f) / 4.f;
        for (int i = 0; i < 8; ++i) {
            float cx = x + (i / 4) * (colW + 6.f);
            float cy = top - (i % 4 + 1) * rh;
            card({ cx, cy + 2.f, colW, rh - 4.f });
            auto* icon = modeIcon(i, 0.45f);
            icon->setPosition({ cx + 18.f, cy + rh / 2.f + 4.f });
            m_page->addChild(icon, 3);
            label(m_page, MODE_NAMES[i], 0.36f, m_muted, { cx + 18.f, cy + 9.f });
            label(m_page, i == 4 ? "Click & release" : "Click", 0.46f, m_text, { cx + 40.f, cy + rh / 2.f }, { 0.f, 0.5f });
            m_modeValues[i] = stepper({ cx + colW - 6.f, cy + rh / 2.f }, menu_selector(BufferMenu::onModeStep), i * 10);
        }
        label(m_page, buf.perMode ? "Used now (Custom stage). Half-frame steps, up to 10 frames."
                                  : "Pick Custom on the Stage page to use these. Half-frame steps, up to 10 frames.",
            0.36f, m_muted, { x + w / 2.f, FOOT + 12.f })->limitLabelWidth(w - 10.f, 0.36f, 0.15f);
        refreshModes();
    }

    void pageSettings() {
        pageTitle("Settings");
        float x = contentX(), w = contentW(), top = contentTop();
        button(m_pageMenu, "Done", menu_selector(BufferMenu::onDone), { W - 98.f - 30.f, H - HEAD / 2.f }, 50.f, true, 0, 16.f);
        float colW = (w - 6.f) / 2.f;
        // window + helpers (left)
        card({ x, FOOT + 6.f, colW, top - FOOT - 6.f }, "Window");
        float y = top - 28.f;
        {
            auto* d0 = CCDrawNode::create();
            m_page->addChild(d0, 1);
            CCDrawNode* sw = nullptr;
            roundRect(d0, { x + 5.f, y - 10.f, colW - 10.f, 20.f }, 6.f, m_light ? col({ 255, 255, 255 }, 0.6f) : col({ 255, 255, 255 }, 0.035f), 0.4f,
                col(m_t.main, 0.25f));
            label(m_page, "Pause Menu Button", 0.48f, m_text, { x + 12.f, y + 3.5f }, { 0.f, 0.5f });
            label(m_page, "The heart button in the pause menu", 0.3f, m_muted, { x + 12.f, y - 5.f }, { 0.f, 0.5f });
            switchItem({ x + colW - 23.f, y }, Mod::get()->getSavedValue<bool>("pause-button", true), menu_selector(BufferMenu::onPauseButton), 0, &sw);
            y -= 21.f;
        }
        auto row = [&](char const* name) {
            auto* d0 = CCDrawNode::create();
            roundRect(d0, { x + 5.f, y - 9.f, colW - 10.f, 18.f }, 6.f, m_light ? col({ 255, 255, 255 }, 0.6f) : col({ 255, 255, 255 }, 0.035f), 0.4f,
                col(m_t.main, 0.25f));
            m_page->addChild(d0, 1);
            label(m_page, name, 0.48f, m_text, { x + 12.f, y }, { 0.f, 0.5f });
        };
        row("Window Size");
        m_scaleLabel = stepper({ x + colW - 9.f, y }, menu_selector(BufferMenu::onScale));
        y -= 19.f;
        row("Window Opacity");
        m_opacityLabel = stepper({ x + colW - 9.f, y }, menu_selector(BufferMenu::onOpacity));
        y -= 19.f;
        row("Physics TPS");
        m_tpsLabel = stepper({ x + colW - 9.f, y }, menu_selector(BufferMenu::onTps));
        y -= 17.f;
        label(m_page, "Helpers", 0.44f, m_text, { x + 9.f, y }, { 0.f, 0.5f });
        // the helper rows share what's left of the card, down to its bottom edge
        float rowsTop = y - 12.f;
        float rowH = std::min(16.f, (rowsTop - (FOOT + 12.f)) / (std::size(HELPERS) - 0.5f));
        y = rowsTop;
        for (size_t i = 0; i < std::size(HELPERS); ++i) {
            optionRow(HELPERS[i], x + 5.f, y, colW - 10.f, rowH);
            y -= rowH;
        }
        // gameplay (right)
        float rx = x + colW + 6.f;
        card({ rx, FOOT + 6.f, colW, top - FOOT - 6.f }, "Gameplay", "prediction");
        float ry = top - 30.f;
        float prH = std::min(20.f, (ry - (FOOT + 40.f)) / (std::size(PREDICTION) - 0.5f));
        for (size_t i = 0; i < std::size(PREDICTION); ++i) {
            optionRow(PREDICTION[i], rx + 5.f, ry, colW - 10.f, prH);
            ry -= prH;
        }
        // test 90: updates (from the CBF site) next to All Geode Settings
        float hb = (colW - 22.f) / 2.f;
        button(m_pageMenu, "All Geode Settings", menu_selector(BufferMenu::onAllSettings), { rx + 8.f + hb / 2.f, FOOT + 18.f }, hb);
        button(m_pageMenu, updateButtonText(), menu_selector(BufferMenu::onUpdates), { rx + 14.f + hb * 1.5f, FOOT + 18.f }, hb, updateAvailable());
    }

    void rebuildPage() {
        if (m_page) {
            m_page->removeFromParent();
        }
        m_tpsLabel = m_scaleLabel = m_opacityLabel = nullptr;
        m_modeValues.fill(nullptr);
        m_options.clear();
        m_switches.clear();
        m_switchLabels.clear();
        m_page = CCNode::create();
        m_mainLayer->addChild(m_page, 3);
        m_pageMenu = CCMenu::create();
        m_pageMenu->setPosition({ 0.f, 0.f });
        m_page->addChild(m_pageMenu, 5);

        switch (s_tab) {
            case 0: pageLog(); break;
            case 2: pageModes(); break;
            case 3: pageTools(); break;
            case TAB_ACCOUNT: pageAccount(); break;
            case TAB_TUNING: pageTuning(); break;
            case TAB_SETTINGS: pageSettings(); break;
            default: pageStage(); break;
        }
        buildTabs();
        buildHeaderButtons();
        cascadeIn();
        buildSide();
        handleTouchPriority(this);
        refresh();
    }
