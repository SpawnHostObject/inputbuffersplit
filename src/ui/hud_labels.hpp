//@ ui/hud_labels.hpp
//@ Part of main.cpp (single translation unit). Not a standalone file: do not add it to the build.
    // v2.14: a run outside practice where auto-release can press for you is marked on screen.
    // v2.46 watermark: whenever the buffer is on (any stage but OFF) the level shows it, and
    // can't be turned off, so runs made with the mod are recognisable.
    void refreshWatermark() {
        if (!buf.watermark) {
            return;
        }
        // hidden in practice mode (practice runs can't be submitted, and on phones it sat on the
        // checkpoint buttons); always shown in normal mode while the buffer is on
        auto* layer = GameManager::get()->m_playLayer;
        buf.watermark->setVisible(buf.enabled && !(layer && layer->m_isPracticeMode));
        buf.watermark->setString(fmt::format("CBF  |  {}", stageText()).c_str());
    }

    void refreshAutoTag() {
        if (!buf.autoTag) {
            return;
        }
        auto* layer = GameManager::get()->m_playLayer;
        bool on = buf.enabled && buf.autoNormalRuns && (buf.autoRelease || buf.robotAutoRelease || buf.dashAutoRelease || buf.autoClick) &&
            layer && !layer->m_isPracticeMode;
        buf.autoTag->setVisible(on);
    }
