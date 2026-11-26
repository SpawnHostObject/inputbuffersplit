//@ ui/buffer_menu/frame.inl
//@ Part of main.cpp (single translation unit). Not a standalone file: do not add it to the build.
    // ---------- the window ----------

    void buildFrame() {
        if (m_frame) {
            m_frame->removeFromParent();
        }
        m_frame = CCNode::create();
        m_mainLayer->addChild(m_frame, -3);
        float opacity = std::clamp(Mod::get()->getSavedValue<float>("ui-opacity", 1.f), 0.5f, 1.f);

        auto* glow = CCDrawNode::create();
        for (int i = 7; i >= 1; --i) {
            float g = i * 1.7f;
            roundRect(glow, { -g, -g, W + 2.f * g, H + 2.f * g }, 14.f + g, col(m_t.main, 0.025f));
        }
        m_frame->addChild(glow, -3);

        auto* stencil = CCDrawNode::create();
        roundRect(stencil, { 0.f, 0.f, W, H }, 14.f, { 1, 1, 1, 1 });
        auto* clip = CCClippingNode::create(stencil);
        m_frame->addChild(clip, -2);
        ccColor3B back = Mod::get()->hasSavedValue("ui-bg") ? BACKS[std::clamp(m_bg, 0, static_cast<int>(std::size(BACKS)) - 1)]
                                                             : ccColor3B { 20, 20, 24 };  // v3.2: the website's near-black
        GLubyte a = static_cast<GLubyte>(253 * opacity);
        CCLayerGradient* sky = nullptr;
        if (m_light) {
            ccColor3B lo = mix({ 214, 208, 234 }, bright(back), 0.25f);
            sky = CCLayerGradient::create({ 240, 237, 250, a }, { lo.r, lo.g, lo.b, a });
        }
        else {
            sky = CCLayerGradient::create({ back.r, back.g, back.b, a },
                { static_cast<GLubyte>(back.r / 3), static_cast<GLubyte>(back.g / 3), static_cast<GLubyte>(back.b / 3), a });
        }
        sky->setContentSize({ W, H });
        clip->addChild(sky);
        uint32_t seed = 1234567;
        auto rnd = [&seed] {
            seed = seed * 1103515245u + 12345u;
            return static_cast<float>((seed >> 8) & 0xffff) / 65535.f;
        };
        m_static = nullptr;
        if (!m_light) {
            // v3.2: TV static over the background, with scan lines, like the website
            m_static = CCSprite::createWithTexture(noiseTexture(), CCRect(0.f, 0.f, W, H));
            m_static->setAnchorPoint({ 0.f, 0.f });
            m_static->setOpacity(46);
            clip->addChild(m_static);
            auto* scan = CCDrawNode::create();
            for (float y = 0.f; y < H; y += 3.f) {
                scan->drawSegment({ 0.f, y }, { W, y }, 0.35f, { 0.f, 0.f, 0.f, 0.28f });
            }
            clip->addChild(scan);
        }
        if (false) {
            // (the old stars, drifting slowly, twinkling)
            for (int layer = 0; layer < 2; ++layer) {
                auto* field = CCNode::create();
                clip->addChild(field);
                CCPoint by = layer == 0 ? CCPoint { -8.f, 3.f } : CCPoint { -18.f, 7.f };
                float period = layer == 0 ? 22.f : 14.f;
                field->runAction(CCRepeatForever::create(CCSequence::create(CCEaseSineInOut::create(CCMoveBy::create(period, by)),
                    CCEaseSineInOut::create(CCMoveBy::create(period, ccpNeg(by))), nullptr)));
                int count = layer == 0 ? 120 : 45;
                for (int i = 0; i < count; ++i) {
                    float size = layer == 0 ? 0.5f + rnd() * 0.5f : 0.9f + rnd() * 0.8f;
                    auto* star = CCLayerColor::create({ 255, 255, 255, 255 }, size, size);
                    star->setPosition({ rnd() * (W + 50.f) - 25.f, rnd() * (H + 30.f) - 15.f });
                    float r = rnd();
                    star->setColor(r < 0.15f ? m_cyan : r < 0.3f ? m_t.soft : ccColor3B { 255, 255, 255 });
                    GLubyte hi = static_cast<GLubyte>((layer == 0 ? 100 : 160) + rnd() * 85);
                    GLubyte lo = static_cast<GLubyte>(15 + rnd() * 45);
                    star->setOpacity(hi);
                    float t = 0.6f + rnd() * 2.2f;
                    star->runAction(CCRepeatForever::create(CCSequence::create(CCDelayTime::create(rnd() * 3.f),
                        CCFadeTo::create(t, lo), CCFadeTo::create(t, hi), nullptr)));
                    field->addChild(star);
                }
            }
        }
        // a big soft accent orb behind the content, like a planet glow
        auto* orb = CCDrawNode::create();
        for (int i = 10; i >= 1; --i) {
            circle(orb, { W * 0.62f, H * 0.42f }, 18.f * i, col(m_t.main, m_light ? 0.012f : 0.018f), 48);
        }
        clip->addChild(orb);
        m_sky = clip;

        // the sidebar panel, the outer edge, the header line, the footer line
        auto* chrome = CCDrawNode::create();
        roundRect(chrome, { 0.f, 0.f, W, H }, 14.f, { 0, 0, 0, 0 }, 0.8f, col(m_t.main, 0.8f));
        roundRect(chrome, { 6.f, 6.f, SIDE - 8.f, H - 12.f }, 11.f, m_light ? col({ 255, 255, 255 }, 0.45f) : col({ 0, 0, 0 }, 0.3f), 0.6f,
            col(m_t.main, 0.35f));
        m_frame->addChild(chrome, -1);
        auto* line = CCLayerGradient::create({ m_t.main.r, m_t.main.g, m_t.main.b, 230 }, { m_t.soft.r, m_t.soft.g, m_t.soft.b, 40 }, { 1.f, 0.f });
        line->setContentSize({ W - SIDE - 14.f, 1.5f });
        line->setPosition({ SIDE + 6.f, H - HEAD });
        m_frame->addChild(line, -1);
        auto* foot = CCLayerColor::create({ m_t.main.r, m_t.main.g, m_t.main.b, 60 }, W - SIDE - 14.f, 0.6f);
        foot->setPosition({ SIDE + 6.f, FOOT });
        m_frame->addChild(foot, -1);
        // test 66: who's online (from the CBF server)
        m_onlineLabel = label(m_frame, online::text(), 0.34f, m_muted, { W - 32.f, FOOT / 2.f + 1.f }, { 1.f, 0.5f });
        m_onlineDot = CCDrawNode::create();
        circle(m_onlineDot, { 0.f, 0.f }, 2.6f, col({ 102, 224, 154 }, 1.f), 14);
        m_frame->addChild(m_onlineDot);
        m_onlineDot->setVisible(false);
        refreshOnline();

        // the logo (v3.2): an eye that follows your cursor and blinks, CBF next to it, the version under
        m_eyeW = 30.f;
        m_eye = makeEye(m_eyeW, &m_eyeIris);
        m_eye->setPosition({ 13.f + m_eyeW / 2.f, H - 27.f });
        m_frame->addChild(m_eye, 2);
        for (int i = 3; i >= 1; --i) {
            auto* g = label(m_frame, "CBF", 0.95f + i * 0.04f, m_t.main, { 48.f - i * 1.2f, H - 25.f }, { 0.f, 0.5f }, true);
            g->setOpacity(static_cast<GLubyte>(35 / i));
        }
        label(m_frame, "CBF", 0.95f, m_light ? m_t.main : m_t.soft, { 48.f, H - 25.f }, { 0.f, 0.5f }, true);
        label(m_frame, Mod::get()->getVersion().toVString(), 0.3f, m_muted, { 48.f, H - 43.f }, { 0.f, 0.5f }, true)
            ->limitLabelWidth(SIDE - 56.f, 0.3f, 0.15f);
        auto* sep = CCLayerColor::create({ m_t.main.r, m_t.main.g, m_t.main.b, 110 }, SIDE - 28.f, 1.f);
        sep->setPosition({ 14.f, H - 52.f });
        m_frame->addChild(sep);

        // the theme box at the bottom of the sidebar
        auto* box = CCDrawNode::create();
        roundRect(box, { 12.f, 30.f, SIDE - 20.f, 96.f }, 9.f, m_light ? col({ 255, 255, 255 }, 0.6f) : col({ 255, 255, 255 }, 0.04f), 0.6f,
            col(m_t.main, 0.45f));
        m_frame->addChild(box);
        label(m_frame, platformName(), 0.4f, m_muted, { SIDE / 2.f + 1.f, 17.f });

        if (m_frameMenu) {
            m_frameMenu->removeFromParent();
        }
        m_frameMenu = CCMenu::create();
        m_frameMenu->setPosition({ 0.f, 0.f });
        m_mainLayer->addChild(m_frameMenu, 5);

        // dark / light: a pill with a moon and a sun, the picked half filled
        {
            float bw = SIDE - 32.f;
            auto* node = CCNode::create();
            node->setContentSize({ bw, 16.f });
            auto* d = CCDrawNode::create();
            roundRect(d, { 0.f, 0.f, bw, 16.f }, 8.f, m_light ? col({ 200, 196, 215 }, 0.8f) : col({ 0, 0, 0 }, 0.35f), 0.5f, col(m_t.main, 0.5f));
            roundRect(d, { m_light ? bw / 2.f : 0.f, 0.f, bw / 2.f, 16.f }, 8.f, col(m_t.main, 0.95f));
            // v3.2: dark: a switched-off TV (black screen, a dot of light); light: a TV with a white screen
            auto tv = [&](CCPoint c, bool on) {
                ccColor4F body = col({ 255, 255, 255 }, 0.95f), dark = col({ 14, 13, 18 }, 1.f);
                roundRect(d, { c.x - 7.f, c.y - 4.6f, 14.f, 9.2f }, 2.2f, col({ 34, 32, 40 }, 1.f), 0.55f, body);
                roundRect(d, { c.x - 5.6f, c.y - 3.3f, 9.4f, 6.6f }, 1.6f, on ? col({ 255, 255, 255 }, 1.f) : dark, 0.35f, col({ 0, 0, 0 }, 1.f));
                if (on) {
                    for (float y = c.y - 2.6f; y < c.y + 3.f; y += 1.4f) {
                        d->drawSegment({ c.x - 5.f, y }, { c.x + 3.2f, y }, 0.2f, col({ 170, 170, 185 }, 0.6f));
                    }
                }
                else {
                    circle(d, { c.x - 0.9f, c.y }, 0.6f, col({ 255, 255, 255 }, 0.9f), 8);
                }
                circle(d, { c.x + 5.3f, c.y + 1.4f }, 0.7f, col({ 200, 200, 210 }, 1.f), 8);
                circle(d, { c.x + 5.3f, c.y - 1.2f }, 0.7f, on ? col({ 255, 46, 59 }, 1.f) : col({ 90, 90, 100 }, 1.f), 8);
                d->drawSegment({ c.x - 1.5f, c.y + 4.6f }, { c.x - 4.f, c.y + 7.2f }, 0.35f, body);
                d->drawSegment({ c.x + 1.5f, c.y + 4.6f }, { c.x + 4.f, c.y + 7.2f }, 0.35f, body);
            };
            tv({ bw * 0.25f, 7.f }, false);
            tv({ bw * 0.75f, 7.f }, true);
            node->addChild(d);
            auto* item = CCMenuItemSpriteExtra::create(node, this, menu_selector(BufferMenu::onLight));
            item->m_scaleMultiplier = 1.04f;
            item->setPosition({ SIDE / 2.f + 1.f, 112.f });
            m_frameMenu->addChild(item);
        }
        label(m_frame, "Accent", 0.36f, m_muted, { 20.f, 97.f }, { 0.f, 0.5f });
        label(m_frame, "Background", 0.36f, m_muted, { 20.f, 64.f }, { 0.f, 0.5f });
        int accent = Mod::get()->getSavedValue<int>("ui-accent", 0);
        float step = (SIDE - 40.f) / 6.f;
        for (int i = 0; i < 7; ++i) {
            bool custom = i == 6;
            ccColor3B c = custom ? hueColor(Mod::get()->getSavedValue<int>("ui-accent-hue", 262)) : ACCENTS[i];
            dot({ 22.f + i * step, 84.f }, c, accent == i, custom, menu_selector(BufferMenu::onAccent), i);
            dot({ 22.f + i * step, 51.f }, bright(BACKS[i]), m_bg == i, false, menu_selector(BufferMenu::onBack), i);
        }

        // header: the stage chip and the close button; footer: the hint and the settings gear
        m_chip = nullptr;
        buildHeaderButtons();
        label(m_frame, "PAUSE MENU BUTTON OR MENU KEY TO TOGGLE", 0.32f, m_muted, { SIDE + 12.f, FOOT / 2.f + 1.f }, { 0.f, 0.5f }, true);
        {
            auto* node = CCNode::create();
            node->setContentSize({ 16.f, 16.f });
            auto* d = CCDrawNode::create();
            circle(d, { 8.f, 8.f }, 7.5f, cardFill(), 28, 0.5f, col(m_t.main, 0.7f));
            for (int i = 0; i < 8; ++i) {
                float an = i * 3.14159265f / 4.f;
                d->drawSegment({ 8.f + 3.f * std::cos(an), 8.f + 3.f * std::sin(an) }, { 8.f + 5.2f * std::cos(an), 8.f + 5.2f * std::sin(an) }, 0.9f,
                    col(m_text, 0.95f));
            }
            circle(d, { 8.f, 8.f }, 3.2f, col(m_text, 0.95f), 20);
            circle(d, { 8.f, 8.f }, 1.5f, m_light ? col({ 230, 228, 240 }, 1.f) : col({ 20, 16, 34 }, 1.f), 14);
            node->addChild(d);
            auto* item = CCMenuItemSpriteExtra::create(node, this, menu_selector(BufferMenu::onSettings));
            item->setPosition({ W - 18.f, FOOT / 2.f + 1.f });
            m_frameMenu->addChild(item);
        }
    }

    // v3.2: each colour is an eye with that colour as its iris; the picked one is wide open and ringed
    void dot(CCPoint at, ccColor3B c, bool picked, bool plus, SEL_MenuHandler handler, int tag) {
        auto* node = CCNode::create();
        node->setContentSize({ 12.f, 12.f });
        auto* d = CCDrawNode::create();
        // (almond's height is its curves' pull: the eye is about 0.65 of it tall each way)
        float w = picked ? 13.f : 11.f, h = picked ? 6.6f : 5.6f;
        auto shape = almond(w, h);
        for (auto& p : shape) {
            p = p + CCPoint { 6.f, 6.f };
        }
        if (picked) {
            auto ring = almond(w + 3.f, h + 2.4f);
            for (auto& p : ring) {
                p = p + CCPoint { 6.f, 6.f };
            }
            d->drawPolygon(ring.data(), static_cast<unsigned>(ring.size()), col(m_light ? ccColor3B { 40, 30, 60 } : ccColor3B { 255, 255, 255 }, 0.9f), 0.f,
                { 0, 0, 0, 0 });
        }
        d->drawPolygon(shape.data(), static_cast<unsigned>(shape.size()), { 0.95f, 0.95f, 0.96f, 1.f }, 0.5f, { 0.05f, 0.05f, 0.07f, 1.f });
        float r = picked ? 3.1f : 2.6f;
        circle(d, { 6.f, 6.f }, r, col(c, 1.f), 18);
        circle(d, { 6.f, 6.f }, r * 0.42f, col({ 8, 8, 10 }, 1.f), 12);
        circle(d, { 5.f, 7.f }, r * 0.25f, col({ 255, 255, 255 }, 0.85f), 8);
        if (plus) {
            d->drawSegment({ 4.2f, 6.f }, { 7.8f, 6.f }, 0.5f, col({ 255, 255, 255 }, 1.f));
            d->drawSegment({ 6.f, 4.2f }, { 6.f, 7.8f }, 0.5f, col({ 255, 255, 255 }, 1.f));
        }
        node->addChild(d);
        auto* item = CCMenuItemSpriteExtra::create(node, this, handler);
        item->m_scaleMultiplier = 1.2f;
        item->setTag(tag);
        item->setPosition(at);
        m_frameMenu->addChild(item);
    }

    CCNode* m_headButtons = nullptr;

    void buildHeaderButtons() {
        if (m_headButtons) {
            m_headButtons->removeFromParent();
        }
        m_headButtons = CCNode::create();
        m_mainLayer->addChild(m_headButtons, 6);
        auto* menu = CCMenu::create();
        menu->setPosition({ 0.f, 0.f });
        m_headButtons->addChild(menu);
        // close: a ring with an x
        auto* node = CCNode::create();
        node->setContentSize({ 18.f, 18.f });
        auto* d = CCDrawNode::create();
        circle(d, { 9.f, 9.f }, 8.f, cardFill(), 28, 0.5f, col(m_t.main, 0.8f));
        d->drawSegment({ 6.2f, 6.2f }, { 11.8f, 11.8f }, 0.6f, col(m_text, 0.95f));
        d->drawSegment({ 6.2f, 11.8f }, { 11.8f, 6.2f }, 0.6f, col(m_text, 0.95f));
        node->addChild(d);
        auto* close = CCMenuItemSpriteExtra::create(node, this, menu_selector(BufferMenu::onCloseButton));
        close->setPosition({ W - 18.f, H - HEAD / 2.f });
        menu->addChild(close);
        // the stage chip: tap it to go to the Stage page
        std::string chip = chipText();
        float cw = std::max(46.f, 12.f + chip.size() * 7.f);
        auto* cnode = CCNode::create();
        cnode->setContentSize({ cw, 16.f });
        auto* cd = CCDrawNode::create();
        roundRect(cd, { 0.f, 0.f, cw, 16.f }, 8.f, cardFill(), 0.6f, col(m_t.main, 0.8f));
        cnode->addChild(cd);
        m_chip = CCLabelBMFont::create(chip.c_str(), headFont());
        m_chip->setScale(s_siteFont ? 0.42f : 0.32f);
        m_chip->setColor(m_text);
        m_chip->setPosition({ cw / 2.f, 8.5f });
        cnode->addChild(m_chip);
        auto* chipItem = CCMenuItemSpriteExtra::create(cnode, this, menu_selector(BufferMenu::onChip));
        chipItem->setPosition({ W - 38.f - cw / 2.f, H - HEAD / 2.f });
        menu->addChild(chipItem);
    }
