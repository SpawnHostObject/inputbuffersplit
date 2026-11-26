//@ core/stage_cycle.hpp
//@ Part of main.cpp (single translation unit). Not a standalone file: do not add it to the build.
    void cycleStage(int dir = 1) {
        buf.stage = (buf.stage + dir + STAGE_COUNT) % STAGE_COUNT;
        resetPending();
        // A delayed input that was waiting is dropped; the game gets what you're really
        // holding now (otherwise, with the buffer switched off, it never would).
        auto* layer = GameManager::get()->m_playLayer;
        if (layer && !buf.autoReleased && buf.appliedInput != buf.realInput) {
            sendButton(layer, buf.realInput);
        }
        applyStage();
        readSettings();
        saveProfile();
        Mod::get()->setSavedValue("stage", buf.stage);  // v2.58: one stage for every level, kept on restart
        say("Stage (6) -> {}", stageText());
        refreshAutoTag();
        refreshWatermark();
        refreshPanel();
    }

    // v2.4 wave auto-release (practice mode only): while you hold in wave, check every
    // tick whether holding crashes within the next `autoWindow` ticks. If it does and
    // letting go now clearly survives longer, let go for you. Your own release later is
    // swallowed, and your next press works as normal.
    // v3.0 autoplay, for level showcases: the hidden ghost looks ahead every physics tick and
    // the mod presses / lets go only when keeping the button as it is would crash sooner than
    // switching. The level is put in test mode, so the game saves nothing it plays; your own
    // clicks are ignored while it runs, and AUTOPLAY stays on screen.
