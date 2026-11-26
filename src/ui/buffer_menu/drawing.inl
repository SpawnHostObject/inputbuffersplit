//@ ui/buffer_menu/drawing.inl
//@ Part of main.cpp (single translation unit). Not a standalone file: do not add it to the build.
    static CCTexture2D* noiseTexture() {
        static Ref<CCTexture2D> tex;
        if (!tex) {
            constexpr int N = 128;
            std::vector<uint8_t> px(N * N * 4);
            uint32_t seed = 2463534242u;
            for (int i = 0; i < N * N; ++i) {
                seed ^= seed << 13; seed ^= seed >> 17; seed ^= seed << 5;
                float v = static_cast<float>(seed & 0xffff) / 65535.f;
                uint8_t b = static_cast<uint8_t>(v * v * 235.f);
                px[i * 4] = px[i * 4 + 1] = px[i * 4 + 2] = b;
                px[i * 4 + 3] = 255;
            }
            auto* t = new CCTexture2D();
            t->initWithData(px.data(), kCCTexture2DPixelFormat_RGBA8888, N, N, CCSize(N, N));
            t->autorelease();
            ccTexParams params { GL_NEAREST, GL_NEAREST, GL_REPEAT, GL_REPEAT };
            t->setTexParameters(&params);
            tex = t;
        }
        return tex;
    }

    static std::vector<CCPoint> almond(float w, float h, int n = 18) {
        std::vector<CCPoint> pts;
        auto bez = [](float a, float b, float c, float d, float t) {
            float u = 1.f - t;
            return u * u * u * a + 3 * u * u * t * b + 3 * u * t * t * c + t * t * t * d;
        };
        for (int i = 0; i <= n; ++i) {
            float t = static_cast<float>(i) / n;
            pts.push_back({ bez(-w / 2, -w * 0.31f, w * 0.31f, w / 2, t), bez(0, h * 0.87f, h * 0.87f, 0, t) });
        }
        for (int i = 1; i < n; ++i) {
            float t = static_cast<float>(i) / n;
            pts.push_back({ bez(w / 2, w * 0.31f, -w * 0.31f, -w / 2, t), bez(0, -h * 0.87f, -h * 0.87f, 0, t) });
        }
        return pts;
    }

    // an eye like the site's: white, a blue iris, a red pupil; `iris` gets the part that looks around
    CCNode* makeEye(float w, CCNode** iris, ccColor3B irisColor = { 46, 107, 255 }, ccColor3B pupilColor = { 224, 20, 31 }) {
        float h = w * 0.62f;
        auto shape = almond(w, h / 2.f);
        auto* eye = CCNode::create();
        auto* stencil = CCDrawNode::create();
        stencil->drawPolygon(shape.data(), static_cast<unsigned>(shape.size()), { 1, 1, 1, 1 }, 0.f, { 0, 0, 0, 0 });
        auto* clip = CCClippingNode::create(stencil);
        eye->addChild(clip);
        auto* white = CCDrawNode::create();
        white->drawPolygon(shape.data(), static_cast<unsigned>(shape.size()), { 0.95f, 0.95f, 0.96f, 1.f }, 0.f, { 0, 0, 0, 0 });
        clip->addChild(white);
        auto* look = CCDrawNode::create();
        float r = h * 0.36f;
        circle(look, { 0.f, 0.f }, r, col(irisColor, 1.f), 28, 0.8f, col({ 6, 16, 48 }, 1.f));
        circle(look, { 0.f, 0.f }, r * 0.45f, col(pupilColor, 1.f), 20);
        circle(look, { -r * 0.35f, r * 0.35f }, r * 0.2f, col({ 255, 255, 255 }, 0.85f), 12);
        clip->addChild(look);
        auto* rim = CCDrawNode::create();
        rim->drawPolygon(shape.data(), static_cast<unsigned>(shape.size()), { 0, 0, 0, 0 }, 1.f, { 0.04f, 0.04f, 0.06f, 1.f });
        eye->addChild(rim);
        if (iris) {
            *iris = look;
        }
        return eye;
    }
    // test 66: the online count in the footer
    CCLabelBMFont* m_onlineLabel = nullptr;
    CCDrawNode* m_onlineDot = nullptr;
    float m_onlineTimer = 0.f;
    // test 65: motion (page cascade, theme wipe, closing, the autoplay panel)
    CCNode* m_side = nullptr;
    bool m_closing = false;
    bool m_wiping = false;
    static inline bool s_sideOut = false;
    static constexpr float PW = 150.f;
    static constexpr float PH = 192.f;

    static constexpr int TPS_MIN = 240;
    static constexpr int TPS_MAX = 1000;

    // ---------- drawing helpers ----------

    // CCDrawNode blends premultiplied colours: a see-through colour has to be darkened by its alpha
    static ccColor4F col(ccColor3B c, float a) {
        return { c.r / 255.f * a, c.g / 255.f * a, c.b / 255.f * a, a };
    }

    static void circle(CCDrawNode* d, CCPoint c, float r, ccColor4F fill, int segments = 28, float border = 0.f,
        ccColor4F borderColor = { 0, 0, 0, 0 }) {
        std::vector<CCPoint> pts;
        for (int i = 0; i < segments; ++i) {
            float a = i * 2.f * 3.14159265f / segments;
            pts.push_back({ c.x + r * std::cos(a), c.y + r * std::sin(a) });
        }
        d->drawPolygon(pts.data(), static_cast<unsigned>(pts.size()), fill, border, borderColor);
    }

    static void roundRect(CCDrawNode* d, CCRect r, float rad, ccColor4F fill, float border = 0.f, ccColor4F borderColor = { 0, 0, 0, 0 }) {
        rad = std::min({ rad, r.size.width / 2.f, r.size.height / 2.f });
        std::vector<CCPoint> pts;
        CCPoint centers[] = { { r.getMaxX() - rad, r.getMaxY() - rad }, { r.getMinX() + rad, r.getMaxY() - rad },
            { r.getMinX() + rad, r.getMinY() + rad }, { r.getMaxX() - rad, r.getMinY() + rad } };
        for (int c = 0; c < 4; ++c) {
            for (int i = 0; i <= 6; ++i) {
                float a = (c * 90.f + i * 15.f) * 3.14159265f / 180.f;
                CCPoint p { centers[c].x + rad * std::cos(a), centers[c].y + rad * std::sin(a) };
                if (pts.empty() || ccpDistance(pts.back(), p) > 0.05f) {
                    pts.push_back(p);
                }
            }
        }
        while (pts.size() > 2 && ccpDistance(pts.back(), pts.front()) <= 0.05f) {
            pts.pop_back();
        }
        d->drawPolygon(pts.data(), static_cast<unsigned>(pts.size()), fill, border, borderColor);
    }

    // v3.2: the website's pixel TV font (VT323) in the dark look; Exo 2 / Orbitron in the light one
    static inline bool s_siteFont = true;
    static char const* bodyFont() {
        return s_siteFont ? "vt323.fnt"_spr : "exo2.fnt"_spr;
    }

    static char const* headFont() {
        return s_siteFont ? "vt323.fnt"_spr : "orbitron.fnt"_spr;
    }

    static float fontK() {
        return s_siteFont ? 1.25f : 1.f;  // VT323's letters are smaller for the same size
    }

    static constexpr float HEAD_K = 0.8f;  // Orbitron is wide

    CCLabelBMFont* label(CCNode* parent, std::string const& str, float scale, ccColor3B color, CCPoint at, CCPoint anchor = { 0.5f, 0.5f },
        bool head = false) {
        auto* l = CCLabelBMFont::create(str.c_str(), head ? headFont() : bodyFont());
        l->setScale(scale * (head && !s_siteFont ? HEAD_K : 1.f) * fontK());
        l->setColor(color);
        l->setAnchorPoint(anchor);
        l->setPosition(at);
        parent->addChild(l, 3);
        return l;
    }

    ccColor4F cardFill() const {
        return m_light ? col({ 255, 255, 255 }, 0.55f) : col({ 44, 44, 52 }, 0.78f);
    }

    // a pill button: filled with the accent (primary), or glassy with an accent edge
    CCMenuItemSpriteExtra* button(CCMenu* menu, std::string const& str, SEL_MenuHandler handler, CCPoint center, float width,
        bool primary = false, int tag = 0, float height = 18.f, std::optional<ccColor3B> tint = std::nullopt) {
        auto* node = CCNode::create();
        node->setContentSize({ width, height });
        auto* d = CCDrawNode::create();
        ccColor3B base = tint.value_or(m_t.main);
        if (primary) {
            roundRect(d, { 0.f, 0.f, width, height }, height / 2.f, col(base, 0.9f), 0.6f, col(mix(base, { 255, 255, 255 }, 0.5f), 0.6f));
        }
        else {
            roundRect(d, { 0.f, 0.f, width, height }, height / 2.f, tint ? col(base, 0.28f) : cardFill(), 0.6f, col(base, 0.55f));
        }
        node->addChild(d);
        auto* l = CCLabelBMFont::create(str.c_str(), bodyFont());
        l->setScale(0.5f * fontK());
        l->setColor(primary ? ccColor3B { 255, 255, 255 } : m_text);
        l->setPosition({ width / 2.f, height / 2.f + 0.5f });
        l->limitLabelWidth(width - 8.f, 0.5f * fontK(), 0.2f);
        node->addChild(l);
        auto* item = CCMenuItemSpriteExtra::create(node, this, handler);
        item->m_scaleMultiplier = 1.05f;
        item->setTag(tag);
        item->setPosition(center);
        menu->addChild(item);
        return item;
    }

    void drawSwitch(CCDrawNode* d, bool on) {
        d->clear();
        roundRect(d, { 0.f, 0.f, 26.f, 13.f }, 6.5f, on ? col(m_t.main, 0.95f) : (m_light ? col({ 190, 188, 205 }, 0.95f) : col({ 70, 74, 110 }, 0.9f)),
            0.5f, col(on ? m_t.soft : ccColor3B { 110, 115, 160 }, 0.6f));
        circle(d, { on ? 19.5f : 6.5f, 6.5f }, 4.6f, col({ 255, 255, 255 }, on ? 1.f : 0.85f));
    }

    CCMenuItemSpriteExtra* switchItem(CCPoint center, bool on, SEL_MenuHandler handler, int tag, CCDrawNode** out) {
        auto* node = CCNode::create();
        node->setContentSize({ 26.f, 13.f });
        auto* d = CCDrawNode::create();
        node->addChild(d);
        drawSwitch(d, on);
        auto* item = CCMenuItemSpriteExtra::create(node, this, handler);
        item->m_scaleMultiplier = 1.08f;
        item->setTag(tag);
        item->setPosition(center);
        m_pageMenu->addChild(item);
        if (out) {
            *out = d;
        }
        return item;
    }

    // your icon for a game mode (MODE_NAMES order = the game's icon types), in your colours
    static CCNode* modeIcon(int mode, float scale = 0.5f) {
        auto* gm = GameManager::get();
        int ids[] = { gm->getPlayerFrame(), gm->getPlayerShip(), gm->getPlayerBall(), gm->getPlayerBird(), gm->getPlayerDart(),
            gm->getPlayerRobot(), gm->getPlayerSpider(), gm->getPlayerSwing() };
        auto* icon = SimplePlayer::create(0);
        icon->updatePlayerFrame(ids[mode], static_cast<IconType>(mode));
        icon->setColor(gm->colorForIdx(gm->getPlayerColor()));
        icon->setSecondColor(gm->colorForIdx(gm->getPlayerColor2()));
        if (gm->getPlayerGlow()) {
            icon->setGlowOutline(gm->colorForIdx(gm->getPlayerGlowColor()));
        }
        icon->setScale(mode == 5 || mode == 6 ? scale * 0.84f : scale);
        return icon;
    }

    static bool optionValue(Option const& o) {
        return o.key ? Mod::get()->getSettingValue<bool>(o.key) : buf.showPath;
    }

    // a setting row: name (and a grey line under it) on the left, switch on the right
    void optionRow(Option const& o, float x, float y, float width, float rowH) {
        auto* d0 = CCDrawNode::create();
        roundRect(d0, { x, y - rowH / 2.f + 1.f, width, rowH - 2.f }, 6.f, m_light ? col({ 255, 255, 255 }, 0.6f) : col({ 255, 255, 255 }, 0.035f), 0.4f,
            col(m_t.main, 0.25f));
        m_page->addChild(d0, 1);
        CCDrawNode* d = nullptr;
        auto* sw = switchItem({ x + width - 18.f, y }, optionValue(o), menu_selector(BufferMenu::onToggle), static_cast<int>(m_options.size()), &d);
        if (rowH < 17.f) {
            sw->setScale(0.8f);
            sw->m_baseScale = 0.8f;  // so a press doesn't spring it back to full size
        }
        bool two = o.sub && rowH >= 19.f;
        float ls = rowH < 17.f ? 0.42f : 0.5f;
        auto* l = label(m_page, o.label, ls, optionValue(o) ? m_text : m_muted, { x + 7.f, two ? y + 4.f : y }, { 0.f, 0.5f });
        l->limitLabelWidth(width - 44.f, ls, 0.2f);
        if (two) {
            label(m_page, o.sub, 0.32f, m_muted, { x + 7.f, y - 5.5f }, { 0.f, 0.5f })->limitLabelWidth(width - 44.f, 0.32f, 0.15f);
        }
        m_options.push_back(&o);
        m_switches.push_back(d);
        m_switchLabels.push_back(l);
    }

    // a translucent card with a title in its top left corner
    void card(CCRect r, std::string const& title = "", std::string const& right = "") {
        auto* d = CCDrawNode::create();
        if (m_light) {
            roundRect(d, r, 9.f, cardFill(), 0.6f, col(m_t.main, 0.45f));
        }
        else {
            // v3.2: an old TV like on the website: a dark case, a grey screen inside with scan lines
            roundRect(d, { r.origin.x - 1.5f, r.origin.y - 1.5f, r.size.width + 3.f, r.size.height + 3.f }, 10.5f, col({ 22, 21, 26 }, 0.97f), 0.6f,
                col({ 255, 255, 255 }, 0.08f));
            CCRect in { r.origin.x + 2.f, r.origin.y + 2.f, r.size.width - 4.f, r.size.height - 4.f };
            roundRect(d, in, 8.f, col({ 50, 51, 58 }, 0.92f), 0.5f, col({ 0, 0, 0 }, 0.7f));
            for (float y = in.getMinY() + 2.f; y < in.getMaxY() - 2.f; y += 3.f) {
                d->drawSegment({ in.getMinX() + 3.f, y }, { in.getMaxX() - 3.f, y }, 0.3f, { 0.f, 0.f, 0.f, 0.22f });
            }
        }
        m_page->addChild(d, 1);
        if (!title.empty()) {
            label(m_page, title, 0.5f, m_text, { r.getMinX() + 9.f, r.getMaxY() - 10.f }, { 0.f, 0.5f });
        }
        if (!right.empty()) {
            label(m_page, right, 0.36f, m_muted, { r.getMaxX() - 9.f, r.getMaxY() - 10.f }, { 1.f, 0.5f });
        }
    }

    float contentX() const {
        return SIDE + 6.f;
    }

    float contentW() const {
        return W - SIDE - 14.f;
    }

    float contentTop() const {
        return H - HEAD - 6.f;
    }

    // a small "- value +" stepper; returns the value label
    CCLabelBMFont* stepper(CCPoint right, SEL_MenuHandler handler, int tagBase = 0) {
        button(m_pageMenu, "-", handler, { right.x - 56.f, right.y }, 18.f, false, tagBase - 1, 16.f);
        button(m_pageMenu, "+", handler, { right.x - 9.f, right.y }, 18.f, false, tagBase + 1, 16.f);
        return label(m_page, "", 0.5f, m_text, { right.x - 32.5f, right.y });
    }

    static std::string platformName() {
#if defined(GEODE_IS_WINDOWS)
        return "Windows - 64-bit";
#elif defined(GEODE_IS_ANDROID64)
        return "Android - 64-bit";
#elif defined(GEODE_IS_ANDROID32)
        return "Android - 32-bit";
#elif defined(GEODE_IS_IOS)
        return "iOS";
#elif defined(GEODE_IS_MACOS)
        return "macOS";
#else
        return "";
#endif
    }

    static std::string chipText() {
        if (buf.perMode) {
            return "CUSTOM";
        }
        if (!buf.enabled) {
            return "OFF";
        }
        for (size_t i = 0; i < std::size(STAGE_BTN); ++i) {
            if (STAGE_BTN[i] == buf.stage) {
                std::string s = STAGE_LABEL[i];
                for (auto& c : s) {
                    c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
                }
                return s;
            }
        }
        return "SETTINGS";
    }
