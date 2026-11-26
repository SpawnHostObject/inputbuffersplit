//@ ui/buffer_menu/touch.inl
//@ Part of main.cpp (single translation unit). Not a standalone file: do not add it to the build.
    // ---------- dragging by the logo or the header ----------

    bool ccTouchBegan(CCTouch* touch, CCEvent* event) override {
        auto p = m_mainLayer->convertToNodeSpace(touch->getLocation());
        bool logo = p.x >= 0.f && p.x <= SIDE && p.y >= H - 52.f && p.y <= H;
        bool header = p.x >= SIDE + 100.f && p.x <= W - 150.f && p.y >= H - HEAD && p.y <= H;
        if (logo || header) {
            m_dragging = true;
            return true;
        }
        return Popup::ccTouchBegan(touch, event);
    }

    void ccTouchMoved(CCTouch* touch, CCEvent* event) override {
        if (!m_dragging) {
            Popup::ccTouchMoved(touch, event);
            return;
        }
        auto delta = touch->getLocation() - touch->getPreviousLocation();
        m_mainLayer->setPosition(m_mainLayer->getPosition() + delta);
        s_pos = m_mainLayer->getPosition();
        s_moved = true;
    }

    void ccTouchEnded(CCTouch* touch, CCEvent* event) override {
        if (m_dragging) {
            m_dragging = false;
            return;
        }
        Popup::ccTouchEnded(touch, event);
    }
