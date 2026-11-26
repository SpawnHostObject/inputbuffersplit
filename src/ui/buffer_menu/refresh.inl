//@ ui/buffer_menu/refresh.inl
//@ Part of main.cpp (single translation unit). Not a standalone file: do not add it to the build.
    void refresh() {
        if (m_tpsLabel) {
            m_tpsLabel->setString(fmt::format("{}", Mod::get()->getSettingValue<int64_t>("tps")).c_str());
        }
        if (m_scaleLabel) {
            m_scaleLabel->setString(fmt::format("{:.1f}x", Mod::get()->getSavedValue<float>("ui-scale", 1.f)).c_str());
        }
        if (m_opacityLabel) {
            m_opacityLabel->setString(fmt::format("{:.0f}%", Mod::get()->getSavedValue<float>("ui-opacity", 1.f) * 100.f).c_str());
        }
        for (size_t i = 0; i < m_switches.size(); ++i) {
            bool on = optionValue(*m_options[i]);
            drawSwitch(m_switches[i], on);
            m_switchLabels[i]->setColor(on ? m_text : m_muted);
        }
        refreshModes();
    }

    void refreshModes() {
        for (int i = 0; i < 8; ++i) {
            if (m_modeValues[i]) {
                m_modeValues[i]->setString(fmt::format("{:g}f", buf.modeTicks[i] * TICK).c_str());
                m_modeValues[i]->setColor(buf.perMode ? m_text : m_muted);
            }
        }
    }

    void applyLook() {
        m_t = currentTheme();
        m_light = Mod::get()->getSavedValue<bool>("ui-light", false);
        s_siteFont = !m_light;
        m_bg = Mod::get()->getSavedValue<int>("ui-bg", 0);
        if (m_light) {
            m_text = { 34, 28, 56 };
            m_muted = { 105, 100, 135 };
            m_cyan = { 40, 120, 200 };
        }
        else {
            m_text = { 236, 236, 255 };
            m_muted = { 150, 150, 192 };
            m_cyan = { 110, 230, 255 };
        }
        m_mainLayer->setScale(std::clamp(Mod::get()->getSavedValue<float>("ui-scale", 1.f), 0.4f, 1.3f));
    }

    void rebuildAll() {
        applyLook();
        buildFrame();
        rebuildPage();
    }

    // ---------- motion: shooting stars ----------

    void refreshOnline() {
        if (!m_onlineLabel) {
            return;
        }
        auto t = online::text();
        m_onlineLabel->setString(t.c_str());
        m_onlineDot->setVisible(!t.empty());
        m_onlineDot->setPosition({ W - 37.f - m_onlineLabel->getScaledContentSize().width, FOOT / 2.f + 1.f });
    }
