//@ autoplay/button.hpp
//@ Part of main.cpp (single translation unit). Not a standalone file: do not add it to the build.
    // v3.0 test 12: autoplay presses on the player itself, like the ghost does, instead of through
    // the game's button handling. Click Between Frames moves clicks that go through there to a
    // sub-frame moment taken from the real clock, so the same plan landed a little differently
    // every attempt: replays drifted and the ghost's predictions missed.
    void autoplayButton(PlayLayer* layer, bool down) {
        buf.appliedInput = down;
        bool dual = layer->m_gameState.m_isDualMode && layer->m_player2;
        for (auto* p : { layer->m_player1, dual ? layer->m_player2 : nullptr }) {
            if (!p) {
                continue;
            }
            if (down) {
                p->pushButton(PlayerButton::Jump);
            }
            else {
                p->releaseButton(PlayerButton::Jump);
            }
        }
    }
