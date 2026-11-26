//@ ui/buffer_menu/update.inl
//@ Part of main.cpp (single translation unit). Not a standalone file: do not add it to the build.
    void update(float dt) override {
        m_onlineTimer += dt;
        if (m_onlineTimer > 1.f) {
            m_onlineTimer = 0.f;
            refreshOnline();
        }
        if (account::dirty) {
            account::dirty = false;
            if (s_tab == TAB_ACCOUNT) {
                rebuildPage();  // test 91: the account page shows what just came in
            }
        }
        // v3.2: the static moves, the logo eye looks at the cursor and blinks, eyes show up in the snow
        m_staticT += dt;
        if (m_static && m_staticT > 0.05f) {
            m_staticT = 0.f;
            m_static->setTextureRect(CCRect(static_cast<float>(std::rand() % 128), static_cast<float>(std::rand() % 128), W, H));
        }
        if (m_eye && m_eyeIris) {
            CCPoint target { 0.f, 0.f };
#ifdef GEODE_IS_DESKTOP
            auto local = m_eye->convertToNodeSpace(geode::cocos::getMousePos());
            float dist = std::max(1.f, ccpLength(local));
            float k = std::min(1.f, dist / 160.f);
            target = CCPoint(local.x / dist * m_eyeW * 0.2f * k, local.y / dist * m_eyeW * 0.09f * k);
#else
            m_eyeWander -= dt;
            if (m_eyeWander <= 0.f) {
                m_eyeWander = 1.2f + static_cast<float>(std::rand() % 200) / 100.f;
                m_eyeTX = (static_cast<float>(std::rand() % 200) / 100.f - 1.f) * m_eyeW * 0.2f;
                m_eyeTY = (static_cast<float>(std::rand() % 200) / 100.f - 1.f) * m_eyeW * 0.08f;
            }
            target = CCPoint(m_eyeTX, m_eyeTY);
#endif
            float a = std::min(1.f, dt * 12.f);
            m_eyeX += (target.x - m_eyeX) * a;
            m_eyeY += (target.y - m_eyeY) * a;
            m_eyeIris->setPosition({ m_eyeX, m_eyeY });
            m_blinkIn -= dt;
            if (m_blinkIn <= 0.f && m_blink < 0.f) {
                m_blink = 0.f;
                m_blinkIn = 1.8f + static_cast<float>(std::rand() % 420) / 100.f;
            }
            if (m_blink >= 0.f) {
                m_blink += dt / 0.22f;
                float c = m_blink < 0.5f ? m_blink * 2.f : std::max(0.f, 2.f - m_blink * 2.f);
                m_eye->setScaleY(std::max(0.05f, 1.f - c));
                if (m_blink >= 1.f) {
                    m_blink = -1.f;
                    m_eye->setScaleY(1.f);
                }
            }
        }
        m_ghostIn -= dt;
        if (m_ghostIn <= 0.f && m_sky && m_static) {
            m_ghostIn = 2.5f + static_cast<float>(std::rand() % 400) / 100.f;
            static constexpr ccColor3B IRIS[] = { { 46, 107, 255 }, { 159, 207, 42 }, { 255, 46, 59 }, { 122, 74, 34 }, { 200, 200, 210 } };
            auto ic = IRIS[std::rand() % std::size(IRIS)];
            float w = 16.f + static_cast<float>(std::rand() % 16);
            auto* g = makeEye(w, nullptr, ic, ic.r > 200 && ic.g < 80 ? ccColor3B { 10, 0, 0 } : ccColor3B { 224, 20, 31 });
            g->setPosition({ SIDE + 20.f + static_cast<float>(std::rand() % static_cast<int>(W - SIDE - 40.f)),
                FOOT + 14.f + static_cast<float>(std::rand() % static_cast<int>(H - HEAD - FOOT - 28.f)) });
            g->setScaleY(0.05f);
            m_sky->addChild(g);
            g->runAction(CCSequence::create(CCBlink::create(0.3f, 3), CCScaleTo::create(0.12f, 1.f, 1.f), CCDelayTime::create(1.1f),
                CCScaleTo::create(0.08f, 1.f, 0.05f), CCScaleTo::create(0.1f, 1.f, 1.f), CCDelayTime::create(0.9f), CCBlink::create(0.25f, 3),
                CCScaleTo::create(0.1f, 1.f, 0.02f), CCRemoveSelf::create(), nullptr));
        }
        m_shootTimer -= dt;
        if (false && m_shootTimer <= 0.f && m_sky && !m_light) {
            m_shootTimer = 3.5f + static_cast<float>(std::rand() % 400) / 100.f;
            auto* streak = CCLayerGradient::create({ 255, 255, 255, 0 }, { 255, 255, 255, 220 }, { 1.f, 0.f });
            streak->setContentSize({ 34.f, 1.2f });
            streak->ignoreAnchorPointForPosition(false);
            streak->setAnchorPoint({ 1.f, 0.5f });
            float sx = 60.f + static_cast<float>(std::rand() % static_cast<int>(W - 60.f));
            streak->setPosition({ sx, H + 10.f });
            streak->setRotation(28.f);
            m_sky->addChild(streak);
            streak->runAction(CCSequence::create(
                CCSpawn::create(CCMoveBy::create(0.9f, { -170.f, -90.f }), CCSequence::create(CCDelayTime::create(0.5f), CCFadeOut::create(0.4f), nullptr),
                    nullptr),
                CCRemoveSelf::create(), nullptr));
        }
    }
