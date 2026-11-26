//@ ui/buffer_menu/motion.inl
//@ Part of main.cpp (single translation unit). Not a standalone file: do not add it to the build.
    // ---------- test 65: motion ----------

    // the page's pieces come in one after another from the top: cards first, then what's on them
    void cascadeIn() {
        auto pop = [this](CCNode* n) {
            auto p = n->getPosition();
            bool drawn = typeinfo_cast<CCDrawNode*>(n) != nullptr || n->getContentSize().width <= 0.f;
            float delay = drawn ? 0.f : std::clamp((H - p.y) / H, 0.f, 1.f) * 0.14f;
            n->setPosition(p + CCPoint { 0.f, -8.f });
            CCFiniteTimeAction* move = CCEaseExponentialOut::create(CCMoveTo::create(0.32f, p));
            if (!drawn) {
                float sc = n->getScale();
                n->setScale(sc * 0.9f);
                move = CCSpawn::create(move, CCEaseBackOut::create(CCScaleTo::create(0.32f, sc)), nullptr);
            }
            n->runAction(CCSequence::create(CCDelayTime::create(delay), move, nullptr));
        };
        for (auto* n : CCArrayExt<CCNode*>(m_page->getChildren())) {
            if (n != m_pageMenu) {
                pop(n);
            }
        }
        for (auto* n : CCArrayExt<CCNode*>(m_pageMenu->getChildren())) {
            pop(n);
        }
    }

    // a colour change: a band of the new colour sweeps over the window, the window changes under it
    void wipe() {
        if (m_wiping) {
            rebuildAll();
            return;
        }
        m_wiping = true;
        auto t = currentTheme();
        auto* grow = CCDrawNode::create();
        roundRect(grow, { 0.f, 0.f, W, H }, 14.f, col(t.main, 0.95f));
        grow->setScaleX(0.001f);
        m_mainLayer->addChild(grow, 60);
        grow->runAction(CCSequence::create(CCEaseSineIn::create(CCScaleTo::create(0.2f, 1.f, 1.f)),
            CallFuncExt::create([this, t]() {
                rebuildAll();
                auto* shrink = CCDrawNode::create();
                roundRect(shrink, { -W, 0.f, W, H }, 14.f, col(t.main, 0.95f));
                shrink->setPositionX(W);
                m_mainLayer->addChild(shrink, 61);
                shrink->runAction(CCSequence::create(CCEaseSineOut::create(CCScaleTo::create(0.3f, 0.001f, 1.f)),
                    CallFuncExt::create([this]() { m_wiping = false; }), CCRemoveSelf::create(), nullptr));
            }),
            CCRemoveSelf::create(), nullptr));
    }

    // autoplay's own panel slides out from under the window (on the side with room) while it plays
    void buildSide() {
        if (m_side) {
            m_side->removeFromParent();
            m_side = nullptr;
        }
        bool want = autoplay.active;
        bool was = s_sideOut;
        s_sideOut = want;
        if (!want && !was) {
            return;
        }
        auto win = CCDirector::get()->getWinSize();
        bool right = m_mainLayer->convertToWorldSpace({ W + 8.f + PW, 0.f }).x <= win.width;
        float outX = right ? W + 8.f : -8.f - PW;
        float inX = right ? W - PW - 12.f : 12.f;
        float y0 = H - HEAD - 4.f - PH;
        auto* side = CCNode::create();
        m_mainLayer->addChild(side, -5);
        auto* d = CCDrawNode::create();
        ccColor3B back = BACKS[std::clamp(m_bg, 0, static_cast<int>(std::size(BACKS)) - 1)];
        roundRect(d, { 0.f, y0, PW, PH }, 11.f, m_light ? col({ 246, 245, 252 }, 0.98f) : col(back, 0.98f), 0.8f, col(m_t.main, 0.6f));
        side->addChild(d);
        label(side, "Autoplay", 0.55f, m_text, { 10.f, y0 + PH - 13.f }, { 0.f, 0.5f });
        label(side, "test mode", 0.32f, m_t.main, { PW - 10.f, y0 + PH - 13.f }, { 1.f, 0.5f });
        if (want) {
            label(side, fmt::format("{} inputs played", autoplay.inputs), 0.36f, m_muted, { 10.f, y0 + PH - 30.f }, { 0.f, 0.5f })
                ->limitLabelWidth(PW - 20.f, 0.36f, 0.15f);
            label(side, autoplay.fromMacro.empty() ? std::string("AUTOPLAY stays on screen") : "macro: " + autoplay.fromMacro, 0.32f,
                autoplay.fromMacro.empty() ? m_muted : m_cyan, { 10.f, y0 + PH - 42.f }, { 0.f, 0.5f })
                ->limitLabelWidth(PW - 20.f, 0.32f, 0.15f);
            auto* menu = CCMenu::create();
            menu->setPosition({ 0.f, 0.f });
            side->addChild(menu, 2);
            bool take = Mod::get()->getSavedValue<bool>("autoplay-takeover", false);
            button(menu, take ? "Take Over: On" : "Take Over: Off", menu_selector(BufferMenu::onTakeOver), { PW / 2.f, y0 + PH - 60.f }, PW - 20.f,
                false, 0, 18.f, take ? std::optional<ccColor3B>(ccColor3B { 80, 200, 120 }) : std::nullopt);
            button(menu, "Forget Level", menu_selector(BufferMenu::onForget), { PW / 2.f, y0 + PH - 82.f }, PW - 20.f);
            button(menu, "Show On Screen", menu_selector(BufferMenu::onConsole), { PW / 2.f, y0 + PH - 104.f }, PW - 20.f);
            button(menu, "Choose Macro", menu_selector(BufferMenu::onChooseMacro), { PW / 2.f, y0 + PH - 126.f }, PW - 20.f, true);  // alpha.31
            float hb = (PW - 26.f) / 2.f;
            button(menu, "Macros", menu_selector(BufferMenu::onMacros), { 10.f + hb / 2.f, y0 + PH - 148.f }, hb);
            button(menu, "Forget Macro", menu_selector(BufferMenu::onForgetMacro), { 16.f + hb * 1.5f, y0 + PH - 148.f }, hb);
            button(menu, "Stop Autoplay", menu_selector(BufferMenu::onAutoplay), { PW / 2.f, y0 + 18.f }, PW - 20.f, false, 0, 20.f,
                ccColor3B { 235, 80, 90 });
            side->setPositionX(was ? outX : inX);
            if (!was) {
                side->runAction(CCEaseExponentialOut::create(CCMoveTo::create(0.45f, { outX, 0.f })));
            }
            m_side = side;
        }
        else {
            // autoplay just stopped: in it goes, then it's gone
            side->setPositionX(outX);
            side->runAction(CCSequence::create(CCEaseExponentialIn::create(CCMoveTo::create(0.3f, { inX, 0.f })), CCRemoveSelf::create(), nullptr));
        }
    }

    // test 85: the game's popup opening scales the window to 1x, over your UI Scale (0.7 opened at
    // full size): the opening is redone to your scale
    void show() override {
        Popup::show();
        float sc = std::clamp(Mod::get()->getSavedValue<float>("ui-scale", 1.f), 0.4f, 1.3f);
        m_mainLayer->stopAllActions();
        m_mainLayer->setScale(sc * 0.85f);
        m_mainLayer->runAction(CCEaseBackOut::create(CCScaleTo::create(0.25f, sc)));
    }

    // closing: the window shrinks back and fades, then goes
    void keyBackClicked() override {
        if (m_closing) {
            return;
        }
        m_closing = true;
        if (m_side) {
            m_side->stopAllActions();
            float x = m_side->getPositionX() > W / 2.f ? W - PW - 12.f : 12.f;
            m_side->runAction(CCEaseExponentialIn::create(CCMoveTo::create(0.16f, { x, 0.f })));
        }
        float sc = m_mainLayer->getScale();
        m_mainLayer->runAction(CCEaseBackIn::create(CCScaleTo::create(0.2f, sc * 0.8f)));
        this->runAction(CCFadeTo::create(0.2f, 0));
        Ref<BufferMenu> self = this;
        this->runAction(CCSequence::create(CCDelayTime::create(0.21f), CallFuncExt::create([self]() {
            Loader::get()->queueInMainThread([self]() { self->Popup::keyBackClicked(); });
        }), nullptr));
    }
